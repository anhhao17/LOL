#include "app/app.hpp"
#include "api/login_handler.hpp"
#include "api/logout_handler.hpp"
#include "api/static_handler.hpp"
#include "common/logger.hpp"
#include "middleware/auth_middleware.hpp"
#include "middleware/cors_middleware.hpp"
#include "middleware/csrf_middleware.hpp"
#include "middleware/logging_middleware.hpp"
#include "middleware/rate_limit_middleware.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <sodium.h>
#include <spdlog/spdlog.h>
#include <iostream>

namespace app {

static constexpr auto kLog = "app";

int App::run(int argc, char** argv) {
    ConfigManager cfgMgr;
    try {
        if (!cfgMgr.parse(argc, argv)) return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Config error: " << ex.what() << '\n';
        return 1;
    }

    const auto& cfg = cfgMgr.config();

    // Init logging before anything else.
    auto level = spdlog::level::from_str(cfg.logLevel);
    common::Logger::init(cfg.logFile.string(), level);

    LOG_INFO(common::Logger::get(kLog), "QUANTUM starting up...");

    // Init libsodium.
    if (sodium_init() < 0) {
        LOG_CRITICAL(common::Logger::get(kLog), "libsodium init failed");
        return 1;
    }

    try {
        setupContext(cfg);
        setupMiddleware(cfg);
        setupRoutes(cfg);

        boost::asio::ip::tcp::endpoint endpoint{
            boost::asio::ip::make_address("0.0.0.0"), cfg.port};

        httpServer_ = std::make_unique<server::HttpServer>(
            ioc_, endpoint, *router_, *wsHandler_);

        httpServer_->start();

        LOG_INFO(common::Logger::get(kLog),
            "Server started on port {} with {} threads", cfg.port, cfg.threadCount);

        // Launch IO threads.
        threads_.reserve(cfg.threadCount - 1);
        for (size_t i = 1; i < cfg.threadCount; ++i) {
            threads_.emplace_back([this] { ioc_.run(); });
        }

        waitForSignal();
        ioc_.run();

        for (auto& t : threads_) t.join();

    } catch (const std::exception& ex) {
        LOG_CRITICAL(common::Logger::get(kLog), "Fatal: {}", ex.what());
        return 1;
    }

    LOG_INFO(common::Logger::get(kLog), "QUANTUM shut down cleanly.");
    common::Logger::shutdown();
    return 0;
}

void App::setupContext(const Config& cfg) {
    ctx_.sessionStore  = std::make_shared<session::SessionStore>();
    ctx_.userStore     = std::make_shared<auth::UserStore>(cfg.dbPath.string());
    ctx_.failDelay     = std::make_shared<auth::FailDelay>();
    ctx_.streamManager = std::make_shared<streaming::StreamManager>();

    wsHandler_ = std::make_unique<streaming::WebSocketHandler>(
        ctx_.streamManager, ctx_.sessionStore, cfg.maxWsSessions);
}

void App::setupMiddleware(const Config& cfg) {
    router_ = std::make_unique<routing::Router>();

    // Pipeline order: Auth → CSRF → Rate Limit → Logging → CORS
    router_->addMiddleware(
        std::make_unique<middleware::AuthMiddleware>(ctx_.sessionStore));
    router_->addMiddleware(
        std::make_unique<middleware::CsrfMiddleware>());
    router_->addMiddleware(
        std::make_unique<middleware::RateLimitMiddleware>(
            cfg.maxRequestsPerWindow, cfg.rateWindow));
    router_->addMiddleware(
        std::make_unique<middleware::LoggingMiddleware>());
    router_->addMiddleware(
        std::make_unique<middleware::CorsMiddleware>(cfg.allowedOrigins));
}

void App::setupRoutes(const Config& cfg) {
    api::LoginHandler{ctx_}.registerRoutes(*router_);
    api::LogoutHandler{ctx_}.registerRoutes(*router_);
    api::StaticHandler{cfg.webRoot}.registerRoutes(*router_);
}

void App::waitForSignal() {
    auto signals = std::make_shared<boost::asio::signal_set>(ioc_, SIGINT, SIGTERM);
    signals->async_wait([this, signals](boost::system::error_code, int sig) {
        LOG_INFO(common::Logger::get(kLog), "Signal {} received — shutting down", sig);
        httpServer_->stop();
        ioc_.stop();
    });
}

} // namespace app
