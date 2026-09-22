#pragma once

#include "../common/version.hpp"
#include "../tools/risk_level.hpp"
#include <string>
#include <vector>
#include <string_view>
#include <cstdint>
#include <functional>

namespace vani::contracts {

enum class CapabilityType : uint8_t {
    Agent,
    Tool,
    Model,
    Provider,
    Device,
    Integration,
    System
};

enum class CapabilityAvailability : uint8_t {
    Available,
    Unavailable,
    Disabled,
    RequiresPermission
};

enum class Reversibility : uint8_t {
    Reversible,
    PartiallyReversible,
    Irreversible
};

enum class Idempotency : uint8_t {
    Idempotent,
    NonIdempotent
};

enum class PlatformId : uint8_t {
    Windows,
    Linux,
    MacOS,
    Universal
};

struct ResourceRequirements {
    uint32_t max_cpu_percent{100};
    uint64_t max_memory_bytes{512 * 1024 * 1024}; // 512 MB default
    uint32_t timeout_ms{30000};                   // 30 seconds default
    uint64_t max_output_bytes{1024 * 1024};       // 1 MB default
    uint64_t max_file_size_bytes{100 * 1024 * 1024}; // 100 MB default
    uint32_t max_process_count{16};
};

struct CapabilityDescriptorExtended {
    CapabilityId id;
    SemanticVersion version{1, 0, 0};
    CapabilityType type{CapabilityType::Tool};
    std::string provider_id;
    std::string description;
    std::string input_schema_json;
    std::string output_schema_json;
    std::vector<std::string> required_permissions;
    RiskLevel risk_level{RiskLevel::Low};
    CapabilityAvailability availability{CapabilityAvailability::Available};
    Reversibility reversibility{Reversibility::Reversible};
    Idempotency idempotency{Idempotency::Idempotent};
    ResourceRequirements resource_requirements{};
    std::string expected_postcondition;
    bool requires_network{false};
    bool requires_gpu{false};
    bool requires_camera{false};
    bool requires_microphone{false};
    bool supports_streaming{false};
    bool supports_cancellation{true};
    bool supports_resume{false};
};

[[nodiscard]] constexpr std::string_view to_string(Reversibility r) noexcept {
    switch (r) {
        case Reversibility::Reversible: return "REVERSIBLE";
        case Reversibility::PartiallyReversible: return "PARTIALLY_REVERSIBLE";
        case Reversibility::Irreversible: return "IRREVERSIBLE";
        default: return "IRREVERSIBLE";
    }
}

[[nodiscard]] constexpr std::string_view to_string(Idempotency i) noexcept {
    switch (i) {
        case Idempotency::Idempotent: return "IDEMPOTENT";
        case Idempotency::NonIdempotent: return "NON_IDEMPOTENT";
        default: return "NON_IDEMPOTENT";
    }
}

[[nodiscard]] constexpr std::string_view to_string(PlatformId p) noexcept {
    switch (p) {
        case PlatformId::Windows: return "WINDOWS";
        case PlatformId::Linux: return "LINUX";
        case PlatformId::MacOS: return "MACOS";
        case PlatformId::Universal: return "UNIVERSAL";
        default: return "UNIVERSAL";
    }
}

} // namespace vani::contracts
