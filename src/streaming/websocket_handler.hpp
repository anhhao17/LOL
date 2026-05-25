#pragma once

#include "streaming/stream_manager.hpp"
#include "session/session_store.hpp"
#include "common/error.hpp"
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <memory>
#include <optional>
#include <string>

namespace streaming {

struct WsUpgradeResult {
    common::ErrorCode code{common::ErrorCode::kOk};
    std::optional<ViewType> viewType;
    std::string sessionToken;

    static WsUpgradeResult fail(common::ErrorCode c) noexcept {
        WsUpgradeResult r;
        r.code = c;
        return r;
    }

    static WsUpgradeResult success(ViewType vt, std::string token) {
        WsUpgradeResult r;
        r.code         = common::ErrorCode::kOk;
        r.viewType     = vt;
        r.sessionToken = std::move(token);
        return r;
    }
};

// Stateless validation logic for the WebSocket upgrade request.
class WebSocketHandler {
public:
    WebSocketHandler(std::shared_ptr<StreamManager> streamManager,
                     std::shared_ptr<session::SessionStore> sessionStore,
                     size_t maxSessions);

    WebSocketHandler(const WebSocketHandler&) = delete;
    WebSocketHandler& operator=(const WebSocketHandler&) = delete;

    // Validates the upgrade request and returns result.
    // Call this before accepting the WS handshake.
    [[nodiscard]] WsUpgradeResult validateUpgrade(
        const boost::beast::http::request<boost::beast::http::string_body>& req,
        std::string_view clientIp) const;

    [[nodiscard]] std::shared_ptr<StreamManager> streamManager() const noexcept {
        return streamManager_;
    }

private:
    std::shared_ptr<StreamManager> streamManager_;
    std::shared_ptr<session::SessionStore> sessionStore_;
    size_t maxSessions_;

    [[nodiscard]] static std::pair<std::string, std::string>
        parseProtocolHeader(std::string_view header);

    [[nodiscard]] std::string extractToken(
        const boost::beast::http::request<boost::beast::http::string_body>& req,
        std::string_view protocolToken) const;
};

} // namespace streaming
