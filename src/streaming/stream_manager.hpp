#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace streaming {

enum class ViewType : uint32_t {
    kColorMain    = 0,
    kIrMain       = 1,
    kBothMain     = 2,
    kColorSub     = 3,
    kIrSub        = 4,
    kBothSub      = 5,
};

using WsStream = boost::beast::websocket::stream<boost::beast::tcp_stream>;

// Tracks connected WebSocket streaming clients and broadcasts frames to them.
class StreamManager {
public:
    StreamManager() = default;
    StreamManager(const StreamManager&) = delete;
    StreamManager& operator=(const StreamManager&) = delete;

    [[nodiscard]] static std::optional<ViewType> parseViewType(std::string_view s) noexcept;

    bool add(WsStream* conn, ViewType viewType, std::string sessionId);
    void remove(WsStream* conn) noexcept;

    // Broadcast binary frame to all clients subscribed to the given view type.
    void broadcast(ViewType viewType, const std::vector<uint8_t>& frame);

    [[nodiscard]] size_t clientCount() const noexcept;

private:
    struct ClientInfo {
        ViewType viewType;
        std::string sessionId;
    };

    std::map<WsStream*, ClientInfo> clients_;
    mutable std::mutex mutex_;
};

} // namespace streaming
