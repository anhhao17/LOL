#include "middleware/logging_middleware.hpp"
#include "common/logger.hpp"

namespace middleware {

void LoggingMiddleware::before(http_layer::HttpRequest& req,
                               const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                               NextFn next) {
    auto start = std::chrono::steady_clock::now();

    LOG_INFO(common::Logger::get("middleware"),
        "--> {} {} ip='{}'", req.method(), req.target(), req.remoteIp);

    auto userAgent = std::string(req.raw[boost::beast::http::field::user_agent]);
    auto referer   = std::string(req.raw[boost::beast::http::field::referer]);
    if (!userAgent.empty())
        LOG_DEBUG(common::Logger::get("middleware"), "    User-Agent: {}", userAgent);
    if (!referer.empty())
        LOG_DEBUG(common::Logger::get("middleware"), "    Referer: {}", referer);

    next();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    LOG_INFO(common::Logger::get("middleware"),
        "<-- {} {} status={} elapsed={}ms",
        req.method(), req.target(),
        asyncResp->resp.raw().result_int(), elapsed);
}

} // namespace middleware
