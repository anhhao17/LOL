#include "middleware/csrf_middleware.hpp"
#include "common/logger.hpp"

namespace middleware {

bool CsrfMiddleware::isMutatingMethod(std::string_view method) noexcept {
    return method == "POST" || method == "PUT" || method == "DELETE" || method == "PATCH";
}

void CsrfMiddleware::before(http_layer::HttpRequest& req,
                            const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                            NextFn next) {
    // Login and stream endpoints are public — no CSRF check needed.
    if (std::string(req.target()) == "/api/v1/login" ||
        std::string(req.target()).rfind("/api/v1/stream", 0) == 0) {
        next();
        return;
    }

    if (!isMutatingMethod(req.method())) {
        next();
        return;
    }

    // If there's no session, Auth middleware will have already rejected
    // unauthenticated requests for protected paths. Skip CSRF check.
    if (!req.hasSession()) {
        next();
        return;
    }

    auto csrfHeader = req.header("X-CSRF-Token");
    if (csrfHeader.empty() || csrfHeader != req.session->csrfToken) {
        LOG_WARN(common::Logger::get("middleware"),
            "CSRF token mismatch for user='{}' path='{}'",
            req.session->username, req.target());
        asyncResp->resp = http_layer::HttpResponse::error(
            common::ErrorCode::kCsrfTokenMismatch, "CSRF token missing or invalid");
        return;
    }

    next();
}

} // namespace middleware
