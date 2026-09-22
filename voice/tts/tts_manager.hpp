#pragma once

#include "../../contracts/providers/tts_engine.hpp"
#include "../../contracts/common/result.hpp"
#include <vector>
#include <memory>
#include <mutex>
#include <string>

namespace vani::voice::tts {

class TTSManager {
public:
    TTSManager();
    ~TTSManager() = default;

    void register_engine(contracts::TTSEnginePtr engine, bool is_primary = false);
    bool set_active_engine(const std::string& name);

    contracts::Result<std::vector<float>> synthesize(
        const std::string& text,
        const contracts::TTSConfig& config = {},
        contracts::CancellationToken cancellation_token = contracts::CancellationToken::none()
    );

    contracts::Result<void> synthesize_stream(
        const std::string& text,
        contracts::AudioChunkCallback chunk_cb,
        const contracts::TTSConfig& config = {},
        contracts::CancellationToken cancellation_token = contracts::CancellationToken::none()
    );

    [[nodiscard]] std::string active_engine_name() const;
    [[nodiscard]] size_t registered_engine_count() const;
    [[nodiscard]] contracts::TTSEnginePtr primary_engine() const;
    [[nodiscard]] contracts::TTSEnginePtr get_engine_for_text(const std::string& text) const;

private:
    std::vector<contracts::TTSEnginePtr> engines_;
    contracts::TTSEnginePtr primary_engine_;
    mutable std::mutex mutex_;
};

using TTSManagerPtr = std::shared_ptr<TTSManager>;

} // namespace vani::voice::tts
