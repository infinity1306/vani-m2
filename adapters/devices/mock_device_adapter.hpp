#pragma once

#include "../../contracts/devices/device.hpp"

namespace vani::adapters {

class MockDeviceAdapter : public contracts::Device {
public:
    explicit MockDeviceAdapter(contracts::DeviceId id = "dev_buds_01", std::string name = "VANI Spatial Buds Pro")
        : id_(std::move(id)), name_(std::move(name)) {}

    [[nodiscard]] contracts::DeviceManifest manifest() const override {
        return contracts::DeviceManifest{
            .id = id_,
            .name = name_,
            .type = contracts::DeviceType::Audio,
            .hardware_info = "Dual beamforming mics, LE Audio 24kHz",
            .has_microphone = true,
            .has_camera = false,
            .has_screen = false,
            .has_speaker = true,
            .supported_capabilities = {"audio.capture", "audio.playback"}
        };
    }

    [[nodiscard]] bool is_connected() const noexcept override {
        return true;
    }

    contracts::Result<void> send_command(const std::string& /*command_json*/) override {
        return contracts::Result<void>::ok();
    }

private:
    contracts::DeviceId id_;
    std::string name_;
};

} // namespace vani::adapters
