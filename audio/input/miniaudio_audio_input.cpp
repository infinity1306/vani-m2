#include "miniaudio_audio_input.hpp"
#include <chrono>
#include <cstring>
#include <iostream>

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_DECODE
#define MA_NO_ENCODE
#include "../miniaudio/miniaudio.h"

namespace vani::audio {

static void ma_capture_data_callback(
    ma_device* pDevice,
    void* /*pOutput*/,
    const void* pInput,
    ma_uint32 frameCount
) {
    if (!pDevice || !pInput || frameCount == 0) return;

    auto* input_backend = static_cast<MiniaudioAudioInput*>(pDevice->pUserData);
    if (!input_backend || !input_backend->is_capturing()) return;

    const float* float_samples = static_cast<const float*>(pInput);
    input_backend->on_audio_data(float_samples, frameCount);
}

MiniaudioAudioInput::MiniaudioAudioInput(AudioFormat format, size_t ring_buffer_capacity)
    : format_(format), ring_buffer_(ring_buffer_capacity) {}

MiniaudioAudioInput::~MiniaudioAudioInput() {
    stop();
}

contracts::Result<void> MiniaudioAudioInput::start() {
    if (is_capturing_.load(std::memory_order_acquire)) {
        return contracts::Result<void>::ok();
    }

    auto* device = new ma_device();
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.format = ma_format_f32;
    deviceConfig.capture.channels = format_.channels;
    // Route empirically validated microphone array channel (Channel 1) directly without stereo downmix phase cancellation
    ma_channel captureChannelMap[1] = { MA_CHANNEL_FRONT_RIGHT };
    if (format_.channels == 1) {
        deviceConfig.capture.pChannelMap = captureChannelMap;
    }
    deviceConfig.capture.channelMixMode = ma_channel_mix_mode_simple;
    deviceConfig.wasapi.noAutoConvertSRC = MA_FALSE;
    deviceConfig.wasapi.noDefaultQualitySRC = MA_FALSE;
    deviceConfig.sampleRate = format_.sample_rate;
    deviceConfig.dataCallback = ma_capture_data_callback;
    deviceConfig.pUserData = this;

    ma_result result = ma_device_init(nullptr, &deviceConfig, device);
    if (result != MA_SUCCESS) {
        delete device;
        return contracts::Result<void>::err(
            contracts::ErrorCode::HardwareError,
            "Failed to initialize audio capture device via miniaudio/WASAPI (code: " + std::to_string(result) + ")",
            "vani.audio.miniaudio"
        );
    }

    result = ma_device_start(device);
    if (result != MA_SUCCESS) {
        ma_device_uninit(device);
        delete device;
        return contracts::Result<void>::err(
            contracts::ErrorCode::HardwareError,
            "Failed to start audio capture stream (code: " + std::to_string(result) + ")",
            "vani.audio.miniaudio"
        );
    }

    device_handle_ = device;
    if (device->capture.name[0] != '\0') {
        device_name_ = device->capture.name;
    }

    is_capturing_.store(true, std::memory_order_release);
    return contracts::Result<void>::ok();
}

contracts::Result<void> MiniaudioAudioInput::stop() {
    if (!is_capturing_.exchange(false, std::memory_order_acq_rel)) {
        return contracts::Result<void>::ok();
    }

    if (device_handle_) {
        auto* device = static_cast<ma_device*>(device_handle_);
        ma_device_stop(device);
        ma_device_uninit(device);
        delete device;
        device_handle_ = nullptr;
    }

    return contracts::Result<void>::ok();
}

contracts::Result<void> MiniaudioAudioInput::set_callback(AudioCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    user_callback_ = std::move(callback);
    return contracts::Result<void>::ok();
}

AudioFormat MiniaudioAudioInput::format() const {
    return format_;
}

bool MiniaudioAudioInput::is_capturing() const {
    return is_capturing_.load(std::memory_order_acquire);
}

std::string MiniaudioAudioInput::device_name() const {
    return device_name_;
}

void MiniaudioAudioInput::on_audio_data(const float* samples, size_t frame_count) {
    if (!samples || frame_count == 0) return;

    size_t total_samples = frame_count * format_.channels;
    std::span<const float> span_samples(samples, total_samples);

    // 1. Thread-safe push to underlying RingBuffer
    ring_buffer_.write(span_samples);

    // 2. Deliver frame to user callback if registered
    uint64_t frame_index = total_frames_captured_.fetch_add(1, std::memory_order_relaxed);
    uint64_t now_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );

    AudioFrame frame_meta{
        .format = format_,
        .timestamp_ns = now_ns,
        .sequence_number = frame_index
    };

    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (user_callback_) {
        user_callback_(span_samples, frame_meta);
    }
}

} // namespace vani::audio
