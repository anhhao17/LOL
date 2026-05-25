#pragma once

#include "routing/router.hpp"
#include "streaming/websocket_handler.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <string>

namespace server {

namespace net = boost::asio;
using tcp     = net::ip::tcp;

class HttpServer {
public:
    HttpServer(net::io_context& ioc,
               const tcp::endpoint& endpoint,
               routing::Router& router,
               streaming::WebSocketHandler& wsHandler);

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    void start();
    void stop();

private:
    void doAccept();
    void onAccept(boost::system::error_code ec, tcp::socket socket);

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    routing::Router& router_;
    streaming::WebSocketHandler& wsHandler_;
};

} // namespace server
