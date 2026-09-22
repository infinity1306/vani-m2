#include "tts_engine_factory.hpp"
#include "../../adapters/tts/real_piper_tts_adapter.hpp"
#include "../../adapters/tts/real_matcha_tts_adapter.hpp"
#include "../../adapters/tts/windows_sapi_tts_adapter.hpp"
#include "../../adapters/tts/mock_tts_engine.hpp"
#include "../../observability/logger.hpp"
#include <filesystem>

namespace vani::voice::tts {

contracts::TTSEnginePtr TTSEngineFactory::create_piper_english(const std::string& models_root) {
    namespace fs = std::filesystem;
    fs::path base = fs::path(models_root) / "vits-piper-en_US-lessac-medium";
    adapters::tts::PiperTTSModelConfig cfg;
    cfg.model_path = (base / "en_US-lessac-medium.onnx").string();
    cfg.tokens_path = (base / "tokens.txt").string();
    cfg.data_dir = (base / "espeak-ng-data").string();
    cfg.voice_name = "en_US-lessac";
    cfg.language = "en";
    cfg.num_threads = 2;

    auto adapter = std::make_shared<adapters::tts::RealPiperTTSAdapter>(cfg);
    if (!adapter->is_healthy()) {
        observability::Logger::instance().warn("TTSEngineFactory", "Piper English adapter failed health check, falling back to mock");
        return std::make_shared<adapters::tts::MockTTSEngine>();
    }
    return adapter;
}

contracts::TTSEnginePtr TTSEngineFactory::create_piper_hindi(const std::string& models_root) {
    namespace fs = std::filesystem;
    fs::path base = fs::path(models_root) / "vits-piper-hi_IN-priyamvada-medium";
    adapters::tts::PiperTTSModelConfig cfg;
    cfg.model_path = (base / "hi_IN-priyamvada-medium.onnx").string();
    cfg.tokens_path = (base / "tokens.txt").string();
    cfg.data_dir = (base / "espeak-ng-data").string();
    cfg.voice_name = "hi_IN-priyamvada";
    cfg.language = "hi";
    cfg.num_threads = 2;

    auto adapter = std::make_shared<adapters::tts::RealPiperTTSAdapter>(cfg);
    if (!adapter->is_healthy()) {
        observability::Logger::instance().warn("TTSEngineFactory", "Piper Hindi adapter failed health check, falling back to mock");
        return std::make_shared<adapters::tts::MockTTSEngine>();
    }
    return adapter;
}

contracts::TTSEnginePtr TTSEngineFactory::create_vits_ljs(const std::string& models_root) {
    namespace fs = std::filesystem;
    fs::path base = fs::path(models_root) / "vits-ljs";
    adapters::tts::PiperTTSModelConfig cfg;
    cfg.model_path = (base / "vits-ljs.onnx").string();
    cfg.tokens_path = (base / "tokens.txt").string();
    cfg.lexicon_path = (base / "lexicon.txt").string();
    cfg.data_dir = ""; // Uses lexicon directly
    cfg.voice_name = "en_US-ljspeech-vits";
    cfg.language = "en";
    cfg.num_threads = 2;

    auto adapter = std::make_shared<adapters::tts::RealPiperTTSAdapter>(cfg);
    if (!adapter->is_healthy()) {
        observability::Logger::instance().warn("TTSEngineFactory", "VITS LJSpeech adapter failed health check, falling back to mock");
        return std::make_shared<adapters::tts::MockTTSEngine>();
    }
    return adapter;
}

contracts::TTSEnginePtr TTSEngineFactory::create_matcha_english(const std::string& models_root) {
    namespace fs = std::filesystem;
    fs::path base = fs::path(models_root) / "matcha-icefall-en_US-ljspeech";
    adapters::tts::MatchaTTSModelConfig cfg;
    cfg.acoustic_model_path = (base / "model-steps-3.onnx").string();
    cfg.tokens_path = (base / "tokens.txt").string();
    cfg.data_dir = (base / "espeak-ng-data").string();
    cfg.voice_name = "en_US-ljspeech";
    cfg.language = "en";
    cfg.num_threads = 2;

    auto adapter = std::make_shared<adapters::tts::RealMatchaTTSAdapter>(cfg);
    if (!adapter->is_healthy()) {
        observability::Logger::instance().warn("TTSEngineFactory", "Matcha English adapter failed health check, falling back to mock");
        return std::make_shared<adapters::tts::MockTTSEngine>();
    }
    return adapter;
}

contracts::TTSEnginePtr TTSEngineFactory::create_sapi_baseline() {
    return std::make_shared<adapters::tts::WindowsSAPITTSAdapter>();
}

contracts::TTSEnginePtr TTSEngineFactory::create_mock_engine() {
    return std::make_shared<adapters::tts::MockTTSEngine>();
}

contracts::TTSEnginePtr TTSEngineFactory::create_engine(const TTSFactoryConfig& config) {
    switch (config.provider) {
        case TTSProvider::PiperEnglish:
            return create_piper_english(config.models_root);
        case TTSProvider::PiperHindi:
            return create_piper_hindi(config.models_root);
        case TTSProvider::MatchaEnglish:
            return create_matcha_english(config.models_root);
        case TTSProvider::SAPIBaseline:
            return create_sapi_baseline();
        case TTSProvider::Mock:
        default:
            return create_mock_engine();
    }
}

} // namespace vani::voice::tts
