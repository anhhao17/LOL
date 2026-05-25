#include "routing/router.hpp"
#include "common/logger.hpp"

namespace routing {

void Router::addMiddleware(std::unique_ptr<middleware::IMiddleware> mw) {
    middleware_.push_back(std::move(mw));
}

void Router::addRoute(http::verb method, const std::string& pattern, HandlerFn handler) {
    size_t index = routes_.size();
    routes_.push_back({method, pattern, std::move(handler)});
    trie_.insert(pattern, index);
    LOG_DEBUG(common::Logger::get("router"),
        "Route registered: {} {}", std::string(http::to_string(method)), pattern);
}

void Router::runMiddleware(size_t index,
                           http_layer::HttpRequest& req,
                           const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                           std::function<void()> final_) {
    if (index >= middleware_.size()) {
        final_();
        return;
    }
    middleware_[index]->before(req, asyncResp, [this, index, &req, &asyncResp, final_]() {
        runMiddleware(index + 1, req, asyncResp, final_);
    });
}

void Router::dispatch(http_layer::HttpRequest& req,
                      std::shared_ptr<http_layer::AsyncResp> asyncResp) {
    auto routeIndex = trie_.match(req.target());

    if (!routeIndex) {
        asyncResp->resp = http_layer::HttpResponse::notFound();
        return;
    }

    const auto& route = routes_[*routeIndex];

    if (route.method != http::verb::unknown && req.raw.method() != route.method) {
        asyncResp->resp = http_layer::HttpResponse::methodNotAllowed();
        return;
    }

    runMiddleware(0, req, asyncResp, [&route, &req, &asyncResp]() {
        route.handler(req, asyncResp);
    });
}

} // namespace routing
