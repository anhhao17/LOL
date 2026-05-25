#include "middleware/csrf_middleware.hpp"
#include "common/logger.hpp"

namespace middleware {

bool CsrfMiddleware::isMutatingMethod(std::string_view method) noexcept {
    return method == "POST" || method == "PUT" || method == "DELETE" || method == "PATCH";
}

void CsrfMiddleware::before(http_layer::HttpRequest& req,
                            const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                            NextFn next) {
    // Login endpoint is public — no CSRF check needed.
    if (std::string(req.target()) == "/api/v1/login") {
        next();
        return;
    }

    if (!isMutatingMethod(req.method())) {
        next();
        return;
    }

    if (!req.hasSession()) {
        // Auth middleware will have already rejected unauthenticated requests.
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
