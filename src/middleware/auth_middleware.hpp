#pragma once

#include "middleware/i_middleware.hpp"
#include "session/session_store.hpp"
#include <memory>
#include <string>

namespace middleware {

class AuthMiddleware final : public IMiddleware {
public:
    explicit AuthMiddleware(std::shared_ptr<session::SessionStore> store);

    void before(http_layer::HttpRequest& req,
                const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                NextFn next) override;

private:
    std::shared_ptr<session::SessionStore> store_;

    // Only /api/ routes require auth (except /api/v1/login which is always public).
    [[nodiscard]] static bool isProtectedPath(std::string_view target) noexcept;
    [[nodiscard]] std::string extractToken(const http_layer::HttpRequest& req) const;
};

} // namespace middleware
