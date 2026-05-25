#include "api/login_handler.hpp"
#include "auth/password_hasher.hpp"
#include "common/logger.hpp"
#include <nlohmann/json.hpp>
#include <boost/beast/http/verb.hpp>

namespace api {

namespace http = boost::beast::http;

LoginHandler::LoginHandler(const app::SharedContext& ctx)
    : ctx_(ctx) {}

void LoginHandler::registerRoutes(routing::Router& router) {
    router.addRoute(http::verb::post, "/api/v1/login",
        [this](http_layer::HttpRequest& req, std::shared_ptr<http_layer::AsyncResp> asyncResp) {
            handleLogin(req, std::move(asyncResp));
        });
}

void LoginHandler::handleLogin(http_layer::HttpRequest& req,
                               std::shared_ptr<http_layer::AsyncResp> asyncResp) {
    // 1. Rate limit / lockout check.
    auto lockResult = ctx_.failDelay->check(req.remoteIp);
    if (lockResult != common::ErrorCode::kOk) {
        asyncResp->resp = http_layer::HttpResponse::error(
            lockResult, "Too many failed attempts — try again later");
        return;
    }

    // 2. Parse and validate JSON body.
    nlohmann::json body;
    try {
        body = nlohmann::json::parse(req.body());
    } catch (...) {
        asyncResp->resp = http_layer::HttpResponse::error(
            common::ErrorCode::kInvalidInput, "Request body must be valid JSON");
        return;
    }

    if (!body.contains("username") || !body.contains("password") ||
        !body["username"].is_string() || !body["password"].is_string()) {
        asyncResp->resp = http_layer::HttpResponse::error(
            common::ErrorCode::kInvalidInput, "Fields 'username' and 'password' are required");
        return;
    }

    std::string username = body["username"].get<std::string>();
    std::string password = body["password"].get<std::string>();

    // 3. Input length guard (never log the password).
    if (username.empty() || username.size() > 64 || password.empty() || password.size() > 256) {
        asyncResp->resp = http_layer::HttpResponse::error(
            common::ErrorCode::kInvalidInput, "Invalid credential format");
        return;
    }

    // 4. User lookup + constant-time verify. Always same error to prevent enumeration.
    auto user = ctx_.userStore->findUser(username);
    bool valid = user.has_value() && auth::PasswordHasher::verify(password, user->passwordHash);

    if (!valid) {
        ctx_.failDelay->recordFailure(req.remoteIp);
        LOG_WARN(common::Logger::get("api"),
            "Failed login attempt for username='{}' ip='{}'", username, req.remoteIp);
        asyncResp->resp = http_layer::HttpResponse::error(
            common::ErrorCode::kInvalidCredentials, "Invalid username or password");
        return;
    }

    // 5. Create session.
    ctx_.failDelay->recordSuccess(req.remoteIp);
    auto session = ctx_.sessionStore->create(username, user->role, req.remoteIp);

    LOG_INFO(common::Logger::get("api"),
        "Login success: username='{}' ip='{}'", username, req.remoteIp);

    // 6. Build response: CSRF token in body, session token in cookie.
    nlohmann::json data{
        {"csrfToken",    session.csrfToken},
        {"username",     session.username},
        {"role",         static_cast<unsigned int>(session.role)},
    };

    asyncResp->resp = http_layer::HttpResponse::ok(data);
    asyncResp->resp.header("Set-Cookie",
        "session=" + session.token +
        "; HttpOnly; Secure; SameSite=Strict; Path=/");
}

} // namespace api
