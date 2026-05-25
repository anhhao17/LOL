#include "middleware/logging_middleware.hpp"
#include "common/logger.hpp"

namespace middleware {

void LoggingMiddleware::before(http_layer::HttpRequest& req,
                               const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                               NextFn next) {
    auto start = std::chrono::steady_clock::now();

    LOG_INFO(common::Logger::get("middleware"),
        "--> {} {} ip='{}'", req.method(), req.target(), req.remoteIp);

    // Wrap the AsyncResp send to log after response is determined.
    // We capture a reference to the resp that will be populated by the handler.
    auto& resp = asyncResp->resp;
    (void)resp; // resp logged in ~AsyncResp path — log the request here.

    next();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    LOG_INFO(common::Logger::get("middleware"),
        "<-- {} {} status={} elapsed={}ms",
        req.method(), req.target(),
        asyncResp->resp.raw().result_int(), elapsed);
}

} // namespace middleware
