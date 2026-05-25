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
// Sources are started on-demand per channel: a CL-only client won't start the IR source.
class StreamManager {
public:
    StreamManager() = default;
    StreamManager(const StreamManager&) = delete;
    StreamManager& operator=(const StreamManager&) = delete;

    [[nodiscard]] static std::optional<ViewType> parseViewType(std::string_view s) noexcept;

    void setSource(SourceChannel channel, std::unique_ptr<IFrameSource> source);

    bool add(WsStream* conn, ViewType viewType, std::string sessionId);
    void remove(WsStream* conn) noexcept;

    void pushFrame(SourceChannel channel, const std::vector<uint8_t>& jpeg);
    void broadcast(ViewType viewType, const std::vector<uint8_t>& frame);

    [[nodiscard]] size_t clientCount() const noexcept;

private:
    // Returns which channels are needed by the current clients_. Call under mutex_.
    std::set<SourceChannel> neededChannels() const;

    // Start/stop sources to match the needed set. Call under sourceMutex_, NOT mutex_.
    void syncSources(const std::set<SourceChannel>& needed);

    struct ClientInfo {
        ViewType    viewType;
        std::string sessionId;
    };

    struct SourceEntry {
        std::unique_ptr<IFrameSource> source;
        bool running{false};
    };

    std::map<WsStream*, ClientInfo>      clients_;
    std::map<SourceChannel, SourceEntry> sources_;
    mutable std::mutex mutex_;        // protects clients_
    std::mutex         sourceMutex_;  // serialises source start/stop lifecycle
};

} // namespace streaming
