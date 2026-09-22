#pragma once

#include <string_view>
#include <cstdint>

namespace vani::runtime {

enum class ContextScopeType : uint8_t {
    Request,
    Task,
    Session,
    User,
    Device,
    Project
};

[[nodiscard]] constexpr std::string_view to_string(ContextScopeType s) noexcept {
    switch (s) {
        case ContextScopeType::Request: return "REQUEST";
        case ContextScopeType::Task: return "TASK";
        case ContextScopeType::Session: return "SESSION";
        case ContextScopeType::User: return "USER";
        case ContextScopeType::Device: return "DEVICE";
        case ContextScopeType::Project: return "PROJECT";
        default: return "TASK";
    }
}

} // namespace vani::runtime
