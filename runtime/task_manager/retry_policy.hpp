#pragma once

#include "../../contracts/common/error_code.hpp"
#include <string_view>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace vani::runtime {

enum class BackoffStrategy : uint8_t {
    Fixed,
    Linear,
    Exponential
};

struct RetryPolicy {
    uint32_t max_attempts{3};
    uint32_t initial_delay_ms{500};
    uint32_t max_delay_ms{10000};
    BackoffStrategy strategy{BackoffStrategy::Exponential};

    [[nodiscard]] bool can_retry(uint32_t current_attempts, contracts::ErrorCode code, contracts::ErrorCategory category) const noexcept {
        if (current_attempts >= max_attempts) return false;

        // Never retry permission, validation, or security violations
        if (code == contracts::ErrorCode::PermissionDenied ||
            code == contracts::ErrorCode::ValidationError ||
            code == contracts::ErrorCode::SecurityViolation ||
            code == contracts::ErrorCode::Cancelled) {
            return false;
        }

        if (category == contracts::ErrorCategory::Permission ||
            category == contracts::ErrorCategory::Validation ||
            category == contracts::ErrorCategory::Security ||
            category == contracts::ErrorCategory::Cancelled) {
            return false;
        }

        return true;
    }

    [[nodiscard]] uint32_t compute_delay_ms(uint32_t attempt) const noexcept {
        if (attempt <= 1) return initial_delay_ms;

        switch (strategy) {
            case BackoffStrategy::Fixed:
                return initial_delay_ms;
            case BackoffStrategy::Linear:
                return std::min(initial_delay_ms * attempt, max_delay_ms);
            case BackoffStrategy::Exponential:
                return std::min(initial_delay_ms * static_cast<uint32_t>(std::pow(2, attempt - 1)), max_delay_ms);
            default:
                return initial_delay_ms;
        }
    }
};

} // namespace vani::runtime
