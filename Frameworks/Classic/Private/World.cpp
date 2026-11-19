#include "Lemuria/Classic/World.hpp"

#include <algorithm>
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

} // namespace

World::World(std::size_t max_other_entities) : max_other_entities_(max_other_entities) {
    entities_.reserve(max_other_entities + 1);
}

void World::enter_map(const MapEntered& entered) {
    entities_.clear();
    grid_.reset();
    map_ = entered.map;
    own_id_ = entered.own_entity;
    entities_.push_back(Entity{entered.own_entity, entered.own_position, std::nullopt});
}

void World::set_grid(TileGrid grid) noexcept {
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

    const Entity entity{appeared.entity, appeared.position, appeared.walking_to};
    if (Entity* existing = find_entity(appeared.entity)) {
        *existing = entity;
    } else if (entities_.size() <= max_other_entities_) {
        entities_.push_back(entity);
    } else {
        ++refused_events_;
    }
}

void World::start_walk(const EntityWalked& walked, const TileGrid& grid) noexcept {
    Entity* entity = find_entity(walked.entity);
    if (entity == nullptr) {
        ++unknown_entity_events_;
    } else if (!is_on_map(walked, grid)) {
        ++refused_events_;
    } else {
        entity->tile = walked.origin.value_or(entity->tile);
        entity->destination = walked.destination;
    }
}

void World::correct_position(const PositionCorrected& corrected, const TileGrid& grid) noexcept {
    Entity* entity = find_entity(corrected.entity);
    if (entity == nullptr) {
        ++unknown_entity_events_;
    } else if (!grid.contains(corrected.position)) {
        ++refused_events_;
    } else {
        entity->tile = corrected.position;
        entity->destination.reset();
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

} // namespace lemuria::classic
