#pragma once

#include <string>
#include <vector>
#include <string_view>

namespace vani::capabilities::system {

class PathSecurity {
public:
    PathSecurity() = default;
    ~PathSecurity() = default;

    [[nodiscard]] static std::string canonicalize_path(std::string_view raw_path);
    [[nodiscard]] static bool is_traversal_attack(std::string_view raw_path);
    [[nodiscard]] static bool is_sensitive_location(std::string_view canonical_path);
    [[nodiscard]] static bool is_within_scope(std::string_view target_path, std::string_view authorized_scope);

    static void add_custom_sensitive_location(const std::string& path);

private:
    static std::vector<std::string> sensitive_locations_;
};

} // namespace vani::capabilities::system
