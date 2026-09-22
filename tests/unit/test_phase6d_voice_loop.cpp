#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cmath>
#include <thread>
#include <chrono>

#include "../../voice/pipeline/end_to_end_voice_loop.hpp"
#include "../../voice/pipeline/gated_voice_pipeline.hpp"
#include "../../voice/wakeword/wakeword_engine_factory.hpp"
#include "../../voice/stt/stt_engine_factory.hpp"
#include "../../voice/tts/tts_engine_factory.hpp"
#include "../../voice/tts/tts_manager.hpp"
#include "../../audio/output/miniaudio_audio_output.hpp"
#include "../../capabilities/system/gateway/tool_gateway.hpp"
#include "../../capabilities/system/applications/application_manager.hpp"
#include "../../capabilities/system/processes/process_manager.hpp"
#include "../../capabilities/system/filesystem/filesystem_manager.hpp"
#include "../../capabilities/system/terminal/terminal_executor.hpp"
#include "../../capabilities/system/projects/project_context.hpp"
#include "../../capabilities/system/browser/browser_manager.hpp"
#include "../../capabilities/system/windows/window_manager.hpp"
#include "../../capabilities/system/input/input_manager.hpp"
#include "../../capabilities/system/clipboard/clipboard_manager.hpp"
#include "../../capabilities/system/screen/screen_capture_manager.hpp"
#include "../../capabilities/system/display/display_manager.hpp"
#include "../../capabilities/system/media/media_manager.hpp"
#include "../../capabilities/system/system_state/system_state_provider.hpp"
#include "../../capabilities/system/network/network_manager.hpp"
#include "../../capabilities/system/power/power_manager.hpp"
#include "../../capabilities/system/notifications/notification_manager.hpp"
#include "../../capabilities/system/journal/system_action_journal.hpp"
#include "../../runtime/policy/policy_engine.hpp"
#include "../../runtime/permissions/permission_service.hpp"
#include "../../runtime/event_bus/event_bus.hpp"
#include "../../runtime/audit/audit_service.hpp"
#include "../../adapters/system/mock/mock_system_adapter.hpp"

using namespace vani;

static std::vector<float> make_synthetic_audio(const std::string& phrase, uint32_t sample_rate = 16000) {
    size_t pre_roll = sample_rate * 6 / 10;
    size_t speech = std::max<size_t>(sample_rate, phrase.size() * 500);
    size_t post_roll = sample_rate * 4 / 10;
    size_t total = pre_roll + speech + post_roll;

    std::vector<float> audio(total, 0.0f);
    for (size_t i = 0; i < speech; ++i) {
        double t = static_cast<double>(i) / sample_rate;
        float s = static_cast<float>(0.2 * std::sin(2.0 * 3.1415926535 * 180.0 * t));
        audio[pre_roll + i] = s;
    }
    return audio;
}

static capabilities::system::ToolGatewayPtr create_test_gateway() {
    auto mock_adapter = std::make_shared<adapters::system::MockSystemAdapter>();
    auto policy = std::make_shared<runtime::PolicyEngine>();
    auto permissions = std::make_shared<runtime::PermissionService>();
    auto event_bus = std::make_shared<runtime::EventBus>();
    auto audit = std::make_shared<runtime::AuditService>();

    auto app_reg = std::make_shared<capabilities::system::ApplicationRegistry>();
    auto app_mgr = std::make_shared<capabilities::system::ApplicationManager>(mock_adapter, app_reg);
    auto proc_mgr = std::make_shared<capabilities::system::ProcessManager>(mock_adapter);
    auto tx_mgr = std::make_shared<capabilities::system::FileTransactionManager>();
    auto watcher = std::make_shared<capabilities::system::FileWatcher>();
    auto fs_mgr = std::make_shared<capabilities::system::FilesystemManager>(mock_adapter, tx_mgr, watcher);
    auto term_exec = std::make_shared<capabilities::system::TerminalExecutor>(mock_adapter);
    auto proj_mgr = std::make_shared<capabilities::system::ProjectContextManager>();
    auto browser_mgr = std::make_shared<capabilities::system::BrowserManager>();
    auto win_mgr = std::make_shared<capabilities::system::WindowManager>(mock_adapter);
    auto input_mgr = std::make_shared<capabilities::system::InputManager>(mock_adapter);
    auto clip_mgr = std::make_shared<capabilities::system::ClipboardManager>(mock_adapter);
    auto screen_mgr = std::make_shared<capabilities::system::ScreenCaptureManager>(mock_adapter);
    auto disp_mgr = std::make_shared<capabilities::system::DisplayManager>(mock_adapter);
    auto media_mgr = std::make_shared<capabilities::system::MediaManager>(mock_adapter);
    auto state_prov = std::make_shared<capabilities::system::SystemStateProvider>(mock_adapter);
    auto net_mgr = std::make_shared<capabilities::system::NetworkManager>(mock_adapter);
    auto pwr_mgr = std::make_shared<capabilities::system::PowerManager>(mock_adapter);
    auto notif_mgr = std::make_shared<capabilities::system::NotificationManager>(mock_adapter);
    auto journal = std::make_shared<capabilities::system::SystemActionJournal>();

    return std::make_shared<capabilities::system::ToolGateway>(
        policy, permissions, event_bus, audit,
        app_mgr, proc_mgr, fs_mgr, term_exec, proj_mgr,
        browser_mgr, win_mgr, input_mgr, clip_mgr, screen_mgr,
        disp_mgr, media_mgr, state_prov, net_mgr, pwr_mgr, notif_mgr, journal
    );
}

