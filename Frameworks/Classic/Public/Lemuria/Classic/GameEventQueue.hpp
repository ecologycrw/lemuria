#pragma once

#include <cstddef>
#include <vector>

#include "Lemuria/Classic/GameEvent.hpp"

namespace lemuria::classic {

// A line of events with a fixed size, which never allocates memory after it is created (INV-13).
class GameEventQueue {
public:
    explicit GameEventQueue(std::size_t capacity = 0) : events_(capacity) {}

    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }

    // False, with nothing changed, when the queue is full.
    [[nodiscard]] bool push(const GameEvent& event) noexcept {
        if (count_ == events_.size()) {
            return false;
        }
        events_[(first_ + count_) % events_.size()] = event;
        ++count_;
        return true;
    }

    // The oldest event. The queue must not be empty.
    [[nodiscard]] const GameEvent& front() const noexcept { return events_[first_]; }

    // Removes the oldest event, if there is one.
    void pop() noexcept {
        if (count_ > 0) {
            first_ = (first_ + 1) % events_.size();
            --count_;
        }
    }

    void clear() noexcept {
        first_ = 0;
        count_ = 0;
    }

private:
    std::vector<GameEvent> events_;
    std::size_t first_ = 0;
    std::size_t count_ = 0;
};

} // namespace lemuria::classic
