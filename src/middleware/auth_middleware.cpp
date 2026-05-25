#include "middleware/auth_middleware.hpp"
#include "common/logger.hpp"

namespace middleware {

const std::vector<std::string> AuthMiddleware::kPublicPrefixes{
    "/api/v1/login",
    "/ui/",
    "/health",
};

AuthMiddleware::AuthMiddleware(std::shared_ptr<session::SessionStore> store)
    : store_(std::move(store)) {}

bool AuthMiddleware::isPublicPath(std::string_view target) const noexcept {
    for (const auto& prefix : kPublicPrefixes) {
        if (target.rfind(prefix, 0) == 0) return true;
    }
    return false;
}

std::string AuthMiddleware::extractToken(const http_layer::HttpRequest& req) const {
    // 1. Authorization: Bearer <token>
    auto authHeader = req.header("Authorization");
    static constexpr std::string_view kBearer = "Bearer ";
    if (authHeader.rfind(kBearer, 0) == 0) {
        return authHeader.substr(kBearer.size());
    }
    // 2. Cookie: session=<token>
    auto cookie = req.header("Cookie");
    const std::string kPrefix = "session=";
    auto pos = cookie.find(kPrefix);
    if (pos != std::string::npos) {
        pos += kPrefix.size();
        auto end = cookie.find(';', pos);
        return cookie.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
    }
    return {};
}

void AuthMiddleware::before(http_layer::HttpRequest& req,
                            const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                            NextFn next) {
    if (isPublicPath(req.target())) {
        next();
        return;
    }

    auto token = extractToken(req);
    if (token.empty()) {
        LOG_WARN(common::Logger::get("middleware"),
            "Rejected unauthenticated request: {} {}", req.method(), req.target());
        asyncResp->resp = http_layer::HttpResponse::error(
            common::ErrorCode::kUnauthorized, "Authentication required");
        return;
    }

    auto session = store_->validate(token);
    if (!session) {
        LOG_WARN(common::Logger::get("middleware"),
            "Invalid or expired session token from ip='{}'", req.remoteIp);
        asyncResp->resp = http_layer::HttpResponse::error(
            common::ErrorCode::kSessionExpired, "Session expired or invalid");
        return;
    }

    req.session = std::move(session);
    next();
}

} // namespace middleware
