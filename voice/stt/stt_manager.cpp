#include "stt_manager.hpp"

namespace vani::voice::stt {

STTManager::STTManager() = default;

void STTManager::register_engine(contracts::STTEnginePtr engine, bool is_primary) {
    if (!engine) return;
    std::lock_guard<std::mutex> lock(mutex_);
    engines_.push_back(engine);
    if (is_primary || !primary_engine_) {
        primary_engine_ = engine;
    }
}

void STTManager::set_fallback_engine(contracts::STTEnginePtr engine) {
    std::lock_guard<std::mutex> lock(mutex_);
    fallback_engine_ = engine;
}

contracts::STTEnginePtr STTManager::select_healthy_engine() {
    if (primary_engine_ && primary_engine_->is_healthy()) {
        return primary_engine_;
    }

    for (const auto& eng : engines_) {
        if (eng && eng->is_healthy()) {
            if (eng != primary_engine_) {
                fallback_switches_++;
            }
            return eng;
        }
    }

    if (fallback_engine_ && fallback_engine_->is_healthy()) {
        fallback_switches_++;
        return fallback_engine_;
    }

    return nullptr;
}

contracts::Result<void> STTManager::start_stream(
    const contracts::STTConfig& config,
    contracts::TranscriptCallback callback
) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_config_ = config;
    current_callback_ = callback;

    current_active_ = select_healthy_engine();
    if (!current_active_) {
        return contracts::Fail(contracts::ErrorCode::ServiceUnavailable, "No healthy STT engine available");
    }

    return current_active_->start_stream(config, callback);
}

contracts::Result<void> STTManager::push_audio(std::span<const float> samples) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!current_active_) {
        return contracts::Fail(contracts::ErrorCode::InvalidState, "STT stream not active");
    }

    // Check if active engine failed mid-stream and needs fallback switch
    if (!current_active_->is_healthy()) {
        auto fallback = select_healthy_engine();
        if (fallback && fallback != current_active_) {
            current_active_->stop_stream();
            current_active_ = fallback;
            current_active_->start_stream(current_config_, current_callback_);
        } else {
            return contracts::Fail(contracts::ErrorCode::InternalError, "STT engine failed and no healthy fallback available");
        }
    }

    return current_active_->push_audio(samples);
}

contracts::Result<void> STTManager::stop_stream() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!current_active_) return contracts::Ok();

    auto res = current_active_->stop_stream();
    current_active_ = nullptr;
    return res;
}

contracts::STTEnginePtr STTManager::active_engine() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_active_ ? current_active_ : primary_engine_;
}

std::string STTManager::active_engine_name() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (current_active_) return current_active_->engine_name();
    if (primary_engine_) return primary_engine_->engine_name();
    return "None";
}

size_t STTManager::registered_engine_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return engines_.size();
}

uint64_t STTManager::fallback_switch_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return fallback_switches_;
}

} // namespace vani::voice::stt
