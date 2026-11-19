#include "TestServer/ServerHost.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "DemoProtocol/DemoMap.hpp"
#include "DemoProtocol/DemoRules.hpp"
#include "DemoProtocol/Framing.hpp"
#include "DemoProtocol/MessageCodec.hpp"

namespace test_server {

namespace {

using demo_protocol::DemoMap;
using lemuria::platform::TcpListener;
using lemuria::platform::TcpPeer;

// Bytes are read in pieces of this size, and at most this many pieces per client in each round, so a client
// that sends a lot cannot hold the others back.
constexpr std::size_t kReadPieceBytes = 512;
constexpr int kMaxReadsPerRound = 32;

// The rest of the connections that arrived together wait for the next round.
constexpr int kMaxAcceptsPerRound = 16;

constexpr std::uint16_t kDemoSpawnCenter = DemoMap::kSpawnCenter;

} // namespace

ServerConfig demo_server_config() noexcept {
    return ServerConfig{
        .map_id = DemoMap::kId,
        .walk_tiles_per_second = demo_protocol::kWalkTilesPerSecond,
        // A client that clicks while walking starts the new walk from the tile it is walking to, up to one
        // diagonal step (0.35 seconds) before it gets there. Half a second also covers the delay of a
        // connection on this computer.
        .walks = {.origin_tolerance_steps = 1, .early_tolerance_seconds = 0.5},
        .max_players = 8,
        .spawn_center = Tile{kDemoSpawnCenter, kDemoSpawnCenter},
    };
}

HostLimits demo_host_limits() noexcept {
    // Twice the players, so a client the rules send away because the map is full is not refused before it
    // can say hello.
    return HostLimits{.max_connections = 16};
}

struct ServerHost::Client {
    ClientId id;
    TcpPeer peer;

    // The frame that has not arrived whole yet.
    demo_protocol::FrameReader frames;

    // The connection ends at the end of the round, and nothing else it sent is handled.
    bool is_ending = false;
};

// The clients that are connected. The rules talk to them through the `ServerOutbox`.
class ServerHost::Clients final : public ServerOutbox {
public:
    explicit Clients(std::size_t capacity) : capacity_(capacity) { clients_.reserve(capacity); }

    [[nodiscard]] bool is_full() const noexcept { return clients_.size() >= capacity_; }
    [[nodiscard]] std::size_t size() const noexcept { return clients_.size(); }
    [[nodiscard]] std::span<Client> all() noexcept { return clients_; }

    void add(TcpPeer peer) {
        ++last_number_;
        clients_.push_back(Client{ClientId{last_number_}, std::move(peer), {}, false});
    }

    // Closes the connection of one client that must end and returns who it was.
    [[nodiscard]] std::optional<ClientId> take_ending() {
        const auto found = std::ranges::find(clients_, true, &Client::is_ending);
        if (found == clients_.end()) {
            return std::nullopt;
        }
        const ClientId ended = found->id;
        clients_.erase(found);
        return ended;
    }

    void send(ClientId id, const demo_protocol::ServerMessage& message) override {
        Client* client = find(id);
        if (client == nullptr || client->is_ending) {
            return;
        }
        const demo_protocol::EncodedFrame frame = demo_protocol::encode(message);
        if (!client->peer.write(frame.bytes())) {
            client->is_ending = true;
        }
    }

    void disconnect(ClientId id) override {
        if (Client* client = find(id)) {
            client->is_ending = true;
        }
    }

    void end_broken(Client& client) noexcept {
        client.is_ending = true;
        ++broken_connections_;
    }

    void count_invalid_message() noexcept { ++invalid_messages_; }

    [[nodiscard]] std::uint64_t broken_connection_count() const noexcept { return broken_connections_; }
    [[nodiscard]] std::uint64_t invalid_message_count() const noexcept { return invalid_messages_; }

private:
    [[nodiscard]] Client* find(ClientId id) noexcept {
        const auto found = std::ranges::find(clients_, id, &Client::id);
        return found == clients_.end() ? nullptr : &*found;
    }

