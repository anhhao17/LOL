#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

namespace common {

enum class ErrorCode {
    kOk,
    kInvalidInput,
    kInvalidCredentials,
    kUnauthorized,
    kForbidden,
    kNotFound,
    kMethodNotAllowed,
    kTooManyRequests,
    kRateLimitExceeded,
    kServiceUnavailable,
    kInternalError,
    kCsrfTokenMismatch,
    kSessionExpired,
    kUserAlreadyExists,
    kUserNotFound,
    kCapacityExceeded,
    kInvalidWebSocketUpgrade,
    kInvalidStreamType,
    kInvalidToken,
    kDatabaseError,
};

constexpr std::string_view errorCodeToString(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::kOk:                       return "OK";
        case ErrorCode::kInvalidInput:             return "INVALID_INPUT";
        case ErrorCode::kInvalidCredentials:       return "INVALID_CREDENTIALS";
        case ErrorCode::kUnauthorized:             return "UNAUTHORIZED";
        case ErrorCode::kForbidden:                return "FORBIDDEN";
        case ErrorCode::kNotFound:                 return "NOT_FOUND";
        case ErrorCode::kMethodNotAllowed:         return "METHOD_NOT_ALLOWED";
        case ErrorCode::kTooManyRequests:          return "TOO_MANY_REQUESTS";
        case ErrorCode::kRateLimitExceeded:        return "RATE_LIMIT_EXCEEDED";
        case ErrorCode::kServiceUnavailable:       return "SERVICE_UNAVAILABLE";
        case ErrorCode::kInternalError:            return "INTERNAL_ERROR";
        case ErrorCode::kCsrfTokenMismatch:        return "CSRF_TOKEN_MISMATCH";
        case ErrorCode::kSessionExpired:           return "SESSION_EXPIRED";
        case ErrorCode::kUserAlreadyExists:        return "USER_ALREADY_EXISTS";
        case ErrorCode::kUserNotFound:             return "USER_NOT_FOUND";
        case ErrorCode::kCapacityExceeded:         return "CAPACITY_EXCEEDED";
        case ErrorCode::kInvalidWebSocketUpgrade:  return "INVALID_WEBSOCKET_UPGRADE";
        case ErrorCode::kInvalidStreamType:        return "INVALID_STREAM_TYPE";
        case ErrorCode::kInvalidToken:             return "INVALID_TOKEN";
        case ErrorCode::kDatabaseError:            return "DATABASE_ERROR";
        default:                                   return "UNKNOWN_ERROR";
    }
}

constexpr int errorCodeToHttpStatus(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::kOk:                       return 200;
        case ErrorCode::kInvalidInput:             return 400;
        case ErrorCode::kCsrfTokenMismatch:        return 400;
        case ErrorCode::kInvalidCredentials:       return 401;
        case ErrorCode::kUnauthorized:             return 401;
        case ErrorCode::kSessionExpired:           return 401;
        case ErrorCode::kInvalidToken:             return 401;
        case ErrorCode::kForbidden:                return 403;
        case ErrorCode::kNotFound:                 return 404;
        case ErrorCode::kMethodNotAllowed:         return 405;
        case ErrorCode::kUserAlreadyExists:        return 409;
        case ErrorCode::kTooManyRequests:          return 429;
        case ErrorCode::kRateLimitExceeded:        return 429;
        case ErrorCode::kCapacityExceeded:         return 503;
        case ErrorCode::kServiceUnavailable:       return 503;
        case ErrorCode::kInvalidWebSocketUpgrade:  return 400;
        case ErrorCode::kInvalidStreamType:        return 400;
        case ErrorCode::kInternalError:            return 500;
        case ErrorCode::kDatabaseError:            return 500;
        default:                                   return 500;
    }
}

class ApiError {
public:
    explicit ApiError(ErrorCode code)
        : code_(code)
        , message_(std::string(errorCodeToString(code))) {}

    ApiError(ErrorCode code, std::string message)
        : code_(code)
        , message_(std::move(message)) {}

    [[nodiscard]] ErrorCode code() const noexcept { return code_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] int httpStatus() const noexcept { return errorCodeToHttpStatus(code_); }

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"error", {
                {"code", std::string(errorCodeToString(code_))},
                {"message", message_}
            }}
        };
    }

    [[nodiscard]] std::string toJsonString() const {
        return toJson().dump();
    }

private:
    ErrorCode code_;
    std::string message_;
};

} // namespace common
