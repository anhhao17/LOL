#pragma once

#include "common/error.hpp"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace http_layer {

namespace beast = boost::beast;
namespace http  = beast::http;

using RawResponse = http::response<http::string_body>;

class HttpResponse {
public:
    HttpResponse() { raw_.version(11); }

    HttpResponse& status(unsigned int code) noexcept {
        raw_.result(code);
        return *this;
    }

    HttpResponse& header(std::string_view name, std::string_view value) {
        raw_.set(name, value);
        return *this;
    }

    HttpResponse& body(std::string content, std::string_view contentType = "text/plain") {
        raw_.set(http::field::content_type, contentType);
        raw_.body() = std::move(content);
        raw_.prepare_payload();
        return *this;
    }

    HttpResponse& json(const nlohmann::json& payload) {
        raw_.set(http::field::content_type, "application/json");
        raw_.body() = payload.dump();
        raw_.prepare_payload();
        return *this;
    }

    HttpResponse& keepAlive(bool value) noexcept {
        raw_.keep_alive(value);
        return *this;
    }

    // Secure headers applied to every response.
    HttpResponse& securityHeaders() {
        raw_.set("X-Content-Type-Options", "nosniff");
        raw_.set("X-Frame-Options", "DENY");
        raw_.set("Referrer-Policy", "strict-origin-when-cross-origin");
        raw_.set("Content-Security-Policy",
            "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' blob:");
        return *this;
    }

    [[nodiscard]] RawResponse& raw() noexcept { return raw_; }
    [[nodiscard]] const RawResponse& raw() const noexcept { return raw_; }

    // Factory helpers ---------------------------------------------------------

    static HttpResponse ok(const nlohmann::json& data) {
        HttpResponse r;
        r.status(200).json(nlohmann::json{{"data", data}}).securityHeaders();
        return r;
    }

    static HttpResponse okEmpty() {
        HttpResponse r;
        r.status(204).securityHeaders();
        return r;
    }

    static HttpResponse error(const common::ApiError& err) {
        HttpResponse r;
        r.status(static_cast<unsigned int>(err.httpStatus())).json(err.toJson()).securityHeaders();
        return r;
    }

    static HttpResponse error(common::ErrorCode code, std::string message = {}) {
        return error(message.empty()
            ? common::ApiError{code}
            : common::ApiError{code, std::move(message)});
    }

    static HttpResponse notFound() {
        return error(common::ErrorCode::kNotFound, "Resource not found");
    }

    static HttpResponse methodNotAllowed() {
        return error(common::ErrorCode::kMethodNotAllowed, "Method not allowed");
    }

private:
    RawResponse raw_;
};

} // namespace http_layer
