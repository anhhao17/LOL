#include "server/http_server.hpp"
#include "server/connection.hpp"
#include "common/logger.hpp"
#include <boost/asio/strand.hpp>

namespace server {

static constexpr auto kLog = "server";

HttpServer::HttpServer(net::io_context& ioc,
                       const tcp::endpoint& endpoint,
                       routing::Router& router,
                       streaming::WebSocketHandler& wsHandler)
    : ioc_(ioc)
    , acceptor_(net::make_strand(ioc))
    , router_(router)
    , wsHandler_(wsHandler) {
    boost::system::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) throw std::runtime_error("acceptor open: " + ec.message());

    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) throw std::runtime_error("acceptor set_option: " + ec.message());

    acceptor_.bind(endpoint, ec);
    if (ec) throw std::runtime_error("acceptor bind: " + ec.message());

    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) throw std::runtime_error("acceptor listen: " + ec.message());

    LOG_INFO(common::Logger::get(kLog),
        "HTTP server listening on {}:{}",
        endpoint.address().to_string(), endpoint.port());
}

void HttpServer::start() {
    doAccept();
}

void HttpServer::stop() {
    boost::system::error_code ec;
    acceptor_.close(ec);
}

void HttpServer::doAccept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        [this](boost::system::error_code ec, tcp::socket socket) {
            onAccept(ec, std::move(socket));
        });
}

void HttpServer::onAccept(boost::system::error_code ec, tcp::socket socket) {
    if (ec == net::error::operation_aborted) return;

    if (ec) {
        LOG_ERROR(common::Logger::get(kLog), "Accept error: {}", ec.message());
    } else {
        std::make_shared<Connection>(
            std::move(socket), router_, wsHandler_)->start();
    }

    doAccept();
}

} // namespace server
