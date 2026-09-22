#pragma once

#include <string_view>
#include <cstdint>

namespace vani::contracts {

enum class ErrorCode : uint32_t {
    Success = 0,
    Failure = 1,
    Timeout = 2,
    Cancelled = 3,
    PermissionDenied = 4,
    Unavailable = 5,
    Offline = 6,
    ValidationError = 7,
    DependencyError = 8,
    ResourceExhausted = 9,
    NotFound = 10,
    AlreadyExists = 11,
    NotImplemented = 12,
    SecurityViolation = 13,
    StorageError = 14,
    NetworkError = 15,
    InternalError = 16,
    InvalidState = 17,
    HardwareError = 18,
    ServiceUnavailable = 5
};

enum class ErrorCategory : uint8_t {
    Validation,
    Permission,
    Timeout,
    Cancelled,
    Unavailable,
    Resource,
    Dependency,
    Network,
    Storage,
    Internal,
    Security
};

enum class ErrorSeverity : uint8_t {
    Info,
    Warning,
    Error,
    Critical
};

[[nodiscard]] constexpr std::string_view to_string(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Success: return "SUCCESS";
        case ErrorCode::Failure: return "FAILURE";
        case ErrorCode::Timeout: return "TIMEOUT";
        case ErrorCode::Cancelled: return "CANCELLED";
        case ErrorCode::PermissionDenied: return "PERMISSION_DENIED";
        case ErrorCode::Unavailable: return "UNAVAILABLE";
        case ErrorCode::Offline: return "OFFLINE";
        case ErrorCode::ValidationError: return "VALIDATION_ERROR";
        case ErrorCode::DependencyError: return "DEPENDENCY_ERROR";
        case ErrorCode::ResourceExhausted: return "RESOURCE_EXHAUSTED";
        case ErrorCode::NotFound: return "NOT_FOUND";
        case ErrorCode::AlreadyExists: return "ALREADY_EXISTS";
        case ErrorCode::NotImplemented: return "NOT_IMPLEMENTED";
        case ErrorCode::SecurityViolation: return "SECURITY_VIOLATION";
        case ErrorCode::StorageError: return "STORAGE_ERROR";
        case ErrorCode::NetworkError: return "NETWORK_ERROR";
        case ErrorCode::InternalError: return "INTERNAL_ERROR";
        case ErrorCode::InvalidState: return "INVALID_STATE";
        case ErrorCode::HardwareError: return "HARDWARE_ERROR";
        default: return "UNKNOWN_ERROR";
    }
}

[[nodiscard]] constexpr std::string_view to_string(ErrorCategory category) noexcept {
    switch (category) {
        case ErrorCategory::Validation: return "VALIDATION";
        case ErrorCategory::Permission: return "PERMISSION";
        case ErrorCategory::Timeout: return "TIMEOUT";
        case ErrorCategory::Cancelled: return "CANCELLED";
        case ErrorCategory::Unavailable: return "UNAVAILABLE";
        case ErrorCategory::Resource: return "RESOURCE";
        case ErrorCategory::Dependency: return "DEPENDENCY";
        case ErrorCategory::Network: return "NETWORK";
        case ErrorCategory::Storage: return "STORAGE";
        case ErrorCategory::Internal: return "INTERNAL";
        case ErrorCategory::Security: return "SECURITY";
        default: return "INTERNAL";
    }
}

} // namespace vani::contracts
