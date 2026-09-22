#pragma once

#include <string_view>
#include <cstdint>

namespace vani::runtime {

enum class DeliveryGuarantee : uint8_t {
    Ephemeral, // Can drop/coalesce under high load
    Durable,   // Must be recorded/persisted
    Critical   // Never dropped; immediate delivery
};

[[nodiscard]] constexpr std::string_view to_string(DeliveryGuarantee g) noexcept {
    switch (g) {
        case DeliveryGuarantee::Ephemeral: return "EPHEMERAL";
        case DeliveryGuarantee::Durable: return "DURABLE";
        case DeliveryGuarantee::Critical: return "CRITICAL";
        default: return "EPHEMERAL";
    }
}

} // namespace vani::runtime