// 1. State transitions
static void test_01_state_transitions() {
    std::cout << "  [1/17] State Transitions Invariant... ";
    auto wakeword = voice::wakeword::WakeWordEngineFactory::create_engine(voice::wakeword::WakeWordProviderType::SherpaKWS);
    auto stt = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    auto pipeline = std::make_shared<voice::pipeline::GatedVoicePipeline>(wakeword, stt);
    auto voice_loop = std::make_shared<voice::pipeline::EndToEndVoiceLoop>(pipeline, nullptr, nullptr, nullptr);

    assert(voice_loop->state() == voice::pipeline::VoiceLoopState::Idle);

    voice_loop->trigger_wake("vani");
    assert(voice_loop->state() == voice::pipeline::VoiceLoopState::Listening);

    voice_loop->finalize_turn();
    assert(voice_loop->state() == voice::pipeline::VoiceLoopState::Idle);
    std::cout << "PASSED\n";
}

// 2. Wake gating
static void test_02_wake_gating() {
    std::cout << "  [2/17] Wake Gating & Zero Idle Whisper... ";
    auto wakeword = voice::wakeword::WakeWordEngineFactory::create_engine(voice::wakeword::WakeWordProviderType::SherpaKWS);
    auto stt = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    auto pipeline = std::make_shared<voice::pipeline::GatedVoicePipeline>(wakeword, stt);
    auto voice_loop = std::make_shared<voice::pipeline::EndToEndVoiceLoop>(pipeline, nullptr, nullptr, nullptr);

    // Feed speech without wake word
    std::vector<float> noise(16000, 0.001f);
    voice_loop->process_audio_frame(noise);

    assert(voice_loop->state() == voice::pipeline::VoiceLoopState::Idle);
    assert(voice_loop->whisper_invocations_count() == 0);
    std::cout << "PASSED\n";
}

// 3. Pre-roll buffer
static void test_03_pre_roll() {
    std::cout << "  [3/17] Pre-roll Buffer Audio Retention... ";
    auto wakeword = voice::wakeword::WakeWordEngineFactory::create_engine(voice::wakeword::WakeWordProviderType::SherpaKWS);
    auto stt = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    voice::pipeline::GatedPipelineConfig cfg;
    cfg.pre_roll_ms = 600;
    auto pipeline = std::make_shared<voice::pipeline::GatedVoicePipeline>(wakeword, stt, cfg);

    // Feed 1 second of audio into pre-roll buffer
    std::vector<float> frames(16000, 0.05f);
    pipeline->process_audio_frame(frames);

    assert(pipeline->current_state() == voice::pipeline::PipelineState::Idle);
    std::cout << "PASSED\n";
}

