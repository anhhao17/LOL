#include "streaming/stream_manager.hpp"
#include "common/logger.hpp"
#include <boost/beast/core/error.hpp>

namespace streaming {

static constexpr auto kLog = "stream";

std::optional<ViewType> StreamManager::parseViewType(std::string_view s) noexcept {
    if (s == "cl_view")        return ViewType::kColorMain;
    if (s == "ir_view")        return ViewType::kIrMain;
    if (s == "both_views")     return ViewType::kBothMain;
    if (s == "cl_sub_view")    return ViewType::kColorSub;
    if (s == "ir_sub_view")    return ViewType::kIrSub;
    if (s == "both_sub_views") return ViewType::kBothSub;
    return std::nullopt;
}

void StreamManager::setSource(SourceChannel channel, std::unique_ptr<IFrameSource> source) {
    std::lock_guard<std::mutex> lock(sourceMutex_);
    auto& entry = sources_[channel];
    if (entry.running && entry.source) {
        entry.source->stop();
        entry.running = false;
    }
    entry.source = std::move(source);
}

std::set<SourceChannel> StreamManager::neededChannels() const {
    // Call under mutex_.
    std::set<SourceChannel> needed;
    for (auto& [conn, info] : clients_) {
        switch (info.viewType) {
            case ViewType::kColorMain:
            case ViewType::kColorSub:
                needed.insert(SourceChannel::CL);
                break;
            case ViewType::kIrMain:
            case ViewType::kIrSub:
                needed.insert(SourceChannel::IR);
                break;
            case ViewType::kBothMain:
            case ViewType::kBothSub:
                needed.insert(SourceChannel::CL);
                needed.insert(SourceChannel::IR);
                break;
        }
    }
    return needed;
}

void StreamManager::syncSources(const std::set<SourceChannel>& needed) {
    // Call under sourceMutex_, not mutex_.
    for (auto& [ch, entry] : sources_) {
        if (!entry.source) continue;
        const bool want = needed.count(ch) > 0;
        if (want && !entry.running) {
            entry.running = true;
            SourceChannel channel = ch;
            entry.source->start([this, channel](const std::vector<uint8_t>& frame) {
                pushFrame(channel, frame);
            });
            LOG_INFO(common::Logger::get(kLog),
                "Source started: channel={}", static_cast<int>(ch));
        } else if (!want && entry.running) {
            entry.running = false;
            entry.source->stop();
            LOG_INFO(common::Logger::get(kLog),
                "Source stopped: channel={}", static_cast<int>(ch));
        }
    }
}

bool StreamManager::add(WsStream* conn, ViewType viewType, std::string sessionId) {
    std::set<SourceChannel> needed;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        clients_[conn] = ClientInfo{viewType, std::move(sessionId)};
        LOG_INFO(common::Logger::get(kLog), "WS client added: total={}", clients_.size());
        needed = neededChannels();
    }
    {
        std::lock_guard<std::mutex> lock(sourceMutex_);
        syncSources(needed);
    }
    return true;
}

void StreamManager::remove(WsStream* conn) noexcept {
    std::set<SourceChannel> needed;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        clients_.erase(conn);
        LOG_INFO(common::Logger::get(kLog), "WS client removed: total={}", clients_.size());
        needed = neededChannels();
    }
    {
        std::lock_guard<std::mutex> lock(sourceMutex_);
        syncSources(needed);
    }
}

void StreamManager::pushFrame(SourceChannel channel, const std::vector<uint8_t>& jpeg) {
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
            LOG_WARN(common::Logger::get(kLog), "Frame send failed: {}", ec.message());
        }
    }
}

size_t StreamManager::clientCount() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return clients_.size();
}

} // namespace streaming
