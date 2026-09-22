#pragma once

#include "../../contracts/providers/stt_engine.hpp"
#include "../../contracts/common/result.hpp"
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

namespace vani::voice::stt {

class STTManager {
public:
    STTManager();
    ~STTManager() = default;

    void register_engine(contracts::STTEnginePtr engine, bool is_primary = false);
    void set_fallback_engine(contracts::STTEnginePtr engine);

    contracts::Result<void> start_stream(
        const contracts::STTConfig& config,
        contracts::TranscriptCallback callback
    );

    contracts::Result<void> push_audio(std::span<const float> samples);
    contracts::Result<void> stop_stream();

    [[nodiscard]] contracts::STTEnginePtr active_engine() const;
    [[nodiscard]] std::string active_engine_name() const;
    [[nodiscard]] size_t registered_engine_count() const;
    [[nodiscard]] uint64_t fallback_switch_count() const;

private:
    contracts::STTEnginePtr select_healthy_engine();

    std::vector<contracts::STTEnginePtr> engines_;
    contracts::STTEnginePtr primary_engine_;
    contracts::STTEnginePtr fallback_engine_;
    contracts::STTEnginePtr current_active_;

    mutable std::mutex mutex_;
    contracts::STTConfig current_config_;
    contracts::TranscriptCallback current_callback_;
    uint64_t fallback_switches_{0};
};

} // namespace vani::voice::stt
