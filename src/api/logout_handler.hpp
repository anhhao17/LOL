#pragma once

#include "api/i_api_handler.hpp"
#include "app/shared_context.hpp"

namespace api {

class LogoutHandler final : public IApiHandler {
public:
    explicit LogoutHandler(const app::SharedContext& ctx);

    void registerRoutes(routing::Router& router) override;

private:
    const app::SharedContext& ctx_;

    void handleLogout(http_layer::HttpRequest& req,
                      std::shared_ptr<http_layer::AsyncResp> asyncResp);
};

} // namespace api
