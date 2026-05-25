#pragma once

#include "session/session.hpp"
#include "common/error.hpp"
#include <optional>
#include <string>
#include <vector>

struct sqlite3;

namespace auth {

struct UserModel {
    std::string username;
    std::string passwordHash;
    std::string displayName;
    session::Role role{session::Role::kViewer};
};

class UserStore {
public:
    explicit UserStore(const std::string& dbPath);
    ~UserStore();
    UserStore(const UserStore&) = delete;
    UserStore& operator=(const UserStore&) = delete;

    [[nodiscard]] std::optional<UserModel> findUser(const std::string& username) const;
    [[nodiscard]] common::ErrorCode createUser(const UserModel& user);
    [[nodiscard]] common::ErrorCode deleteUser(const std::string& username);
    [[nodiscard]] std::vector<UserModel> listUsers() const;
    [[nodiscard]] bool isEmpty() const;

private:
    void open(const std::string& dbPath);
    void migrate();
    void seedDefaultAdmin();
    static void checkRc(int rc, const char* context);

    sqlite3* db_{nullptr};
};

} // namespace auth
