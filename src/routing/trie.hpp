#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace routing {

// Segment-based trie for URL pattern matching.
// Patterns: "/api/v1/login" — exact; "/ui/#" — '#' matches any remaining path.
class Trie {
public:
    using HandlerIndex = size_t;

    void insert(const std::string& pattern, HandlerIndex index);

    [[nodiscard]] std::optional<HandlerIndex> match(const std::string& path) const noexcept;

private:
    struct Node {
        std::map<std::string, std::unique_ptr<Node>> children;
        std::optional<HandlerIndex> handlerIndex;
        bool isWildcard{false};
    };

    [[nodiscard]] static std::vector<std::string> split(const std::string& path);

    Node root_;
};

} // namespace routing
