#pragma once

#include "http/request.hpp"
#include "http/async_resp.hpp"
#include <functional>
#include <memory>

namespace middleware {

using NextFn = std::function<void()>;

class IMiddleware {
public:
    virtual ~IMiddleware() = default;

    // Called before the route handler. Call next() to continue the chain.
    // Set resp on asyncResp and return without calling next() to short-circuit.
    virtual void before(http_layer::HttpRequest& req,
                        const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                        NextFn next) = 0;
};

} // namespace middleware
