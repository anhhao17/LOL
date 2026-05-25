#pragma once

#include "http/request.hpp"
#include "http/response.hpp"
#include "http/async_resp.hpp"
#include "routing/router.hpp"
#include "streaming/websocket_handler.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <memory>
#include <string>

namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;
namespace ws    = beast::websocket;
namespace net   = boost::asio;
using tcp       = net::ip::tcp;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(tcp::socket socket,
               routing::Router& router,
               streaming::WebSocketHandler& wsHandler);

    void start();

private:
    void doRead();
    void onRead(beast::error_code ec, size_t);
    void sendResponse(http_layer::HttpResponse response);
    void onWrite(beast::error_code ec, size_t, bool close);
    void doClose();

    // WebSocket upgrade path.
    void upgradeToWebSocket();
    void doWsRead();
    void onWsRead(beast::error_code ec, size_t bytes);
    void onWsWrite(beast::error_code ec, size_t);

    [[nodiscard]] std::string remoteIp() const;

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    routing::Router& router_;
    streaming::WebSocketHandler& wsHandler_;

    // Active WebSocket connection (non-null after upgrade).
    std::unique_ptr<ws::stream<beast::tcp_stream>> ws_;
    streaming::ViewType wsViewType_{streaming::ViewType::kColorMain};
    std::string wsSessionToken_;
};

} // namespace server
