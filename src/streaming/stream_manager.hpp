#pragma once

#include "streaming/i_frame_source.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
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

    // Register a frame source for a channel. Sources are started on-demand
    // when the first client connects and stopped when the last one leaves.
    void setSource(SourceChannel channel, std::unique_ptr<IFrameSource> source);

    bool add(WsStream* conn, ViewType viewType, std::string sessionId);
    void remove(WsStream* conn) noexcept;

    // Called by frame sources: prepends the 1-byte channel tag and broadcasts.
    void pushFrame(SourceChannel channel, const std::vector<uint8_t>& jpeg);

    // Broadcast binary frame to all clients subscribed to the given view type.
    void broadcast(ViewType viewType, const std::vector<uint8_t>& frame);

    [[nodiscard]] size_t clientCount() const noexcept;

private:
    void startSources();
    void stopSources();

    struct ClientInfo {
        ViewType viewType;
        std::string sessionId;
    };

    std::map<WsStream*, ClientInfo>          clients_;
    std::map<SourceChannel, std::unique_ptr<IFrameSource>> sources_;
    bool                                     sourcesRunning_{false};
    mutable std::mutex                       mutex_;
};

} // namespace streaming
