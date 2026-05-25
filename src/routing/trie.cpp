#include "routing/trie.hpp"
#include <sstream>

namespace routing {

std::vector<std::string> Trie::split(const std::string& path) {
    std::vector<std::string> segments;
    std::istringstream ss(path);
    std::string segment;
    while (std::getline(ss, segment, '/')) {
        if (!segment.empty()) segments.push_back(segment);
    }
    return segments;
}

void Trie::insert(const std::string& pattern, HandlerIndex index) {
    auto segments = split(pattern);
    Node* current = &root_;

    for (size_t i = 0; i < segments.size(); ++i) {
        const auto& seg = segments[i];

        if (seg == "#") {
            // Wildcard: matches all remaining segments.
            if (!current->children.count("#")) {
                current->children["#"] = std::make_unique<Node>();
            }
            current->children["#"]->isWildcard    = true;
            current->children["#"]->handlerIndex  = index;
            return;
        }

        if (!current->children.count(seg)) {
            current->children[seg] = std::make_unique<Node>();
        }
        current = current->children[seg].get();
    }

    current->handlerIndex = index;
}

std::optional<Trie::HandlerIndex> Trie::match(const std::string& path) const noexcept {
    auto segments = split(path);
    const Node* current = &root_;

    for (const auto& seg : segments) {
        auto it = current->children.find(seg);
        if (it != current->children.end()) {
            current = it->second.get();
            continue;
        }
        // Try wildcard child.
        auto wild = current->children.find("#");
        if (wild != current->children.end() && wild->second->isWildcard) {
            return wild->second->handlerIndex;
        }
        return std::nullopt;
    }

    // Check for a trailing wildcard that accepts the full path.
    auto wild = current->children.find("#");
    if (wild != current->children.end()) {
        return wild->second->handlerIndex;
    }

    return current->handlerIndex;
}

} // namespace routing
