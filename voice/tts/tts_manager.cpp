#include "tts_manager.hpp"
#include <algorithm>

namespace vani::voice::tts {

TTSManager::TTSManager() = default;

void TTSManager::register_engine(contracts::TTSEnginePtr engine, bool is_primary) {
    if (!engine) return;
    std::lock_guard<std::mutex> lock(mutex_);
    engines_.push_back(engine);
    if (is_primary || !primary_engine_) {
        primary_engine_ = engine;
    }
}

bool TTSManager::set_active_engine(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& eng : engines_) {
        if (eng && eng->engine_name().find(name) != std::string::npos) {
            primary_engine_ = eng;
            return true;
        }
    }
    return false;
}

contracts::TTSEnginePtr TTSManager::get_engine_for_text(const std::string& text) const {
    // Check if text contains Devanagari UTF-8 bytes (0xE0 0xA4 .. 0xE0 0xA5)
    bool has_devanagari = false;
    for (size_t i = 0; i + 2 < text.size(); ++i) {
        unsigned char c1 = static_cast<unsigned char>(text[i]);
        unsigned char c2 = static_cast<unsigned char>(text[i + 1]);
        if (c1 == 0xE0 && (c2 == 0xA4 || c2 == 0xA5)) {
            has_devanagari = true;
            break;
        }
    }

    if (has_devanagari) {
        for (const auto& eng : engines_) {
            if (eng && (eng->engine_name().find("hi") != std::string::npos ||
                        eng->engine_name().find("Hindi") != std::string::npos ||
                        eng->engine_name().find("priyamvada") != std::string::npos)) {
                return eng;
            }
        }
    }

    return primary_engine_;
}

contracts::Result<std::vector<float>> TTSManager::synthesize(
    const std::string& text,
    const contracts::TTSConfig& config,
    contracts::CancellationToken cancellation_token
) {
    contracts::TTSEnginePtr engine;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (engines_.empty() && !primary_engine_) {
            return contracts::Fail(contracts::ErrorCode::ServiceUnavailable, "No TTS engine registered");
        }
        engine = get_engine_for_text(text);
        if (!engine) engine = primary_engine_;
    }

    if (!engine) {
        return contracts::Fail(contracts::ErrorCode::ServiceUnavailable, "No suitable TTS engine found");
    }

    return engine->synthesize(text, config, cancellation_token);
}

contracts::Result<void> TTSManager::synthesize_stream(
    const std::string& text,
    contracts::AudioChunkCallback chunk_cb,
    const contracts::TTSConfig& config,
    contracts::CancellationToken cancellation_token
) {
    contracts::TTSEnginePtr engine;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (engines_.empty() && !primary_engine_) {
            return contracts::Fail(contracts::ErrorCode::ServiceUnavailable, "No TTS engine registered");
        }
        engine = get_engine_for_text(text);
        if (!engine) engine = primary_engine_;
    }

    if (!engine) {
        return contracts::Fail(contracts::ErrorCode::ServiceUnavailable, "No suitable TTS engine found");
    }

    return engine->synthesize_stream(text, config, chunk_cb, cancellation_token);
}

std::string TTSManager::active_engine_name() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return primary_engine_ ? primary_engine_->engine_name() : "None";
}

size_t TTSManager::registered_engine_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return engines_.size();
}

contracts::TTSEnginePtr TTSManager::primary_engine() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return primary_engine_;
}

} // namespace vani::voice::tts
