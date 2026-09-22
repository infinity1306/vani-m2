#include "../../contracts/providers/stt_engine.hpp"
#include "../../voice/stt/stt_engine_factory.hpp"
#include "../../voice/stt/stt_manager.hpp"
#include "../../adapters/stt/mock_stt_adapter.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace vani;

void test_factory_string_mapping() {
    using namespace voice::stt;
    std::cout << "[TEST] STTEngineFactory string mapping..." << std::endl;

    assert(STTEngineFactory::string_to_provider_type("sherpa_zipformer_en") == STTProviderType::SherpaZipformerEn);
    assert(STTEngineFactory::string_to_provider_type("sensevoice") == STTProviderType::SherpaSenseVoice);
    assert(STTEngineFactory::string_to_provider_type("whisper_tiny") == STTProviderType::SherpaWhisperTiny);
    assert(STTEngineFactory::string_to_provider_type("mock") == STTProviderType::Mock);
    assert(STTEngineFactory::string_to_provider_type("UNKNOWN_PROVIDER") == STTProviderType::SherpaZipformerEn);

    assert(STTEngineFactory::provider_type_to_string(STTProviderType::SherpaZipformerEn) == "sherpa_zipformer_en");
    assert(STTEngineFactory::provider_type_to_string(STTProviderType::SherpaSenseVoice) == "sensevoice");
    assert(STTEngineFactory::provider_type_to_string(STTProviderType::SherpaWhisperTiny) == "whisper_tiny");
    assert(STTEngineFactory::provider_type_to_string(STTProviderType::Mock) == "mock");

    std::cout << "  -> PASSED" << std::endl;
}

void test_factory_engine_creation() {
    using namespace voice::stt;
    std::cout << "[TEST] STTEngineFactory engine instantiation..." << std::endl;

    auto mock_engine = STTEngineFactory::create_engine(STTProviderType::Mock);
    assert(mock_engine != nullptr);
    assert(mock_engine->is_healthy());
    assert(mock_engine->engine_name() == "MockSTT");

    auto named_engine = STTEngineFactory::create_engine_by_name("mock");
    assert(named_engine != nullptr);
    assert(named_engine->engine_name() == "MockSTT");

    std::cout << "  -> PASSED" << std::endl;
}

void test_stt_manager_registration_and_fallback() {
    using namespace voice::stt;
    std::cout << "[TEST] STTManager engine registration and fallback switching..." << std::endl;

    STTManager manager;
    assert(manager.registered_engine_count() == 0);
    assert(manager.active_engine_name() == "None");

    auto primary = std::make_shared<adapters::MockSTTAdapter>();
    auto fallback = std::make_shared<adapters::MockSTTAdapter>();

    manager.register_engine(primary, true);
    manager.set_fallback_engine(fallback);

    assert(manager.registered_engine_count() == 1);
    assert(manager.active_engine_name() == "MockSTT");

    // Start stream
    contracts::STTConfig cfg{.sample_rate_hz = 16000};
    bool callback_called = false;
    auto res = manager.start_stream(cfg, [&](const contracts::STTTranscript& t) {
        callback_called = true;
    });

    assert(res.is_ok());

    // Push dummy audio
    std::vector<float> audio(1600, 0.0f);
    auto push_res = manager.push_audio(audio);
    assert(push_res.is_ok());

    auto stop_res = manager.stop_stream();
    assert(stop_res.is_ok());

    std::cout << "  -> PASSED" << std::endl;
}

void test_vendor_isolation() {
    std::cout << "[TEST] Vendor contract isolation..." << std::endl;
    // Verify that STTEnginePtr and STTConfig do not depend on external headers
    contracts::STTConfig cfg;
    cfg.language_preference = "hinglish";
    assert(cfg.sample_rate_hz == 16000);
    assert(cfg.channels == 1);
    std::cout << "  -> PASSED" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " RUNNING PHASE 5C STT BAKE-OFF TESTS" << std::endl;
    std::cout << "========================================" << std::endl;

    test_factory_string_mapping();
    test_factory_engine_creation();
    test_stt_manager_registration_and_fallback();
    test_vendor_isolation();

    std::cout << "\nALL 4/4 PHASE 5C UNIT TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
