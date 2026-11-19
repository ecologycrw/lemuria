#include "Lemuria/Classic/World.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <variant>

namespace lemuria::classic {

namespace {

[[nodiscard]] bool is_on_map(const EntityAppeared& appeared, const TileGrid& grid) noexcept {
    return grid.contains(appeared.position) && (!appeared.walking_to || grid.contains(*appeared.walking_to));
}

[[nodiscard]] bool is_on_map(const EntityWalked& walked, const TileGrid& grid) noexcept {
    const auto is_inside = [&grid](TileCoord tile) { return grid.contains(tile); };
    return grid.contains(walked.destination) && (!walked.origin || grid.contains(*walked.origin)) &&
           std::ranges::all_of(walked.steps.view(), is_inside);
}

[[nodiscard]] bool are_neighbors(TileCoord first, TileCoord second) noexcept {
    const int x_distance = std::abs(first.x - second.x);
    const int z_distance = std::abs(first.z - second.z);
    return x_distance <= 1 && z_distance <= 1 && x_distance + z_distance > 0;
}

// Every step must start where the previous one ended. The tiles were already checked to be on the map.
[[nodiscard]] bool is_connected_route(TileCoord start, std::span<const TileCoord> steps) noexcept {
    TileCoord previous = start;
    for (const TileCoord step : steps) {
        if (!are_neighbors(previous, step)) {
            return false;
        }
        previous = step;
    }
    return true;
}

[[nodiscard]] bool is_usable_speed(double tiles_per_second) noexcept {
    return std::isfinite(tiles_per_second) && tiles_per_second > 0.0;
}

} // namespace

core::Result<World> World::create(std::size_t max_other_entities, const MovementRules& rules) {
    if (!is_usable_speed(rules.speeds.walk_tiles_per_second) ||
        !is_usable_speed(rules.speeds.slide_tiles_per_second)) {
        return core::make_error("walking and sliding need speeds above zero");
    }
    if (rules.paths.max_searched_tiles == 0) {
        return core::make_error("finding a path needs a search limit above zero");
    }
    const bool sends_steps = rules.walk_requests == WalkRequestMode::Steps;
    if (sends_steps &&
        (rules.max_steps_per_request == 0 || rules.max_steps_per_request > WalkSteps::kCapacity)) {
        return core::make_error("a walk request must be able to carry the steps of the route");
    }
    return World(max_other_entities, rules);
}

World::World(std::size_t max_other_entities, const MovementRules& rules)
    : max_other_entities_(max_other_entities), rules_(rules), path_finder_(rules.paths) {
    entities_.reserve(max_other_entities + 1);
}

void World::enter_map(const MapEntered& entered) {
    entities_.clear();
    grid_.reset();
    forget_own_walk();
    map_ = entered.map;
    own_id_ = entered.own_entity;
    entities_.push_back(Entity{entered.own_entity, EntityMotion(entered.own_position), std::nullopt});
}

void World::set_grid(TileGrid grid) {
    path_finder_.prepare_for(grid);
    grid_ = std::move(grid);
}

void World::apply(const GameEvent& event) {
    if (!grid_) {
        ++refused_events_;
        return;
    }

    TileGrid& grid = *grid_;
    if (const auto* appeared = std::get_if<EntityAppeared>(&event)) {
        place(*appeared, grid);
    } else if (const auto* walked = std::get_if<EntityWalked>(&event)) {
        start_walk(*walked, grid);
    } else if (const auto* confirmed = std::get_if<OwnWalkConfirmed>(&event)) {
        confirm_own_walk(*confirmed, grid);
    } else if (const auto* corrected = std::get_if<PositionCorrected>(&event)) {
        correct_position(*corrected, grid);
    } else if (const auto* left = std::get_if<EntityLeft>(&event)) {
        remove(*left);
    } else if (const auto* changed = std::get_if<TilesChanged>(&event)) {
        change_tiles(*changed, grid);
    } else {
        ++refused_events_;
    }
}

void World::update(double seconds) {
    if (!grid_) {
        return;
    }
    const TileGrid& grid = *grid_;
    for (Entity& entity : entities_) {
        advance_entity(entity, seconds, grid);
    }
}

tl::expected<void, PathFailure> World::walk_own_to(TileCoord destination) {
    Entity* own = find_entity(own_id_);
    if (own == nullptr || !grid_) {
        return tl::unexpected(PathFailure::OutsideGrid);
    }

    const TileCoord origin = own->motion.route_start();
    const auto path = path_finder_.find_path(*grid_, origin, destination);
    if (!path) {
        return tl::unexpected(path.error());
    }
    if (path->empty() && !own->motion.is_walking()) {
        own->destination.reset();
        return {};
    }

    own->destination = destination;
    if (rules_.walk_requests == WalkRequestMode::DestinationOnly) {
        own_walk_ = OwnWalk{origin, destination, WalkSteps{}};
        pending_request_ = WalkRequest{origin, destination, WalkSteps{}};
    }
    walk_next_steps(*own, *path);
    return {};
}

std::optional<WalkRequest> World::take_walk_request() noexcept {
    return std::exchange(pending_request_, std::nullopt);
}

const Entity* World::find(EntityId id) const noexcept {
    const auto found = std::ranges::find(entities_, id, &Entity::id);
    return found == entities_.end() ? nullptr : &*found;
}

Entity* World::find_entity(EntityId id) noexcept {
    const auto found = std::ranges::find(entities_, id, &Entity::id);
    return found == entities_.end() ? nullptr : &*found;
}

// A number already in use now names something else, so nothing of the previous entity is kept.
void World::place(const EntityAppeared& appeared, const TileGrid& grid) {
    if (!is_on_map(appeared, grid)) {
        ++refused_events_;
        return;
    }

    const Entity arrival{appeared.entity, EntityMotion(appeared.position), std::nullopt};
    Entity* entity = find_entity(appeared.entity);
    if (entity != nullptr) {
        *entity = arrival;
    } else if (entities_.size() <= max_other_entities_) {
        entity = &entities_.emplace_back(arrival);
    } else {
        ++refused_events_;
        return;
    }

    if (appeared.entity == own_id_) {
        forget_own_walk();
    } else if (appeared.walking_to) {
        entity->destination = appeared.walking_to;
    }
}

// The walks of the player come from the player, and the server answers them with other events.
void World::start_walk(const EntityWalked& walked, const TileGrid& grid) {
    Entity* entity = find_entity(walked.entity);
    if (entity == nullptr) {
        ++unknown_entity_events_;
        return;
    }
    if (walked.entity == own_id_ || !is_on_map(walked, grid) ||
        !is_connected_route(walked.origin.value_or(entity->motion.route_start()), walked.steps.view())) {
        ++refused_events_;
        return;
    }

    if (walked.origin) {
        entity->motion.place_at(*walked.origin);
    }
    entity->destination = walked.destination;
    if (walked.steps.empty()) {
        plan_steps(*entity, walked.destination, grid);
    } else {
        entity->motion.follow(walked.steps.view());
    }
}

void World::correct_position(const PositionCorrected& corrected, const TileGrid& grid) noexcept {
    Entity* entity = find_entity(corrected.entity);
    if (entity == nullptr) {
        ++unknown_entity_events_;
    } else if (!grid.contains(corrected.position)) {
        ++refused_events_;
    } else {
        entity->motion.place_at(corrected.position);
        entity->destination.reset();
        if (corrected.entity == own_id_) {
            forget_own_walk();
        }
    }
}

// The player always exists while there is a map, because the camera and the controls depend on it.
void World::remove(const EntityLeft& left) noexcept {
    if (left.entity == own_id_) {
        ++refused_events_;
        return;
    }
    const auto found = std::ranges::find(entities_, left.entity, &Entity::id);
    if (found == entities_.end()) {
        ++unknown_entity_events_;
    } else {
        entities_.erase(found);
    }
}

void World::change_tiles(const TilesChanged& changed, TileGrid& grid) noexcept {
    const int first_x = std::max(std::min(changed.first.x, changed.last.x), 0);
    const int last_x = std::min(std::max(changed.first.x, changed.last.x), grid.width() - 1);
    const int first_z = std::max(std::min(changed.first.z, changed.last.z), 0);
    const int last_z = std::min(std::max(changed.first.z, changed.last.z), grid.depth() - 1);
    if (first_x > last_x || first_z > last_z) {
        ++refused_events_;
        return;
    }

    for (int z = first_z; z <= last_z; ++z) {
        for (int x = first_x; x <= last_x; ++x) {
            static_cast<void>(grid.set_walkable({x, z}, changed.walkable));
        }
    }
}

void World::confirm_own_walk(const OwnWalkConfirmed& confirmed, const TileGrid& grid) {
    if (!answers_current_walk(confirmed) || !own_walk_) {
        ++stale_confirmations_;
        return;
    }
    if (!grid.contains(confirmed.destination)) {
        ++refused_events_;
        return;
    }

    OwnWalk& walk = *own_walk_;
    Entity* own = find_entity(own_id_);
    if (own != nullptr && confirmed.destination != walk.end) {
        walk.end = confirmed.destination;
        adopt_shorter_walk(*own, confirmed.destination, grid);
    }
}

// An answer belongs to the current walk when it starts at the same tile and ends somewhere the walk can end.
bool World::answers_current_walk(const OwnWalkConfirmed& confirmed) const noexcept {
    if (!own_walk_ || confirmed.origin != own_walk_->origin) {
        return false;
    }
    if (rules_.walk_requests == WalkRequestMode::DestinationOnly ||
        confirmed.destination == confirmed.origin) {
        return true;
    }
    const auto steps = own_walk_->steps.view();
    return std::ranges::find(steps, confirmed.destination) != steps.end();
}

// The server accepted less than was asked, which is not an error: the player simply stops earlier, even if it
// has to turn back.
void World::adopt_shorter_walk(Entity& own, TileCoord accepted, const TileGrid& grid) {
    own.destination = accepted;
    const auto path = path_finder_.find_path(grid, own.motion.route_start(), accepted);
    if (path) {
        own.motion.follow(*path);
    } else {
        own.motion.place_at(accepted);
    }
}

void World::forget_own_walk() noexcept {
    own_walk_.reset();
    pending_request_.reset();
}

// A walk can go on in the same frame, so the distance is the same at any frame rate.
void World::advance_entity(Entity& entity, double seconds, const TileGrid& grid) {
    double time_left = entity.motion.advance(seconds, rules_.speeds);
    while (time_left > 0.0 && continue_walk(entity, grid)) {
        time_left = entity.motion.advance(time_left, rules_.speeds);
    }
}

bool World::continue_walk(Entity& entity, const TileGrid& grid) {
    if (!entity.destination) {
        return false;
    }
    const TileCoord destination = *entity.destination;
    if (entity.tile() == destination) {
        entity.destination.reset();
        return false;
    }
    plan_steps(entity, destination, grid);
    return entity.motion.is_walking();
}

// Without a way on the client's grid, the player stops and waits for the server. Anyone else is walking by
// decision of the server, so it goes to the destination anyway.
void World::plan_steps(Entity& entity, TileCoord destination, const TileGrid& grid) {
    const auto path = path_finder_.find_path(grid, entity.motion.route_start(), destination);
    if (path) {
        walk_next_steps(entity, *path);
        return;
    }
    if (entity.id != own_id_) {
        entity.motion.place_at(destination);
    }
    entity.destination.reset();
}

void World::walk_next_steps(Entity& entity, std::span<const TileCoord> path) {
    const TileCoord origin = entity.motion.route_start();
    const auto steps = path.first(std::min(path.size(), route_step_limit()));

    entity.motion.follow(steps);
    if (entity.id == own_id_ && rules_.walk_requests == WalkRequestMode::Steps) {
        announce_own_steps(origin, steps);
    }
}

void World::announce_own_steps(TileCoord origin, std::span<const TileCoord> steps) noexcept {
    WalkSteps sent;
    for (const TileCoord step : steps) {
        static_cast<void>(sent.add(step));
    }
    const TileCoord end = steps.empty() ? origin : steps.back();
    own_walk_ = OwnWalk{origin, end, sent};
    pending_request_ = WalkRequest{origin, end, sent};
}

std::size_t World::route_step_limit() const noexcept {
    return rules_.walk_requests == WalkRequestMode::Steps ? rules_.max_steps_per_request
                                                          : WalkSteps::kCapacity;
}

} // namespace lemuria::classic