    std::size_t capacity_ = 0;
    std::vector<Client> clients_;
    std::uint64_t last_number_ = 0;
    std::uint64_t broken_connections_ = 0;
    std::uint64_t invalid_messages_ = 0;
};

// Turns the frames of one client into messages for the rules. A wrong message is left out, and the next ones
// are still read (CA-11).
class ServerHost::ClientReader final : public demo_protocol::FrameSink {
public:
    ClientReader(ServerWorld& world, Clients& clients, Client& client, double now) noexcept
        : world_(world), clients_(clients), client_(client), now_(now) {}

    void on_frame(const demo_protocol::Frame& frame) override {
        if (client_.is_ending) {
            return;
        }
        const auto message = demo_protocol::read_client_message(frame);
        if (!message) {
            clients_.count_invalid_message();
            return;
        }
        world_.receive(client_.id, *message, now_);
    }

private:
    ServerWorld& world_;
    Clients& clients_;
    Client& client_;
    double now_ = 0.0;
};

lemuria::core::Result<ServerHost> ServerHost::create(TcpListener listener, ServerGrid grid,
                                                     const ServerConfig& config, const HostLimits& limits) {
    if (limits.max_connections == 0) {
        return lemuria::core::make_error("the server needs room for at least one connection");
    }
    auto clients = std::make_unique<Clients>(limits.max_connections);
    auto world = ServerWorld::create(std::move(grid), config, *clients);
    if (!world) {
        return tl::unexpected(std::move(world.error()));
    }
    return ServerHost(std::move(listener), std::move(clients), std::move(*world));
}

ServerHost::ServerHost(TcpListener listener, std::unique_ptr<Clients> clients, ServerWorld world) noexcept
    : listener_(std::move(listener)), clients_(std::move(clients)), world_(std::move(world)) {}

ServerHost::ServerHost(ServerHost&& other) noexcept = default;
ServerHost::~ServerHost() = default;

lemuria::core::Result<void> ServerHost::serve(double now) {
    if (auto accepted = accept_connections(); !accepted) {
        return accepted;
    }
    read_messages(now);
    end_connections();
    return {};
}

std::size_t ServerHost::connection_count() const noexcept {
    return clients_->size();
}

std::uint64_t ServerHost::broken_connection_count() const noexcept {
    return clients_->broken_connection_count();
}

std::uint64_t ServerHost::invalid_message_count() const noexcept {
    return clients_->invalid_message_count();
}

// A connection beyond the limit is accepted only to be closed at once, so it does not wait in vain.
lemuria::core::Result<void> ServerHost::accept_connections() {
    for (int accept_count = 0; accept_count < kMaxAcceptsPerRound; ++accept_count) {
        auto accepted = listener_.accept();
        if (!accepted) {
            return tl::unexpected(std::move(accepted.error()));
        }
        if (!accepted->has_value()) {
            return {};
        }
        if (clients_->is_full()) {
            ++refused_connections_;
        } else {
            clients_->add(std::move(**accepted));
        }
    }
    return {};
}

void ServerHost::read_messages(double now) {
    for (Client& client : clients_->all()) {
        read_from(client, now);
    }
}

void ServerHost::read_from(Client& client, double now) {
    ClientReader reader(world_, *clients_, client, now);
    std::array<std::byte, kReadPieceBytes> piece{};
    for (int read_count = 0; read_count < kMaxReadsPerRound && !client.is_ending; ++read_count) {
        const auto received = client.peer.read(piece);
        if (!received) {
            // The client left.
            client.is_ending = true;
            return;
        }
        if (*received == 0) {
            return;
        }
        if (!client.frames.read(std::span(piece).first(*received), reader)) {
            clients_->end_broken(client);
            return;
        }
    }
}

// Ending one connection can make the rules write to the others, which may end them too.
void ServerHost::end_connections() {
    while (const std::optional<ClientId> ended = clients_->take_ending()) {
        world_.remove(*ended);
    }
}

} // namespace test_server