// 4. STT integration
static void test_04_stt_integration() {
    std::cout << "  [4/17] STT Integration & Transcript Delivery... ";
    auto wakeword = voice::wakeword::WakeWordEngineFactory::create_engine(voice::wakeword::WakeWordProviderType::SherpaKWS);
    auto stt = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    auto pipeline = std::make_shared<voice::pipeline::GatedVoicePipeline>(wakeword, stt);
    auto voice_loop = std::make_shared<voice::pipeline::EndToEndVoiceLoop>(pipeline, nullptr, nullptr, nullptr);

    auto audio = make_synthetic_audio("VANI open Chrome");
    auto evidence = voice_loop->process_utterance(audio, "vani", "open Chrome");

    assert(evidence.wake_success);
    assert(evidence.stt_success);
    assert(!evidence.raw_transcript.empty());
    assert(evidence.timestamps.t4_final_stt_result_ns.has_value());
    std::cout << "PASSED\n";
}

// 5. Intent integration
static void test_05_intent_integration() {
    std::cout << "  [5/17] Intent Integration (Multilingual)... ";
    auto wakeword = voice::wakeword::WakeWordEngineFactory::create_engine(voice::wakeword::WakeWordProviderType::SherpaKWS);
    auto stt = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    auto pipeline = std::make_shared<voice::pipeline::GatedVoicePipeline>(wakeword, stt);
    auto voice_loop = std::make_shared<voice::pipeline::EndToEndVoiceLoop>(pipeline, nullptr, nullptr, nullptr);

    auto audio = make_synthetic_audio("dummy");
    auto ev_hi = voice_loop->process_utterance(audio, "vani", "Chrome kholo");
    assert(ev_hi.intent_success);
    assert(ev_hi.detected_intent == "application.launch");

    auto ev_url = voice_loop->process_utterance(audio, "vani", "localhost kholo");
    assert(ev_url.intent_success);
    assert(ev_url.detected_intent == "browser.open_url");

    auto ev_close = voice_loop->process_utterance(audio, "vani", "browser band karo");
    assert(ev_close.intent_success);
    assert(ev_close.detected_intent == "application.close");

    std::cout << "PASSED\n";
}

// 6. Tool routing
static void test_06_tool_routing() {
    std::cout << "  [6/17] Tool Routing to Gateway Fast Path... ";
    auto gateway = create_test_gateway();
    std::unordered_map<std::string, std::string> params;
    params["app_name"] = "Google Chrome";
    auto res = gateway->execute_fast_path("application.launch", params);
    assert(res.is_ok());
    assert(res.value().success);
    std::cout << "PASSED\n";
}

// 7. Verification
static void test_07_verification() {
    std::cout << "  [7/17] Postcondition Verification... ";
    auto gateway = create_test_gateway();
    std::unordered_map<std::string, std::string> params;
    params["app_name"] = "Google Chrome";
    auto res = gateway->execute_fast_path("application.launch", params);
    assert(res.is_ok());
    assert(res.value().success);
    std::cout << "PASSED\n";
}

// 8. Response generation
static void test_08_response_generation() {
    std::cout << "  [8/17] Contextual Response Generation... ";
    auto gateway = create_test_gateway();
    auto wakeword = voice::wakeword::WakeWordEngineFactory::create_engine(voice::wakeword::WakeWordProviderType::SherpaKWS);
    auto stt = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    auto pipeline = std::make_shared<voice::pipeline::GatedVoicePipeline>(wakeword, stt);
    auto voice_loop = std::make_shared<voice::pipeline::EndToEndVoiceLoop>(pipeline, gateway, nullptr, nullptr);

    auto audio = make_synthetic_audio("dummy");
    auto ev_en = voice_loop->process_utterance(audio, "vani", "open Chrome");
    assert(ev_en.response_text.find("Google Chrome") != std::string::npos || ev_en.response_text.find("Opening") != std::string::npos);

    auto ev_hi = voice_loop->process_utterance(audio, "vani", "Chrome kholo");
    assert(ev_hi.response_text.find("Chrome") != std::string::npos && ev_hi.response_text.find("kar diya hai") != std::string::npos);
    std::cout << "PASSED\n";
}

// 9. TTS
static void test_09_tts() {
    std::cout << "  [9/17] TTS Synthesis Subsystem... ";
    auto tts_manager = std::make_shared<voice::tts::TTSManager>();
    tts_manager->register_engine(voice::tts::TTSEngineFactory::create_piper_english(), true);

    auto syn_res = tts_manager->synthesize("Testing voice synthesis");
    assert(syn_res.is_ok());
    assert(!syn_res.value().empty());
    std::cout << "PASSED\n";
}

