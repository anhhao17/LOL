#pragma once

#include "session/session_store.hpp"
#include "auth/user_store.hpp"
#include "auth/fail_delay.hpp"
#include <memory>

namespace streaming { class StreamManager; }

namespace app {

struct SharedContext {
    std::shared_ptr<session::SessionStore> sessionStore;
    std::shared_ptr<auth::UserStore> userStore;
    std::shared_ptr<auth::FailDelay> failDelay;
    std::shared_ptr<streaming::StreamManager> streamManager;
};

} // namespace app
