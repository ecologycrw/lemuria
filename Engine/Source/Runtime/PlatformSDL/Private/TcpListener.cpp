#include "Lemuria/Platform/TcpListener.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "NetworkLibrary.hpp"

namespace lemuria::platform {

namespace {

constexpr const char* kThisComputer = "127.0.0.1";
constexpr int kResolveTimeoutMilliseconds = 5'000;

[[nodiscard]] tl::unexpected<core::Error> network_error(const std::string& what) {
    return core::make_error(what + ": " + SDL_GetError());
}

// Without this, two programs could share the same port without noticing.
[[nodiscard]] NetServerHandle listen_alone_on(NET_Address* address, std::uint16_t port) {
    const SDL_PropertiesID properties = SDL_CreateProperties();
    SDL_SetBooleanProperty(properties, NET_PROP_SERVER_REUSEADDR_BOOLEAN, false);
    NetServerHandle server(NET_CreateServer(address, port, properties));
    SDL_DestroyProperties(properties);
    return server;
}

} // namespace

struct TcpPeer::Socket {
    NetworkLibrary library;
    StreamSocketHandle handle;
};

struct TcpListener::Server {
    NetworkLibrary library;
    NetServerHandle handle;
};

TcpPeer::TcpPeer(std::unique_ptr<Socket> socket) noexcept : socket_(std::move(socket)) {}
TcpPeer::TcpPeer(TcpPeer&& other) noexcept = default;
TcpPeer& TcpPeer::operator=(TcpPeer&& other) noexcept = default;
TcpPeer::~TcpPeer() = default;

core::Result<std::size_t> TcpPeer::read(std::span<std::byte> buffer) {
    const std::size_t wanted =
        std::min(buffer.size(), static_cast<std::size_t>(std::numeric_limits<int>::max()));
    const int count =
        NET_ReadFromStreamSocket(socket_->handle.get(), buffer.data(), static_cast<int>(wanted));
    if (count < 0) {
        return network_error("the connection has ended");
    }
    return static_cast<std::size_t>(count);
}

core::Result<void> TcpPeer::write(std::span<const std::byte> bytes) {
    if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return core::make_error("too many bytes to write at once");
    }
    if (!NET_WriteToStreamSocket(socket_->handle.get(), bytes.data(), static_cast<int>(bytes.size()))) {
        return network_error("the connection has ended");
    }
    return {};
}

TcpListener::TcpListener(std::unique_ptr<Server> server, std::uint16_t port) noexcept
    : server_(std::move(server)), port_(port) {}
TcpListener::TcpListener(TcpListener&& other) noexcept = default;
TcpListener& TcpListener::operator=(TcpListener&& other) noexcept = default;
TcpListener::~TcpListener() = default;

core::Result<TcpListener> TcpListener::open_on_this_computer(std::uint16_t first_port,
                                                             std::uint16_t port_count) {
    if (first_port == 0 || port_count == 0) {
        return core::make_error("a listener needs at least one port to try");
    }
    auto server = std::make_unique<Server>();
    if (!server->library.ready()) {
        return network_error("could not start the network");
    }
    const NetAddressHandle address(NET_ResolveHostname(kThisComputer));
    if (!address || NET_WaitUntilResolved(address.get(), kResolveTimeoutMilliseconds) != NET_SUCCESS) {
        return network_error("could not find the address of this computer");
    }
    const std::uint32_t end_port = std::min<std::uint32_t>(std::uint32_t{first_port} + port_count, 65'536);
    for (std::uint32_t port = first_port; port < end_port; ++port) {
        server->handle = listen_alone_on(address.get(), static_cast<std::uint16_t>(port));
        if (server->handle) {
            return TcpListener(std::move(server), static_cast<std::uint16_t>(port));
        }
    }
    return network_error("no free port to listen on");
}

core::Result<std::optional<TcpPeer>> TcpListener::accept() {
    NET_StreamSocket* accepted = nullptr;
    if (!NET_AcceptClient(server_->handle.get(), &accepted)) {
        return network_error("could not accept a connection");
    }
    if (accepted == nullptr) {
        return std::optional<TcpPeer>{};
    }
    auto socket = std::make_unique<TcpPeer::Socket>();
    socket->handle.reset(accepted);
    return std::optional<TcpPeer>(TcpPeer(std::move(socket)));
}

} // namespace lemuria::platform
