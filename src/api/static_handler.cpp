#include "api/static_handler.hpp"
#include "common/logger.hpp"
#include <boost/beast/http/verb.hpp>
#include <fstream>
#include <sstream>

namespace api {

namespace http = boost::beast::http;

StaticHandler::StaticHandler(std::filesystem::path webRoot)
    : webRoot_(std::move(webRoot)) {}

void StaticHandler::registerRoutes(routing::Router& router) {
    // Catch-all: serves any path not already claimed by an API route.
    // The trie resolves specific routes (e.g. /api/**) before falling back here.
    router.addRoute(http::verb::get, "/#",
        [this](http_layer::HttpRequest& req, std::shared_ptr<http_layer::AsyncResp> asyncResp) {
            serveFile(req, std::move(asyncResp));
        });
    router.addRoute(http::verb::get, "/",
        [this](http_layer::HttpRequest& req, std::shared_ptr<http_layer::AsyncResp> asyncResp) {
            serveFile(req, std::move(asyncResp));
        });
}

std::filesystem::path StaticHandler::resolvePath(const std::string& target) const {
    // Strip query string before resolving.
    auto qpos        = target.find('?');
    std::string path = (qpos != std::string::npos) ? target.substr(0, qpos) : target;

    // Map bare "/" to index.html; everything else maps directly into webRoot_.
    std::string relativePart = (path == "/") ? "/index.html" : path;

    auto resolved = std::filesystem::weakly_canonical(
        webRoot_ / relativePart.substr(1)); // remove leading /

    // Path traversal guard: resolved must be inside webRoot_.
    auto root = std::filesystem::weakly_canonical(webRoot_);
    auto rel  = std::mismatch(root.begin(), root.end(), resolved.begin());
    if (rel.first != root.end()) {
        return {}; // traversal attempt
    }
    return resolved;
}

std::string StaticHandler::mimeType(const std::filesystem::path& path) noexcept {
    auto ext = path.extension().string();
    if (ext == ".html") return "text/html; charset=utf-8";
    if (ext == ".js")   return "application/javascript";
    if (ext == ".css")  return "text/css";
    if (ext == ".json") return "application/json";
    if (ext == ".png")  return "image/png";
    if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".ico")  return "image/x-icon";
    return "application/octet-stream";
}

void StaticHandler::serveFile(http_layer::HttpRequest& req,
                              std::shared_ptr<http_layer::AsyncResp> asyncResp) {
    auto filePath = resolvePath(req.target());

    if (filePath.empty() || !std::filesystem::exists(filePath)) {
        // SPA fallback: always serve index.html for client-side routes.
        filePath = webRoot_ / "index.html";
    }

    if (!std::filesystem::exists(filePath)) {
        asyncResp->resp = http_layer::HttpResponse::notFound();
        return;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR(common::Logger::get("api"), "Failed to open static file: {}", filePath.string());
        asyncResp->resp = http_layer::HttpResponse::error(
            common::ErrorCode::kInternalError, "Failed to read file");
        return;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    asyncResp->resp.status(200)
        .body(ss.str(), mimeType(filePath))
        .securityHeaders()
        .header("Cache-Control", "public, max-age=3600");
}

} // namespace api
