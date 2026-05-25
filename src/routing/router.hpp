#pragma once

#include "routing/trie.hpp"
#include "http/request.hpp"
#include "http/async_resp.hpp"
#include "middleware/i_middleware.hpp"
#include <boost/beast/http/verb.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace routing {

namespace http = boost::beast::http;

using HandlerFn = std::function<void(http_layer::HttpRequest&,
                                     std::shared_ptr<http_layer::AsyncResp>)>;

class Router {
public:
    Router() = default;
    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    void addMiddleware(std::unique_ptr<middleware::IMiddleware> mw);

    void addRoute(http::verb method, const std::string& pattern, HandlerFn handler);

    void dispatch(http_layer::HttpRequest& req,
                  std::shared_ptr<http_layer::AsyncResp> asyncResp);

private:
    struct Route {
        http::verb method;
        std::string pattern;
        HandlerFn handler;
    };

    std::vector<std::unique_ptr<middleware::IMiddleware>> middleware_;
    std::vector<Route> routes_;
    Trie trie_;

    void runMiddleware(size_t index,
                       http_layer::HttpRequest& req,
                       const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                       std::function<void()> final_);
};

} // namespace routing
