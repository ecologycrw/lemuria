#include <iostream>

#include "DemoProtocol/Protocol.hpp"

int main() {
    std::cout << "Lemuria test server: protocol " << demo_protocol::protocol_version() << '\n';
    return 0;
}
