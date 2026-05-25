#include "middleware/rate_limit_middleware.hpp"
#include "common/logger.hpp"

namespace middleware {

RateLimitMiddleware::RateLimitMiddleware(unsigned int maxRequests, std::chrono::seconds window)
    : maxRequests_(maxRequests)
    , window_(window) {}

void RateLimitMiddleware::before(http_layer::HttpRequest& req,
                                 const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                                 NextFn next) {
    const auto now = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& bucket = buckets_[req.remoteIp];

        if (now - bucket.windowStart > window_) {
            bucket.count       = 0;
            bucket.windowStart = now;
        }

        ++bucket.count;

        if (bucket.count > maxRequests_) {
            LOG_WARN(common::Logger::get("middleware"),
                "Rate limit exceeded for ip='{}' count={}", req.remoteIp, bucket.count);
            asyncResp->resp = http_layer::HttpResponse::error(
                common::ErrorCode::kRateLimitExceeded,
                "Too many requests — slow down");
            asyncResp->resp.header("Retry-After", std::to_string(window_.count()));
            return;
        }
    }

    next();
}

} // namespace middleware
