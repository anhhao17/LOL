#pragma once

#include "session/session.hpp"
#include <boost/beast/http.hpp>
#include <optional>
#include <string>

namespace http_layer {

namespace beast = boost::beast;
namespace http  = beast::http;

struct HttpRequest {
    http::request<http::string_body> raw;
    std::string remoteIp;

    // Populated by auth middleware after token validation.
    std::optional<session::Session> session;

    [[nodiscard]] std::string method() const {
        return std::string(http::to_string(raw.method()));
    }

    [[nodiscard]] std::string target() const {
        return std::string(raw.target());
    }

    [[nodiscard]] std::string body() const {
        return raw.body();
    }

    [[nodiscard]] std::string header(std::string_view name) const {
        auto it = raw.find(name);
        if (it == raw.end()) return {};
        return std::string(it->value());
    }

    [[nodiscard]] bool hasSession() const noexcept {
        return session.has_value();
    }
};

} // namespace http_layer
