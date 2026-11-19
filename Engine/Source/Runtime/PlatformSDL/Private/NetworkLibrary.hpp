#pragma once

#include <memory>

#include <SDL3_net/SDL_net.h>

namespace lemuria::platform {

// Keeps the SDL_net library open while it exists.
class NetworkLibrary {
public:
    NetworkLibrary() : ready_(NET_Init()) {}
    ~NetworkLibrary() {
        if (ready_) {
            NET_Quit();
        }
    }
    NetworkLibrary(const NetworkLibrary&) = delete;
    NetworkLibrary& operator=(const NetworkLibrary&) = delete;
    NetworkLibrary(NetworkLibrary&&) = delete;
    NetworkLibrary& operator=(NetworkLibrary&&) = delete;

    [[nodiscard]] bool ready() const noexcept { return ready_; }

private:
    bool ready_;
};

struct NetAddressRelease {
    void operator()(NET_Address* address) const noexcept { NET_UnrefAddress(address); }
};

struct StreamSocketRelease {
    void operator()(NET_StreamSocket* socket) const noexcept { NET_DestroyStreamSocket(socket); }
};

struct NetServerRelease {
    void operator()(NET_Server* server) const noexcept { NET_DestroyServer(server); }
};

using NetAddressHandle = std::unique_ptr<NET_Address, NetAddressRelease>;
using StreamSocketHandle = std::unique_ptr<NET_StreamSocket, StreamSocketRelease>;
using NetServerHandle = std::unique_ptr<NET_Server, NetServerRelease>;

} // namespace lemuria::platform
