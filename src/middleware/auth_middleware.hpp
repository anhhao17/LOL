#pragma once

#include "middleware/i_middleware.hpp"
#include "session/session_store.hpp"
#include <memory>
#include <string>
#include <vector>

namespace middleware {

class AuthMiddleware final : public IMiddleware {
public:
    explicit AuthMiddleware(std::shared_ptr<session::SessionStore> store);

    void before(http_layer::HttpRequest& req,
                const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                NextFn next) override;

private:
    std::shared_ptr<session::SessionStore> store_;

    static const std::vector<std::string> kPublicPrefixes;

    [[nodiscard]] bool isPublicPath(std::string_view target) const noexcept;
    [[nodiscard]] std::string extractToken(const http_layer::HttpRequest& req) const;
};

} // namespace middleware
