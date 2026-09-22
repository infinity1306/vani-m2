#pragma once

#include "../../contracts/models/model_provider.hpp"

namespace vani::adapters {

class MockModelAdapter : public contracts::ModelProvider {
public:
    [[nodiscard]] contracts::ModelCapabilities capabilities() const override {
        return contracts::ModelCapabilities{
            .model_id = "llama3.8b-local",
            .provider_name = "OllamaEngine",
            .is_local = true,
            .supports_text = true,
            .supports_vision = false,
            .supports_tools = true,
            .supports_streaming = true,
            .supports_structured_output = true,
            .context_window_tokens = 32768,
            .max_output_tokens = 4096,
            .estimated_vram_mb = 4800
        };
    }

    contracts::Result<contracts::ModelResponse> generate(
        const contracts::ModelRequest& request
    ) override {
        if (request.cancellation_token.is_cancelled()) {
            return contracts::Result<contracts::ModelResponse>::err(
                contracts::ErrorCode::Cancelled,
                "Model generation was cancelled.",
                "adapter.mock_model"
            );
        }

        return contracts::Result<contracts::ModelResponse>(contracts::ModelResponse{
            .content = "VANI Local Model execution result.",
            .finish_reason = "stop",
            .prompt_tokens = 42,
            .completion_tokens = 18,
            .total_tokens = 60,
            .ttft_ms = 35
        });
    }

    contracts::Result<void> stream(
        const contracts::ModelRequest& request,
        contracts::StreamChunkCallback chunk_cb
    ) override {
        if (request.cancellation_token.is_cancelled()) {
            return contracts::Result<void>::err(
                contracts::ErrorCode::Cancelled,
                "Model streaming was cancelled.",
                "adapter.mock_model"
            );
        }

        if (chunk_cb) {
            chunk_cb("Hello from ", false);
            chunk_cb("VANI Local Model!", true);
        }
        return contracts::Result<void>::ok();
    }
};

} // namespace vani::adapters
