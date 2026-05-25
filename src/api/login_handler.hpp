#pragma once

#include "api/i_api_handler.hpp"
#include "app/shared_context.hpp"

namespace api {

class LoginHandler final : public IApiHandler {
public:
    explicit LoginHandler(const app::SharedContext& ctx);

    void registerRoutes(routing::Router& router) override;

private:
    const app::SharedContext& ctx_;

    void handleLogin(http_layer::HttpRequest& req,
                     std::shared_ptr<http_layer::AsyncResp> asyncResp);
};

} // namespace api
