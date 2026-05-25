#include "auth/user_store.hpp"
#include "auth/password_hasher.hpp"
#include "common/logger.hpp"

#include <sqlite3.h>
#include <stdexcept>

namespace auth {

static constexpr auto kLog = "auth";

namespace {

session::Role roleFromInt(int v) noexcept {
    switch (v) {
        case 3:  return session::Role::kAdmin;
        case 2:  return session::Role::kOperator;
        case 1:  return session::Role::kViewer;
        default: return session::Role::kViewer;
    }
}

int roleToInt(session::Role r) noexcept {
    return static_cast<int>(r);
}

} // anonymous namespace

UserStore::UserStore(const std::string& dbPath) {
    open(dbPath);
    migrate();
    if (isEmpty()) {
        seedDefaultAdmin();
    }
}

UserStore::~UserStore() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void UserStore::checkRc(int rc, const char* context) {
    if (rc != SQLITE_OK && rc != SQLITE_DONE && rc != SQLITE_ROW) {
        throw std::runtime_error(std::string("SQLite error at ") + context);
    }
}

void UserStore::open(const std::string& dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    checkRc(rc, "open");
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    LOG_INFO(common::Logger::get(kLog), "User database opened: {}", dbPath);
}

void UserStore::migrate() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  username     TEXT PRIMARY KEY NOT NULL,"
        "  password_hash TEXT NOT NULL,"
        "  display_name TEXT NOT NULL,"
        "  role         INTEGER NOT NULL DEFAULT 1"
        ");";
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);
    checkRc(rc, "migrate");
}

void UserStore::seedDefaultAdmin() {
    LOG_WARN(common::Logger::get(kLog), "No users found — seeding default admin account");
    UserModel admin;
    admin.username     = "admin";
    admin.displayName  = "Administrator";
    admin.role         = session::Role::kAdmin;
    admin.passwordHash = PasswordHasher::hash("admin");
    auto rc = createUser(admin);
    if (rc != common::ErrorCode::kOk) {
        throw std::runtime_error("Failed to seed default admin user");
    }
}

std::optional<UserModel> UserStore::findUser(const std::string& username) const {
    const char* sql = "SELECT username, password_hash, display_name, role FROM users WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    std::optional<UserModel> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        UserModel u;
        u.username     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        u.passwordHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.displayName  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.role         = roleFromInt(sqlite3_column_int(stmt, 3));
        result         = std::move(u);
    }
    sqlite3_finalize(stmt);
    return result;
}

common::ErrorCode UserStore::createUser(const UserModel& user) {
    const char* sql =
        "INSERT INTO users (username, password_hash, display_name, role) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user.passwordHash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user.displayName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, roleToInt(user.role));

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_CONSTRAINT) {
        return common::ErrorCode::kUserAlreadyExists;
    }
    if (rc != SQLITE_DONE) {
        LOG_ERROR(common::Logger::get(kLog), "createUser failed for '{}'", user.username);
        return common::ErrorCode::kDatabaseError;
    }
    LOG_INFO(common::Logger::get(kLog), "User created: '{}'", user.username);
    return common::ErrorCode::kOk;
}

common::ErrorCode UserStore::deleteUser(const std::string& username) {
    const char* sql = "DELETE FROM users WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (sqlite3_changes(db_) == 0) {
        return common::ErrorCode::kUserNotFound;
    }
    if (rc != SQLITE_DONE) {
        return common::ErrorCode::kDatabaseError;
    }
    LOG_INFO(common::Logger::get(kLog), "User deleted: '{}'", username);
    return common::ErrorCode::kOk;
}

std::vector<UserModel> UserStore::listUsers() const {
    const char* sql = "SELECT username, password_hash, display_name, role FROM users;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    std::vector<UserModel> users;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        UserModel u;
        u.username     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        u.passwordHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.displayName  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.role         = roleFromInt(sqlite3_column_int(stmt, 3));
        users.push_back(std::move(u));
    }
    sqlite3_finalize(stmt);
    return users;
}

bool UserStore::isEmpty() const {
    const char* sql = "SELECT COUNT(*) FROM users;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count == 0;
}

} // namespace auth
