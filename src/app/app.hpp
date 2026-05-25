#pragma once

#include "app/config_manager.hpp"
#include "app/shared_context.hpp"
#include "routing/router.hpp"
#include "server/http_server.hpp"
#include "streaming/websocket_handler.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <memory>
#include <vector>
#include <thread>

namespace app {

class App {
public:
    App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    int run(int argc, char** argv);

private:
    void setupContext(const Config& cfg);
    void setupRoutes(const Config& cfg);
    void setupMiddleware(const Config& cfg);
    void waitForSignal();

    boost::asio::io_context ioc_;
    std::unique_ptr<routing::Router> router_;
    std::unique_ptr<streaming::WebSocketHandler> wsHandler_;
    std::unique_ptr<server::HttpServer> httpServer_;
    SharedContext ctx_;
    std::vector<std::thread> threads_;
};

} // namespace app
