#include "streaming/stream_manager.hpp"
#include "common/logger.hpp"
#include <boost/beast/core/error.hpp>

namespace streaming {

static constexpr auto kLog = "stream";

std::optional<ViewType> StreamManager::parseViewType(std::string_view s) noexcept {
    if (s == "cl_view")       return ViewType::kColorMain;
    if (s == "ir_view")       return ViewType::kIrMain;
    if (s == "both_views")    return ViewType::kBothMain;
    if (s == "cl_sub_view")   return ViewType::kColorSub;
    if (s == "ir_sub_view")   return ViewType::kIrSub;
    if (s == "both_sub_views") return ViewType::kBothSub;
    return std::nullopt;
}

bool StreamManager::add(WsStream* conn, ViewType viewType, std::string sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_[conn] = ClientInfo{viewType, std::move(sessionId)};
    LOG_INFO(common::Logger::get(kLog),
        "WS client added: total={}", clients_.size());
    return true;
}

void StreamManager::remove(WsStream* conn) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_.erase(conn);
    LOG_INFO(common::Logger::get(kLog),
        "WS client removed: total={}", clients_.size());
}

void StreamManager::pushFrame(SourceChannel channel, const std::vector<uint8_t>& jpeg) {
    // Build message: [1-byte channel tag][jpeg data]
    std::vector<uint8_t> msg;
    msg.reserve(1 + jpeg.size());
    msg.push_back(static_cast<uint8_t>(channel));
    msg.insert(msg.end(), jpeg.begin(), jpeg.end());

    ViewType vt = (channel == SourceChannel::CL) ? ViewType::kColorMain : ViewType::kIrMain;
    broadcast(vt, msg);
}

void StreamManager::broadcast(ViewType viewType, const std::vector<uint8_t>& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    boost::beast::error_code ec;

    for (auto& [conn, info] : clients_) {
        bool matches = (info.viewType == viewType)
            || (viewType == ViewType::kColorMain && info.viewType == ViewType::kBothMain)
            || (viewType == ViewType::kIrMain    && info.viewType == ViewType::kBothMain)
            || (viewType == ViewType::kColorSub  && info.viewType == ViewType::kBothSub)
            || (viewType == ViewType::kIrSub     && info.viewType == ViewType::kBothSub);

        if (!matches) continue;

        conn->write(boost::asio::buffer(frame), ec);
        if (ec) {
            LOG_WARN(common::Logger::get(kLog),
                "Frame send failed: {}", ec.message());
        }
    }
}

size_t StreamManager::clientCount() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return clients_.size();
}

} // namespace streaming
