#pragma once

#include "middleware/i_middleware.hpp"

namespace middleware {

// Verifies X-CSRF-Token header on mutating methods (POST, PUT, DELETE, PATCH).
// Skipped for public paths (auth middleware handles those first).
class CsrfMiddleware final : public IMiddleware {
public:
    CsrfMiddleware() = default;

    void before(http_layer::HttpRequest& req,
                const std::shared_ptr<http_layer::AsyncResp>& asyncResp,
                NextFn next) override;

private:
    [[nodiscard]] static bool isMutatingMethod(std::string_view method) noexcept;
};

} // namespace middleware
