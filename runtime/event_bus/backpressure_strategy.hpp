#pragma once

#include <string_view>
#include <cstdint>

namespace vani::runtime {

enum class BackpressureStrategy : uint8_t {
    DropOldest,
    DropNewest,
    Coalesce,
    Block,
    Persist,
    Fail
};

[[nodiscard]] constexpr std::string_view to_string(BackpressureStrategy s) noexcept {
    switch (s) {
        case BackpressureStrategy::DropOldest: return "DROP_OLDEST";
        case BackpressureStrategy::DropNewest: return "DROP_NEWEST";
        case BackpressureStrategy::Coalesce: return "COALESCE";
        case BackpressureStrategy::Block: return "BLOCK";
        case BackpressureStrategy::Persist: return "PERSIST";
        case BackpressureStrategy::Fail: return "FAIL";
        default: return "DROP_OLDEST";
    }
}

} // namespace vani::runtime
