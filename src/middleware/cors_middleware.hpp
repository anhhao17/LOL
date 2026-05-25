#pragma once

#include "middleware/i_middleware.hpp"
#include <string>
#include <vector>

namespace middleware {

class CorsMiddleware final : public IMiddleware {
public:
    explicit CorsMiddleware(std::vector<std::string> allowedOrigins);

    void before(http_layer::HttpRequest& req,
                const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                NextFn next) override;

private:
    std::vector<std::string> allowedOrigins_;
    [[nodiscard]] bool isOriginAllowed(std::string_view origin) const noexcept;
};

} // namespace middleware
