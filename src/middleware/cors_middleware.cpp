#include "middleware/cors_middleware.hpp"
#include "common/logger.hpp"

namespace middleware {

CorsMiddleware::CorsMiddleware(std::vector<std::string> allowedOrigins)
    : allowedOrigins_(std::move(allowedOrigins)) {}

bool CorsMiddleware::isOriginAllowed(std::string_view origin) const noexcept {
    for (const auto& allowed : allowedOrigins_) {
        if (origin == allowed) return true;
    }
    return false;
}

void CorsMiddleware::before(http_layer::HttpRequest& req,
                            const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                            NextFn next) {
    auto origin = req.header("Origin");

    if (!origin.empty()) {
        if (!isOriginAllowed(origin)) {
            LOG_WARN(common::Logger::get("middleware"),
                "CORS rejected origin='{}' path='{}'", origin, req.target());
            asyncResp->resp = http_layer::HttpResponse::error(
                common::ErrorCode::kForbidden, "Origin not allowed");
            return;
        }
        asyncResp->resp.header("Access-Control-Allow-Origin", origin);
        asyncResp->resp.header("Access-Control-Allow-Credentials", "true");
        asyncResp->resp.header("Access-Control-Allow-Headers",
            "Content-Type, Authorization, X-CSRF-Token");
        asyncResp->resp.header("Access-Control-Allow-Methods",
            "GET, POST, PUT, DELETE, OPTIONS");
    }

    // Handle pre-flight.
    if (req.method() == "OPTIONS") {
        asyncResp->resp.status(204);
        return;
    }

    next();
}

} // namespace middleware
