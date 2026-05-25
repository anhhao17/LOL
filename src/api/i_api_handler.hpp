#pragma once

#include "http/request.hpp"
#include "http/async_resp.hpp"
#include "routing/router.hpp"
#include <memory>

namespace api {

class IApiHandler {
public:
    virtual ~IApiHandler() = default;

    virtual void registerRoutes(routing::Router& router) = 0;
};

} // namespace api
