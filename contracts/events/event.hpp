#pragma once

#include "../common/version.hpp"
#include <string>
#include <chrono>
#include <memory>
#include <any>
#include <unordered_map>

namespace vani::contracts {

enum class EventCategory : uint8_t {
    System,
    Voice,
    Task,
    Agent,
    Tool,
    Security,
    Device,
    Perception,
    Memory,
    Custom
};

struct EventHeader {
    EventId event_id;
    std::string event_type;
    SemanticVersion event_version{1, 0, 0};
    uint64_t timestamp_ms{0};
    std::string source;
    SessionId session_id;
    TaskId task_id;
    CorrelationId correlation_id;
    EventCategory category{EventCategory::System};
};

class Event {
public:
    explicit Event(
        EventHeader header,
        std::unordered_map<std::string, std::string> attributes = {},
        std::any payload = {}
    ) : header_(std::move(header)), attributes_(std::move(attributes)), payload_(std::move(payload)) {
        if (header_.timestamp_ms == 0) {
            header_.timestamp_ms = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
            );
        }
    }

    [[nodiscard]] const EventHeader& header() const noexcept { return header_; }
    [[nodiscard]] const std::string& event_type() const noexcept { return header_.event_type; }
    [[nodiscard]] const EventId& id() const noexcept { return header_.event_id; }
    [[nodiscard]] const CorrelationId& correlation_id() const noexcept { return header_.correlation_id; }
    [[nodiscard]] const TaskId& task_id() const noexcept { return header_.task_id; }
    [[nodiscard]] const SessionId& session_id() const noexcept { return header_.session_id; }
    [[nodiscard]] EventCategory category() const noexcept { return header_.category; }

    [[nodiscard]] const std::unordered_map<std::string, std::string>& attributes() const noexcept {
        return attributes_;
    }

    template <typename T>
    [[nodiscard]] const T* payload_as() const noexcept {
        try {
            return std::any_cast<T>(&payload_);
        } catch (...) {
            return nullptr;
        }
    }

private:
    EventHeader header_;
    std::unordered_map<std::string, std::string> attributes_;
    std::any payload_;
};

using EventPtr = std::shared_ptr<const Event>;

} // namespace vani::contracts
