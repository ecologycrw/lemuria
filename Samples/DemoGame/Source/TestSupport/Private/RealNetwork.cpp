#include "DemoTestSupport/RealNetwork.hpp"

#include <chrono>
#include <thread>

#include "Lemuria/Platform/MonotonicClock.hpp"

namespace demo_test_support {

double real_seconds() noexcept {
    return static_cast<double>(lemuria::platform::monotonic_nanoseconds()) / 1'000'000'000.0;
}

void pause_briefly() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

} // namespace demo_test_support
