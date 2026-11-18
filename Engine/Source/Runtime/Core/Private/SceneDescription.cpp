#include "Lemuria/Core/SceneDescription.hpp"

namespace lemuria::core {

ShapeList::ShapeList(std::size_t capacity) : storage_(capacity) {}

bool ShapeList::add(const ShapeInstance& shape) noexcept {
    if (count_ == storage_.size()) {
        ++dropped_count_;
        return false;
    }
    storage_[count_] = shape;
    ++count_;
    return true;
}

void ShapeList::clear() noexcept {
    count_ = 0;
    dropped_count_ = 0;
}

std::span<const ShapeInstance> ShapeList::shapes() const noexcept {
    return std::span<const ShapeInstance>(storage_).first(count_);
}

SceneDescription::SceneDescription(std::size_t ground_tile_capacity, std::size_t box_capacity)
    : ground_tiles(ground_tile_capacity), boxes(box_capacity) {}

void SceneDescription::clear() noexcept {
    ground_tiles.clear();
    boxes.clear();
}

} // namespace lemuria::core