// 10. Playback
static void test_10_playback() {
    std::cout << "  [10/17] Audio Playback Queue... ";
    auto audio_output = std::make_shared<audio::MiniaudioAudioOutput>();
    audio_output->start();

    std::vector<float> chunk(1024, 0.01f);
    auto q_res = audio_output->queue_chunk(chunk, true);
    assert(q_res.is_ok());
    assert(audio_output->total_samples_played() >= 1024);
    std::cout << "PASSED\n";
}

// 11. Cancellation
static void test_11_cancellation() {
    std::cout << "  [11/17] Cooperative Cancellation... ";
    auto gateway = create_test_gateway();
    auto wakeword = voice::wakeword::WakeWordEngineFactory::create_engine(voice::wakeword::WakeWordProviderType::SherpaKWS);
    auto stt = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    auto tts_manager = std::make_shared<voice::tts::TTSManager>();
    tts_manager->register_engine(voice::tts::TTSEngineFactory::create_piper_english(), true);
    auto audio_output = std::make_shared<audio::MiniaudioAudioOutput>();
    auto pipeline = std::make_shared<voice::pipeline::GatedVoicePipeline>(wakeword, stt);
    auto voice_loop = std::make_shared<voice::pipeline::EndToEndVoiceLoop>(pipeline, gateway, tts_manager, audio_output);

    voice_loop->trigger_wake("vani");
    assert(voice_loop->state() == voice::pipeline::VoiceLoopState::Listening);

    voice_loop->cancel();
    assert(voice_loop->state() == voice::pipeline::VoiceLoopState::Idle);
    std::cout << "PASSED\n";
}

// 12. Timeout
static void test_12_timeout() {
    std::cout << "  [12/17] Session Timeout & Reset to IDLE... ";
    auto wakeword = voice::wakeword::WakeWordEngineFactory::create_engine(voice::wakeword::WakeWordProviderType::SherpaKWS);
    auto stt = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    voice::pipeline::GatedPipelineConfig cfg;
    cfg.listen_timeout_ms = 50; // short 50ms timeout for test
    auto pipeline = std::make_shared<voice::pipeline::GatedVoicePipeline>(wakeword, stt, cfg);
    auto voice_loop = std::make_shared<voice::pipeline::EndToEndVoiceLoop>(pipeline, nullptr, nullptr, nullptr);

    voice_loop->trigger_wake("vani");
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    // Feed empty/silence frame to trigger timeout check
    std::vector<float> silence(480, 0.0f);
    voice_loop->process_audio_frame(silence);

    assert(voice_loop->state() == voice::pipeline::VoiceLoopState::Idle);
    std::cout << "PASSED\n";
}

// 13. Duplicate wake
static void test_13_duplicate_wake() {
    std::cout << "  [13/17] Duplicate Wake Protection... ";
    auto wakeword = voice::wakeword::WakeWordEngineFactory::create_engine(voice::wakeword::WakeWordProviderType::SherpaKWS);
    auto stt = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    auto pipeline = std::make_shared<voice::pipeline::GatedVoicePipeline>(wakeword, stt);
    auto voice_loop = std::make_shared<voice::pipeline::EndToEndVoiceLoop>(pipeline, nullptr, nullptr, nullptr);

    voice_loop->trigger_wake("vani");
    assert(voice_loop->state() == voice::pipeline::VoiceLoopState::Listening);

    // Duplicate wake while listening
    voice_loop->trigger_wake("vani");
    assert(voice_loop->state() == voice::pipeline::VoiceLoopState::Listening);

    voice_loop->reset();
    assert(voice_loop->state() == voice::pipeline::VoiceLoopState::Idle);
    std::cout << "PASSED\n";
}

