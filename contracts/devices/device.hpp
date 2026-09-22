#pragma once

#include "../common/version.hpp"
#include "../common/result.hpp"
#include <string>
#include <vector>
#include <memory>

namespace vani::contracts {

enum class DeviceType : uint8_t {
    Desktop,
    Mobile,
    Audio,
    Wearable,
    SmartDisplay,
    Vehicle,
    Custom
};

struct DeviceManifest {
    DeviceId id;
    std::string name;
    DeviceType type{DeviceType::Desktop};
    std::string hardware_info;
    bool has_microphone{false};
    bool has_camera{false};
    bool has_screen{true};
    bool has_speaker{true};
    std::vector<std::string> supported_capabilities;
};

class Device {
public:
    virtual ~Device() = default;

    [[nodiscard]] virtual DeviceManifest manifest() const = 0;

    [[nodiscard]] virtual bool is_connected() const noexcept = 0;

    virtual Result<void> send_command(const std::string& command_json) = 0;
};

using DevicePtr = std::shared_ptr<Device>;

} // namespace vani::contracts
