cmake_minimum_required(VERSION 3.16)
project(Avenger VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_package(Boost 1.82 REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(SODIUM REQUIRED libsodium)

add_executable(avenger
    src/main.cpp
    src/common/logger.cpp
    src/session/token_generator.cpp
    src/session/session_store.cpp
    src/auth/password_hasher.cpp
    src/auth/user_store.cpp
    src/auth/fail_delay.cpp
    src/middleware/auth_middleware.cpp
    src/middleware/csrf_middleware.cpp
    src/middleware/rate_limit_middleware.cpp
    src/middleware/cors_middleware.cpp
    src/middleware/logging_middleware.cpp
    src/routing/trie.cpp
    src/routing/router.cpp
    src/api/login_handler.cpp
    src/api/logout_handler.cpp
    src/api/static_handler.cpp
    src/streaming/stream_manager.cpp
    src/streaming/websocket_handler.cpp
    src/server/connection.cpp
    src/server/http_server.cpp
    src/app/config_manager.cpp
    src/app/app.cpp
)

target_include_directories(avenger PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLITE3_INCLUDE_DIRS}
    ${SODIUM_INCLUDE_DIRS}
)

target_link_libraries(avenger PRIVATE
    Threads::Threads
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    fmt::fmt
    ${SQLITE3_LIBRARIES}
    ${SODIUM_LIBRARIES}
)

target_compile_options(avenger PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(avenger PRIVATE -O2 -DNDEBUG)
else()
    target_compile_options(avenger PRIVATE -g -O0)
endif()
