#include "api/logout_handler.hpp"
#include "common/logger.hpp"
#include <nlohmann/json.hpp>
#include <boost/beast/http/verb.hpp>

namespace api {

namespace http = boost::beast::http;

LogoutHandler::LogoutHandler(const app::SharedContext& ctx)
    : ctx_(ctx) {}

void LogoutHandler::registerRoutes(routing::Router& router) {
    router.addRoute(http::verb::post, "/api/v1/logout",
        [this](http_layer::HttpRequest& req, std::shared_ptr<http_layer::AsyncResp> asyncResp) {
            handleLogout(req, std::move(asyncResp));
        });
}

void LogoutHandler::handleLogout(http_layer::HttpRequest& req,
                                 std::shared_ptr<http_layer::AsyncResp> asyncResp) {
    if (req.hasSession()) {
        LOG_INFO(common::Logger::get("api"),
            "Logout: username='{}' ip='{}'", req.session->username, req.remoteIp);
        ctx_.sessionStore->revoke(req.session->token);
    }

    asyncResp->resp = http_layer::HttpResponse::ok(nlohmann::json{});
    // Clear the session cookie.
    asyncResp->resp.header("Set-Cookie",
        "session=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0");
}

} // namespace api
