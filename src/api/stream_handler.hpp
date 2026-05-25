#pragma once

#include "api/i_api_handler.hpp"
#include "app/shared_context.hpp"
#include "streaming/websocket_handler.hpp"
#include <memory>

namespace api {

class StreamHandler : public IApiHandler {
public:
    explicit StreamHandler(const app::SharedContext& ctx,
                          streaming::WebSocketHandler& wsHandler);

    void registerRoutes(routing::Router& router) override;

private:
    void handleStream(http_layer::HttpRequest& req,
                      std::shared_ptr<http_layer::AsyncResp> asyncResp);

    app::SharedContext ctx_;
    streaming::WebSocketHandler& wsHandler_;
};

} // namespace api