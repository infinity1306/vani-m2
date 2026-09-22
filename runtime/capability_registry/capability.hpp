#pragma once

#include "../../contracts/common/version.hpp"
#include "../../contracts/tools/risk_level.hpp"
#include <string>
#include <vector>
#include <string_view>

namespace vani::runtime {

enum class CapabilityType : uint8_t {
    Agent,
    Tool,
    Model,
    Provider,
    Device,
    Integration
};

enum class CapabilityAvailability : uint8_t {
    Available,
    Unavailable,
    Disabled,
    RequiresPermission
};

[[nodiscard]] constexpr std::string_view to_string(CapabilityAvailability a) noexcept {
    switch (a) {
        case CapabilityAvailability::Available: return "AVAILABLE";
        case CapabilityAvailability::Unavailable: return "UNAVAILABLE";
        case CapabilityAvailability::Disabled: return "DISABLED";
        case CapabilityAvailability::RequiresPermission: return "REQUIRES_PERMISSION";
        default: return "UNAVAILABLE";
    }
}

struct CapabilityDescriptor {
    contracts::CapabilityId id;
    contracts::SemanticVersion version{1, 0, 0};
    CapabilityType type{CapabilityType::Tool};
    std::string provider_id;
    std::string description;
    std::string input_schema_json;
    std::string output_schema_json;
    std::vector<std::string> required_permissions;
    contracts::RiskLevel risk_level{contracts::RiskLevel::Low};
    CapabilityAvailability availability{CapabilityAvailability::Available};
    bool requires_network{false};
    bool requires_gpu{false};
    bool requires_camera{false};
    bool requires_microphone{false};
    bool supports_streaming{false};
    bool supports_cancellation{true};
    bool supports_resume{false};
};

} // namespace vani::runtime
