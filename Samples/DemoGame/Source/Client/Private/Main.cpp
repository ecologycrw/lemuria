#include <iostream>

#include "DemoProtocol/Protocol.hpp"
#include "Lemuria/Classic/FrameworkInfo.hpp"
#include "Lemuria/Core/Version.hpp"
#include "Lemuria/Platform/PlatformInfo.hpp"

namespace {

void print_version(const lemuria::core::Version& version) {
    std::cout << version.major << '.' << version.minor << '.' << version.patch;
}

} // namespace

int main() {
    std::cout << "Lemuria Demo: engine ";
    print_version(lemuria::core::engine_version());
    std::cout << ", framework " << lemuria::classic::framework_name() << ", SDL ";
    print_version(lemuria::platform::sdl_version());
    std::cout << ", protocol " << demo_protocol::protocol_version() << '\n';
    return 0;
}
