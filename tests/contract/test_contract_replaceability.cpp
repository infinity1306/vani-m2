#include "../../contracts/providers/stt_engine.hpp"
#include "../../contracts/models/model_provider.hpp"
#include "../../contracts/agents/agent.hpp"
#include "../../contracts/devices/device.hpp"
#include "../../adapters/stt/mock_stt_adapter.hpp"
#include "../../adapters/models/mock_model_adapter.hpp"
#include "../../adapters/agents/mock_agent_adapter.hpp"
#include "../../adapters/devices/mock_device_adapter.hpp"
#include <cassert>
#include <iostream>

// Hypothetical alternative STT engine (e.g. Whisper engine)
class AlternativeWhisperSTT : public vani::contracts::STTEngine {
public:
    vani::contracts::Result<void> start_stream(
        const vani::contracts::STTConfig&,
        vani::contracts::TranscriptCallback
    ) override {
        return vani::contracts::Result<void>::ok();
    }
    vani::contracts::Result<void> push_audio(std::span<const float>) override {
        return vani::contracts::Result<void>::ok();
    }
    vani::contracts::Result<void> stop_stream() override {
        return vani::contracts::Result<void>::ok();
    }
    [[nodiscard]] std::string engine_name() const override {
        return "Whisper_v3_Turbo_Adapter";
    }
    [[nodiscard]] vani::contracts::STTCapabilities capabilities() const override {
        return {
            .supports_streaming = true,
            .supports_interim_results = true,
            .supports_word_timestamps = true,
            .supports_multilingual = true,
            .is_offline_capable = true,
            .typical_latency_ms = 40,
            .supported_languages = {"en", "hi"}
        };
    }
};

void test_contract_replaceability() {
    // 1. Test STT Replaceability
    std::unique_ptr<vani::contracts::STTEngine> stt_1 = std::make_unique<vani::adapters::MockSTTAdapter>();
    assert(stt_1->engine_name() == "MockSTTAdapter_SherpaV2");

    std::unique_ptr<vani::contracts::STTEngine> stt_2 = std::make_unique<AlternativeWhisperSTT>();
    assert(stt_2->engine_name() == "Whisper_v3_Turbo_Adapter");

    // 2. Test Model Provider Replaceability
    std::shared_ptr<vani::contracts::ModelProvider> model_provider = std::make_shared<vani::adapters::MockModelAdapter>();
    auto caps = model_provider->capabilities();
    assert(caps.is_local == true);
    assert(caps.context_window_tokens == 32768);

    vani::contracts::ModelRequest req{
        .model_id = "llama3.8b-local",
        .messages = {},
        .temperature = 0.7f,
        .max_tokens = 2048,
        .tools_json_schema = {},
        .cancellation_token = vani::contracts::CancellationToken::none()
    };
    auto gen_res = model_provider->generate(req);
    assert(gen_res.is_ok());
    assert(!gen_res.value().content.empty());

    // 3. Test Agent Replaceability
    std::shared_ptr<vani::contracts::Agent> agent = std::make_shared<vani::adapters::MockAgentAdapter>("agent.hermes", "Research");
    assert(agent->manifest().role == "Research");

    vani::contracts::AgentRequest agent_req{
        .task_id = "task_test_123",
        .session_id = "sess_001",
        .instruction = "Research AI Agents",
        .allowed_tools = {},
        .model_id_preference = "",
        .cancellation_token = vani::contracts::CancellationToken::none()
    };
    auto agent_res = agent->run(agent_req);
    assert(agent_res.is_ok());
    assert(agent_res.value().success == true);

    // 4. Test Device Replaceability
    std::shared_ptr<vani::contracts::Device> device = std::make_shared<vani::adapters::MockDeviceAdapter>("dev_watch_01", "VANI Watch Ultra");
    assert(device->manifest().name == "VANI Watch Ultra");
    assert(device->is_connected() == true);

    std::cout << "  [PASS] test_contract_replaceability\n";
}
