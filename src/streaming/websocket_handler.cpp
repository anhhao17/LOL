#include "streaming/websocket_handler.hpp"
#include "common/logger.hpp"
#include <boost/beast/http/field.hpp>
#include <sstream>

namespace streaming {

static constexpr auto kLog = "websocket";

WebSocketHandler::WebSocketHandler(std::shared_ptr<StreamManager> streamManager,
                                   std::shared_ptr<session::SessionStore> sessionStore,
                                   size_t maxSessions)
    : streamManager_(std::move(streamManager))
    , sessionStore_(std::move(sessionStore))
    , maxSessions_(maxSessions) {}

// Parses "view=cl_view, token=abc123" into {viewStr, tokenStr}.
std::pair<std::string, std::string>
WebSocketHandler::parseProtocolHeader(std::string_view header) {
    std::string viewStr, tokenStr;
    std::istringstream ss{std::string(header)};
    std::string item;
    while (std::getline(ss, item, ',')) {
        auto first = item.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        item = item.substr(first);

        if (item.rfind("view=", 0) == 0) {
            viewStr = item.substr(5);
        } else if (item.rfind("token=", 0) == 0) {
            tokenStr = item.substr(6);
        }
    }
    return {viewStr, tokenStr};
}

// Parses "?view=cl_view" (or "&view=cl_view") from a URL target.
std::string WebSocketHandler::parseViewFromTarget(std::string_view target) {
    auto qpos = target.find('?');
    if (qpos == std::string_view::npos) return {};

    auto query = target.substr(qpos + 1);
    std::istringstream ss{std::string(query)};
    std::string param;
    while (std::getline(ss, param, '&')) {
        if (param.rfind("view=", 0) == 0) {
            return param.substr(5);
        }
    }
    return {};
}

std::string WebSocketHandler::extractToken(
    const boost::beast::http::request<boost::beast::http::string_body>& req,
    std::string_view protocolToken) const {
    if (!protocolToken.empty()) return std::string(protocolToken);

    // Fallback: Cookie header (browser sends HttpOnly cookie automatically on same-origin WS).
    auto cookieIt = req.find(boost::beast::http::field::cookie);
    if (cookieIt != req.end()) {
        std::string cookie(cookieIt->value());
        const std::string kPrefix = "session=";
        auto pos = cookie.find(kPrefix);
        if (pos != std::string::npos) {
            pos += kPrefix.size();
            auto end = cookie.find(';', pos);
            return cookie.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        }
    }
    return {};
}

WsUpgradeResult WebSocketHandler::validateUpgrade(
    const boost::beast::http::request<boost::beast::http::string_body>& req,
    std::string_view clientIp) const {

    // Step 1: RFC 6455 required headers.
    auto upgradeIt  = req.find(boost::beast::http::field::upgrade);
    auto versionIt  = req.find("Sec-WebSocket-Version");
    auto keyIt      = req.find("Sec-WebSocket-Key");

    if (upgradeIt == req.end() || upgradeIt->value() != "websocket") {
        LOG_WARN(common::Logger::get(kLog), "WS upgrade: missing Upgrade: websocket");
        return WsUpgradeResult::fail(common::ErrorCode::kInvalidWebSocketUpgrade);
    }
    if (versionIt == req.end() || versionIt->value() != "13") {
        LOG_WARN(common::Logger::get(kLog), "WS upgrade: Sec-WebSocket-Version != 13");
        return WsUpgradeResult::fail(common::ErrorCode::kInvalidWebSocketUpgrade);
    }
    if (keyIt == req.end()) {
        LOG_WARN(common::Logger::get(kLog), "WS upgrade: missing Sec-WebSocket-Key");
        return WsUpgradeResult::fail(common::ErrorCode::kInvalidWebSocketUpgrade);
    }

    // Step 2: Resolve view type.
    // Priority: URL query string (?view=cl_view) → Sec-WebSocket-Protocol header.
    std::string viewStr = parseViewFromTarget(req.target());
    std::string protocolToken;

    if (viewStr.empty()) {
        auto protocolIt = req.find("Sec-WebSocket-Protocol");
        if (protocolIt != req.end()) {
            auto [v, t] = parseProtocolHeader(protocolIt->value());
            viewStr      = v;
            protocolToken = t;
        }
    }

    if (viewStr.empty()) {
        LOG_WARN(common::Logger::get(kLog), "WS upgrade: no view type specified");
        return WsUpgradeResult::fail(common::ErrorCode::kInvalidStreamType);
    }

    auto viewType = StreamManager::parseViewType(viewStr);
    if (!viewType) {
        LOG_WARN(common::Logger::get(kLog),
            "WS upgrade: invalid view type '{}'", viewStr);
        return WsUpgradeResult::fail(common::ErrorCode::kInvalidStreamType);
    }

    // Step 3: Token validation (protocol header token or HttpOnly session cookie).
    auto token = extractToken(req, protocolToken);
    if (token.empty()) {
        LOG_WARN(common::Logger::get(kLog),
            "WS upgrade: no auth token from ip='{}'", clientIp);
        return WsUpgradeResult::fail(common::ErrorCode::kUnauthorized);
    }

    auto session = sessionStore_->validate(token);
    if (!session) {
        LOG_WARN(common::Logger::get(kLog),
            "WS upgrade: invalid/expired token from ip='{}'", clientIp);
        return WsUpgradeResult::fail(common::ErrorCode::kInvalidToken);
    }

    // Step 4: Capacity check.
    if (streamManager_->clientCount() >= maxSessions_) {
        LOG_WARN(common::Logger::get(kLog),
            "WS upgrade: max sessions ({}) reached", maxSessions_);
        return WsUpgradeResult::fail(common::ErrorCode::kCapacityExceeded);
    }

    LOG_INFO(common::Logger::get(kLog),
        "WS upgrade accepted: user='{}' view='{}' ip='{}'",
        session->username, viewStr, clientIp);

    return WsUpgradeResult::success(*viewType, token);
}

} // namespace streaming
