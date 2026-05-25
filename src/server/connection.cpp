#include "server/connection.hpp"
#include "common/logger.hpp"
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/websocket/rfc6455.hpp>

namespace server {

static constexpr auto kLog = "server";
static constexpr size_t kMaxBodySize = 1 * 1024 * 1024; // 1 MB

Connection::Connection(tcp::socket socket,
                       routing::Router& router,
                       streaming::WebSocketHandler& wsHandler)
    : stream_(std::move(socket))
    , router_(router)
    , wsHandler_(wsHandler) {}

void Connection::start() {
    doRead();
}

std::string Connection::remoteIp() const {
    try {
        return stream_.socket().remote_endpoint().address().to_string();
    } catch (...) {
        return "unknown";
    }
}

void Connection::doRead() {
    req_ = {};
    stream_.expires_after(std::chrono::seconds(30));
    http::async_read(stream_, buffer_, req_,
        [self = shared_from_this()](beast::error_code ec, size_t bytes) {
            self->onRead(ec, bytes);
        });
}

void Connection::onRead(beast::error_code ec, size_t) {
    if (ec == http::error::end_of_stream) {
        doClose();
        return;
    }
    if (ec) {
        LOG_DEBUG(common::Logger::get(kLog), "Read error: {}", ec.message());
        return;
    }

    if (req_.body().size() > kMaxBodySize) {
        http_layer::HttpResponse resp;
        resp.status(413).body("Request entity too large").securityHeaders();
        sendResponse(std::move(resp));
        return;
    }

    // WebSocket upgrade request?
    if (ws::is_upgrade(req_)) {
        upgradeToWebSocket();
        return;
    }

    http_layer::HttpRequest httpReq;
    httpReq.raw      = std::move(req_);
    httpReq.remoteIp = remoteIp();

    auto asyncResp = std::make_shared<http_layer::AsyncResp>(
        [self = shared_from_this()](http_layer::HttpResponse response) {
            self->sendResponse(std::move(response));
        });

    try {
        router_.dispatch(httpReq, asyncResp);
    } catch (const std::exception& ex) {
        LOG_ERROR(common::Logger::get(kLog), "Dispatch exception: {}", ex.what());
        asyncResp->resp = http_layer::HttpResponse::error(
            common::ErrorCode::kInternalError, "Internal server error");
    }
}

void Connection::sendResponse(http_layer::HttpResponse response) {
    auto& raw = response.raw();
    raw.keep_alive(false);
    raw.prepare_payload();

    // Move the response into a shared_ptr so the lambda can own it.
    auto sharedResp = std::make_shared<http_layer::RawResponse>(std::move(raw));
    bool close      = !sharedResp->keep_alive();

    http::async_write(stream_, *sharedResp,
        [self = shared_from_this(), sharedResp, close]
        (beast::error_code ec, size_t bytes) {
            self->onWrite(ec, bytes, close);
        });
}

void Connection::onWrite(beast::error_code ec, size_t, bool close) {
    if (ec) {
        LOG_DEBUG(common::Logger::get(kLog), "Write error: {}", ec.message());
        return;
    }
    if (close) {
        doClose();
        return;
    }
    doRead();
}

void Connection::doClose() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}

void Connection::upgradeToWebSocket() {
    auto result = wsHandler_.validateUpgrade(req_, remoteIp());

    if (result.code != common::ErrorCode::kOk) {
        http_layer::HttpResponse resp;
        resp = http_layer::HttpResponse::error(result.code);
        sendResponse(std::move(resp));
        return;
    }

    wsViewType_    = *result.viewType;
    wsSessionToken_ = result.sessionToken;

    ws_ = std::make_unique<ws::stream<beast::tcp_stream>>(std::move(stream_));
    ws_->set_option(ws::stream_base::decorator([](ws::response_type& res) {
        res.set(boost::beast::http::field::server, "QUANTUM/1.0");
    }));
    ws_->binary(true);

    ws_->async_accept(req_,
        [self = shared_from_this()](beast::error_code ec) {
            if (ec) {
                LOG_WARN(common::Logger::get(kLog), "WS accept error: {}", ec.message());
                return;
            }
            // Register this connection with the stream manager.
            // Using raw pointer — StreamManager does NOT own it.
            auto* wsPtr = self->ws_.get();
            self->wsHandler_.streamManager()->add(wsPtr, self->wsViewType_, self->wsSessionToken_);
            self->doWsRead();
        });
}

void Connection::doWsRead() {
    ws_->async_read(buffer_,
        [self = shared_from_this()](beast::error_code ec, size_t bytes) {
            self->onWsRead(ec, bytes);
        });
}

void Connection::onWsRead(beast::error_code ec, size_t) {
    if (ec == ws::error::closed || ec == net::error::eof) {
        wsHandler_.streamManager()->remove(ws_.get());
        return;
    }
    if (ec) {
        LOG_WARN(common::Logger::get(kLog), "WS read error: {}", ec.message());
        wsHandler_.streamManager()->remove(ws_.get());
        return;
    }
    buffer_.consume(buffer_.size());
    doWsRead();
}

} // namespace server
