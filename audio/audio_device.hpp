#pragma once

#include "audio_format.hpp"
#include <string>
#include <vector>

namespace vani::audio {

enum class DeviceType : uint8_t {
    InputMicrophone,
    OutputSpeaker,
    BluetoothHeadset,
    VirtualLoopback
};

enum class DeviceState : uint8_t {
    Available,
    Active,
    Disconnected,
    Error
};

struct AudioDeviceInfo {
    std::string id;
    std::string name;
    DeviceType type{DeviceType::InputMicrophone};
    DeviceState state{DeviceState::Available};
    bool is_default{false};
    std::vector<uint32_t> supported_sample_rates{16000, 44100, 48000};
    uint8_t max_channels{1};
};

} // namespace vani::audio
