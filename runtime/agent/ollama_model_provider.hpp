#pragma once

#include "agent_model_provider.hpp"
#include <string>
#include <memory>
#include <vector>
#include <mutex>

namespace vani::runtime::agent {

struct OllamaInferenceTelemetry {
    std::string provider{"Ollama"};
    std::string model{"qwen2.5:3b"};
    std::string request_id;
    uint64_t prompt_token_count{0};
    uint64_t output_token_count{0};
    uint64_t inference_start_ns{0};
    uint64_t first_token_ns{0};
    uint64_t inference_complete_ns{0};
    uint64_t latency_ms{0};
    bool success{false};
    std::string error;
    std::string prompt;
    std::string raw_response;
};

class OllamaAgentModelProvider : public AgentModelProvider {
public:
    explicit OllamaAgentModelProvider(
        std::string model_name = "qwen2.5:3b",
        std::string host = "127.0.0.1",
        int port = 11434
    );
    ~OllamaAgentModelProvider() override = default;

    [[nodiscard]] std::string provider_id() const override { return "ollama_model_provider"; }
    [[nodiscard]] bool is_local() const override { return true; }
    [[nodiscard]] bool is_available() const override;

    contracts::Result<contracts::AgentPlan> generate_plan(
        const contracts::AgentRequest& request,
        const ShortTermTaskMemoryPtr& memory
    ) override;

    contracts::Result<contracts::AgentPlan> replan(
        const contracts::AgentRequest& request,
        const contracts::AgentPlan& current_plan,
        const std::string& failed_step_id,
        const contracts::AgentObservation& failure_obs,
        const ShortTermTaskMemoryPtr& memory
    ) override;

    contracts::Result<std::string> summarize_observation(
        const contracts::AgentObservation& observation
    ) override;

    // Telemetry & diagnostics
    [[nodiscard]] OllamaInferenceTelemetry last_telemetry() const;
    [[nodiscard]] std::string model_name() const { return model_name_; }
    void set_model_name(std::string model) { model_name_ = std::move(model); }

    // Direct connectivity probe
    bool ping() const;

private:
    std::string model_name_;
    std::string host_;
    int port_;
    mutable std::mutex telemetry_mutex_;
    mutable OllamaInferenceTelemetry last_telemetry_;

    contracts::Result<std::string> call_generate_api(
        const std::string& prompt,
        const std::string& request_id,
        uint32_t timeout_ms = 45000
    );

    contracts::Result<contracts::AgentPlan> parse_plan_json(
        const std::string& raw_json,
        const std::string& request_id
    );
};

using OllamaAgentModelProviderPtr = std::shared_ptr<OllamaAgentModelProvider>;

} // namespace vani::runtime::agent
