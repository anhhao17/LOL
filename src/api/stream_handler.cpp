#include "api/stream_handler.hpp"
#include "common/logger.hpp"
#include <boost/beast/http/verb.hpp>

namespace api {

namespace http = boost::beast::http;

StreamHandler::StreamHandler(const app::SharedContext& ctx,
                             streaming::WebSocketHandler& wsHandler)
    : ctx_(ctx)
    , wsHandler_(wsHandler) {}

void StreamHandler::registerRoutes(routing::Router& router) {
    // Register a catch-all route for /api/v1/stream to handle pre-upgrade validation
    router.addRoute(http::verb::get, "/api/v1/stream",
        [this](http_layer::HttpRequest& req, std::shared_ptr<http_layer::AsyncResp> asyncResp) {
            handleStream(req, std::move(asyncResp));
        });
}

void StreamHandler::handleStream(http_layer::HttpRequest& req,
                                 std::shared_ptr<http_layer::AsyncResp> asyncResp) {
    // This handler is called for plain HTTP GET requests to /api/v1/stream
    // (without WebSocket upgrade headers). We perform the same validation as
    // the WebSocket upgrade logic but return HTTP error codes instead.

    auto result = wsHandler_.validateUpgrade(req.raw, req.remoteIp);

    if (result.code != common::ErrorCode::kOk) {
        asyncResp->resp = http_layer::HttpResponse::error(result.code);
        return;
    }

    // If validation passes but this is not a WebSocket upgrade request,
    // return an error indicating this is a WebSocket-only endpoint.
    asyncResp->resp = http_layer::HttpResponse::error(
        common::ErrorCode::kInvalidWebSocketUpgrade,
        "This endpoint requires WebSocket upgrade");
}

} // namespace api