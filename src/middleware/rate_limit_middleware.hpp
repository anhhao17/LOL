#pragma once

#include "middleware/i_middleware.hpp"
#include <chrono>
#include <map>
#include <mutex>
#include <string>

namespace middleware {

static constexpr unsigned int kDefaultMaxRequests = 200;
static constexpr auto kDefaultWindow = std::chrono::seconds(10);

class RateLimitMiddleware final : public IMiddleware {
public:
    explicit RateLimitMiddleware(
        unsigned int maxRequests = kDefaultMaxRequests,
        std::chrono::seconds window = kDefaultWindow);

    void before(http_layer::HttpRequest& req,
                const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                NextFn next) override;

private:
    struct Bucket {
        unsigned int count{0};
        std::chrono::steady_clock::time_point windowStart{std::chrono::steady_clock::now()};
    };

    unsigned int maxRequests_;
    std::chrono::seconds window_;
    std::map<std::string, Bucket> buckets_;
    std::mutex mutex_;
};

} // namespace middleware
