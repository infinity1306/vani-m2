#pragma once

#include <string_view>
#include <cstdint>

namespace vani::contracts {

enum class RiskLevel : uint8_t {
    Safe = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Critical = 4
};

[[nodiscard]] constexpr std::string_view to_string(RiskLevel risk) noexcept {
    switch (risk) {
        case RiskLevel::Safe: return "SAFE";
        case RiskLevel::Low: return "LOW";
        case RiskLevel::Medium: return "MEDIUM";
        case RiskLevel::High: return "HIGH";
        case RiskLevel::Critical: return "CRITICAL";
        default: return "MEDIUM";
    }
}

} // namespace vani::contracts
