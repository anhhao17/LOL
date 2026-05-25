#pragma once

#include "api/i_api_handler.hpp"
#include <filesystem>
#include <string>

namespace api {

class StaticHandler final : public IApiHandler {
public:
    explicit StaticHandler(std::filesystem::path webRoot);

    void registerRoutes(routing::Router& router) override;

private:
    std::filesystem::path webRoot_;

    void serveFile(http_layer::HttpRequest& req,
                   std::shared_ptr<http_layer::AsyncResp> asyncResp);

    [[nodiscard]] static std::string mimeType(const std::filesystem::path& path) noexcept;
    [[nodiscard]] std::filesystem::path resolvePath(const std::string& target) const;
};

} // namespace api
