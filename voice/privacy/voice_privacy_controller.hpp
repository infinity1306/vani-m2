#pragma once

#include <cstdint>
#include <mutex>
#include <string>

namespace vani::voice::privacy {

enum class MicrophonePrivacyMode : uint8_t {
    On,
    Off,
    SessionOnly
};

enum class CloudVoicePolicy : uint8_t {
    Allowed,
    LocalOnly
};

struct VoicePrivacySettings {
    MicrophonePrivacyMode mic_mode{MicrophonePrivacyMode::SessionOnly};
    bool wakeword_enabled{true};
    CloudVoicePolicy cloud_policy{CloudVoicePolicy::LocalOnly};
    bool ephemeral_audio_only{true}; // Never persist raw microphone buffer
    bool retain_transcripts{true};
};

class VoicePrivacyController {
public:
    explicit VoicePrivacyController(VoicePrivacySettings settings = {})
        : settings_(settings) {}

    [[nodiscard]] VoicePrivacySettings settings() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return settings_;
    }

    void update_settings(const VoicePrivacySettings& settings) {
        std::lock_guard<std::mutex> lock(mutex_);
        settings_ = settings;
    }

    [[nodiscard]] bool can_record_audio() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return settings_.mic_mode != MicrophonePrivacyMode::Off;
    }

    [[nodiscard]] bool is_cloud_allowed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return settings_.cloud_policy == CloudVoicePolicy::Allowed;
    }

private:
    VoicePrivacySettings settings_;
    mutable std::mutex mutex_;
};

} // namespace vani::voice::privacy
