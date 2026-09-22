#pragma once

#include "../../contracts/providers/tts_engine.hpp"
#include <string>
#include <memory>

namespace vani::voice::tts {

enum class TTSProvider {
    PiperEnglish,
    PiperHindi,
    MatchaEnglish,
    SAPIBaseline,
    Mock
};

struct TTSFactoryConfig {
    TTSProvider provider{TTSProvider::PiperEnglish};
    std::string models_root{"models/tts"};
    int32_t num_threads{2};
    float speed{1.0f};
};

class TTSEngineFactory {
public:
    static contracts::TTSEnginePtr create_engine(const TTSFactoryConfig& config);
    static contracts::TTSEnginePtr create_piper_english(const std::string& models_root = "models/tts");
    static contracts::TTSEnginePtr create_piper_hindi(const std::string& models_root = "models/tts");
    static contracts::TTSEnginePtr create_vits_ljs(const std::string& models_root = "models/tts");
    static contracts::TTSEnginePtr create_matcha_english(const std::string& models_root = "models/tts");
    static contracts::TTSEnginePtr create_sapi_baseline();
    static contracts::TTSEnginePtr create_mock_engine();
};

} // namespace vani::voice::tts