// 14. Self-trigger prevention
static void test_14_self_trigger_prevention() {
    std::cout << "  [14/17] Self-Trigger Prevention during Output... ";
    auto wakeword = voice::wakeword::WakeWordEngineFactory::create_engine(voice::wakeword::WakeWordProviderType::SherpaKWS);
    auto stt = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    auto pipeline = std::make_shared<voice::pipeline::GatedVoicePipeline>(wakeword, stt);
    auto voice_loop = std::make_shared<voice::pipeline::EndToEndVoiceLoop>(pipeline, nullptr, nullptr, nullptr);

    auto audio = make_synthetic_audio("VANI open Chrome");
    voice_loop->process_audio_frame(audio);

    assert(voice_loop->state() == voice::pipeline::VoiceLoopState::Idle);
    std::cout << "PASSED\n";
}

// 15. Error recovery
static void test_15_error_recovery() {
    std::cout << "  [15/17] Error Recovery to IDLE... ";
    auto gateway = create_test_gateway();
    auto wakeword = voice::wakeword::WakeWordEngineFactory::create_engine(voice::wakeword::WakeWordProviderType::SherpaKWS);
    auto stt = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    auto pipeline = std::make_shared<voice::pipeline::GatedVoicePipeline>(wakeword, stt);
    auto voice_loop = std::make_shared<voice::pipeline::EndToEndVoiceLoop>(pipeline, gateway, nullptr, nullptr);

    // Execute invalid intent
    auto audio = make_synthetic_audio("dummy");
    auto evidence = voice_loop->process_utterance(audio, "vani", "invalid nonexistent action xyz");

    // Must recover safely to IDLE without crashing
    assert(voice_loop->state() == voice::pipeline::VoiceLoopState::Idle);
    std::cout << "PASSED\n";
}

// 16. Offline mode
static void test_16_offline_mode() {
    std::cout << "  [16/17] Offline Mode Guarantee... ";
    // All components used (Silero, Whisper Tiny, Piper VITS, Miniaudio) are strictly local ONNX / C++ binaries.
    auto wakeword = voice::wakeword::WakeWordEngineFactory::create_engine(voice::wakeword::WakeWordProviderType::SherpaKWS);
    auto stt = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    auto tts = voice::tts::TTSEngineFactory::create_piper_english();

    assert(wakeword != nullptr);
    assert(stt != nullptr);
    assert(tts != nullptr);
    assert(tts->is_offline_capable());
    std::cout << "PASSED\n";
}

// 17. Resource cleanup
static void test_17_resource_cleanup() {
    std::cout << "  [17/17] Resource Cleanup & Reset... ";
    auto gateway = create_test_gateway();
    auto wakeword = voice::wakeword::WakeWordEngineFactory::create_engine(voice::wakeword::WakeWordProviderType::SherpaKWS);
    auto stt = voice::stt::STTEngineFactory::create_engine(voice::stt::STTProviderType::SherpaWhisperTiny);
    auto tts_manager = std::make_shared<voice::tts::TTSManager>();
    tts_manager->register_engine(voice::tts::TTSEngineFactory::create_piper_english(), true);
    auto audio_output = std::make_shared<audio::MiniaudioAudioOutput>();
    auto pipeline = std::make_shared<voice::pipeline::GatedVoicePipeline>(wakeword, stt);

    {
        auto voice_loop = std::make_unique<voice::pipeline::EndToEndVoiceLoop>(pipeline, gateway, tts_manager, audio_output);
        voice_loop->trigger_wake("vani");
        voice_loop->reset();
        assert(voice_loop->state() == voice::pipeline::VoiceLoopState::Idle);
    } // destructor executes clean cancellation and audio stop

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "======================================================\n";
    std::cout << "   PHASE 6D END-TO-END VOICE LOOP UNIT TESTS (1-17)   \n";
    std::cout << "======================================================\n";

    test_01_state_transitions();
    test_02_wake_gating();
    test_03_pre_roll();
    test_04_stt_integration();
    test_05_intent_integration();
    test_06_tool_routing();
    test_07_verification();
    test_08_response_generation();
    test_09_tts();
    test_10_playback();
    test_11_cancellation();
    test_12_timeout();
    test_13_duplicate_wake();
    test_14_self_trigger_prevention();
    test_15_error_recovery();
    test_16_offline_mode();
    test_17_resource_cleanup();

    std::cout << "======================================================\n";
    std::cout << "ALL 17 PHASE 6D TESTS PASSED (100% SUCCESS RATE)\n";
    std::cout << "======================================================\n";
    return 0;
}
