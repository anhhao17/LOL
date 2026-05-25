#pragma once

#include "http/response.hpp"
#include <functional>
#include <memory>

namespace http_layer {

// RAII wrapper: sends the response when the last shared_ptr is destroyed.
// Handlers receive a shared_ptr<AsyncResp> and populate resp before releasing it.
class AsyncResp {
public:
    using SendFn = std::function<void(HttpResponse)>;

    explicit AsyncResp(SendFn sendFn)
        : sendFn_(std::move(sendFn)) {}

    ~AsyncResp() {
        if (sendFn_) {
            sendFn_(std::move(resp));
        }
    }

    AsyncResp(const AsyncResp&) = delete;
    AsyncResp& operator=(const AsyncResp&) = delete;

    HttpResponse resp;

private:
    SendFn sendFn_;
};

} // namespace http_layer
