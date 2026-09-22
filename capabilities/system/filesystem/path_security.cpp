#include "path_security.hpp"
#include <algorithm>
#include <sstream>

namespace vani::capabilities::system {

std::vector<std::string> PathSecurity::sensitive_locations_ = {
    // Windows Sensitive Locations
    "c:/windows",
    "c:/windows/system32",
    "c:/program files",
    "c:/program files (x86)",
    "c:/users/default",
    // Linux/Unix Sensitive Locations
    "/etc",
    "/etc/shadow",
    "/etc/passwd",
    "/etc/sudoers",
    "/proc",
    "/sys",
    "/dev",
    "/root",
    "/bin",
    "/sbin",
    "/usr/bin",
    "/usr/sbin",
    // Credential Stores & Secrets
    ".ssh",
    ".aws",
    ".gnupg",
    ".azure",
    ".kube",
    "id_rsa",
    "id_ed25519",
    "credentials",
    "secrets",
    ".env"
};

std::string PathSecurity::canonicalize_path(std::string_view raw_path) {
    if (raw_path.empty()) {
        return ".";
    }

    // Convert all backslashes to forward slashes and collect segments
    std::string normalized;
    normalized.reserve(raw_path.size());
    for (char c : raw_path) {
        if (c == '\\') {
            normalized.push_back('/');
        } else {
            normalized.push_back(c);
        }
    }

    bool is_absolute = (!normalized.empty() && normalized[0] == '/') ||
                       (normalized.size() > 1 && normalized[1] == ':');

    std::vector<std::string> segments;
    std::stringstream ss(normalized);
    std::string segment;

    while (std::getline(ss, segment, '/')) {
        if (segment.empty() || segment == ".") {
            continue;
        }
        if (segment == "..") {
            if (!segments.empty() && segments.back() != "..") {
                segments.pop_back();
            } else if (!is_absolute) {
                segments.push_back("..");
            }
        } else {
            segments.push_back(segment);
        }
    }

    std::string result;
    if (is_absolute && normalized[0] == '/') {
        result.push_back('/');
    }
    for (size_t i = 0; i < segments.size(); ++i) {
        result += segments[i];
        if (i + 1 < segments.size()) {
            result.push_back('/');
        }
    }

    return result.empty() ? (is_absolute ? "/" : ".") : result;
}

bool PathSecurity::is_traversal_attack(std::string_view raw_path) {
    if (raw_path.find("../") != std::string_view::npos ||
        raw_path.find("..\\") != std::string_view::npos ||
        raw_path.find("/..") != std::string_view::npos ||
        raw_path.find("\\..") != std::string_view::npos ||
        raw_path == "..") {
        return true;
    }
    return false;
}

bool PathSecurity::is_sensitive_location(std::string_view canonical_path) {
    std::string lower_path;
    lower_path.reserve(canonical_path.size());
    for (char c : canonical_path) {
        lower_path.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    for (const auto& sensitive : sensitive_locations_) {
        std::string lower_sensitive;
        lower_sensitive.reserve(sensitive.size());
        for (char c : sensitive) {
            lower_sensitive.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }

        // Check prefix match or substring match for secret stores
        if (lower_path.find(lower_sensitive) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool PathSecurity::is_within_scope(std::string_view target_path, std::string_view authorized_scope) {
    std::string c_target = canonicalize_path(target_path);
    std::string c_scope = canonicalize_path(authorized_scope);

    // Normalize lowercase for comparison
    std::string l_target;
    l_target.reserve(c_target.size());
    for (char c : c_target) l_target.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

    std::string l_scope;
    l_scope.reserve(c_scope.size());
    for (char c : c_scope) l_scope.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

    if (l_scope == "*" || l_scope == "all" || l_scope.empty()) {
        return true;
    }

    if (l_target.rfind(l_scope, 0) == 0) {
        return true;
    }

    return false;
}

void PathSecurity::add_custom_sensitive_location(const std::string& path) {
    sensitive_locations_.push_back(path);
}

} // namespace vani::capabilities::system
