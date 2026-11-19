#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

#include "Lemuria/Core/Result.hpp"

namespace lemuria::platform {

// One connection accepted by a `TcpListener`. None of its functions waits for the network.
class TcpPeer {
public:
    TcpPeer(TcpPeer&& other) noexcept;
    TcpPeer& operator=(TcpPeer&& other) noexcept;
    TcpPeer(const TcpPeer&) = delete;
    TcpPeer& operator=(const TcpPeer&) = delete;
    ~TcpPeer();

    // Copies what has already arrived and returns how many bytes it copied, which can be zero. It fails when
    // the connection has ended.
    [[nodiscard]] core::Result<std::size_t> read(std::span<std::byte> buffer);

    // Bytes that cannot leave yet are kept, and leave during the next calls of `read` and `write`.
    [[nodiscard]] core::Result<void> write(std::span<const std::byte> bytes);

private:
    friend class TcpListener;
    struct Socket;

    explicit TcpPeer(std::unique_ptr<Socket> socket) noexcept;

    std::unique_ptr<Socket> socket_;
};

// Accepts TCP connections that come from this computer only. It serves the test server and the tests.
class TcpListener {
public:
    // Takes the first free port among `port_count` ports in a row, starting at `first_port`.
    [[nodiscard]] static core::Result<TcpListener> open_on_this_computer(std::uint16_t first_port,
                                                                         std::uint16_t port_count = 1);

    TcpListener(TcpListener&& other) noexcept;
    TcpListener& operator=(TcpListener&& other) noexcept;
    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(const TcpListener&) = delete;
    ~TcpListener();

    [[nodiscard]] std::uint16_t port() const noexcept { return port_; }

    // Returns the next connection that is waiting, if there is one.
    [[nodiscard]] core::Result<std::optional<TcpPeer>> accept();

private:
    struct Server;

    TcpListener(std::unique_ptr<Server> server, std::uint16_t port) noexcept;

    std::unique_ptr<Server> server_;
    std::uint16_t port_ = 0;
};

} // namespace lemuria::platform
