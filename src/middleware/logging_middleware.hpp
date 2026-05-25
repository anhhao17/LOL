#pragma once

#include "middleware/i_middleware.hpp"
#include <chrono>

namespace middleware {

class LoggingMiddleware final : public IMiddleware {
public:
    LoggingMiddleware() = default;

    void before(http_layer::HttpRequest& req,
                const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                NextFn next) override;
};

} // namespace middleware
