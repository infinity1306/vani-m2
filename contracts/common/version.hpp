#pragma once

#include <string>
#include <cstdint>

namespace vani::contracts {

struct SemanticVersion {
    uint32_t major{1};
    uint32_t minor{0};
    uint32_t patch{0};

    bool operator==(const SemanticVersion& other) const noexcept {
        return major == other.major && minor == other.minor && patch == other.patch;
    }

    bool operator<(const SemanticVersion& other) const noexcept {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }

    bool operator<=(const SemanticVersion& other) const noexcept {
        return *this < other || *this == other;
    }

    bool operator>(const SemanticVersion& other) const noexcept {
        return !(*this <= other);
    }

    bool operator>=(const SemanticVersion& other) const noexcept {
        return !(*this < other);
    }

    [[nodiscard]] std::string to_string() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }
};

using TaskId = std::string;
using SessionId = std::string;
using AgentId = std::string;
using ToolId = std::string;
using ModelId = std::string;
using DeviceId = std::string;
using CapabilityId = std::string;
using CorrelationId = std::string;
using EventId = std::string;

} // namespace vani::contracts
