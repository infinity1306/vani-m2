#pragma once

#include "../common/version.hpp"
#include "../common/result.hpp"
#include "../common/cancellation_token.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace vani::contracts {

struct ModelCapabilities {
    ModelId model_id;
    std::string provider_name;
    bool is_local{true};
    bool supports_text{true};
    bool supports_vision{false};
    bool supports_tools{false};
    bool supports_streaming{true};
    bool supports_structured_output{false};
    uint32_t context_window_tokens{8192};
    uint32_t max_output_tokens{4096};
    uint32_t estimated_vram_mb{0};
};

struct ModelMessage {
    std::string role; // "system", "user", "assistant", "tool"
    std::string content;
    std::string tool_call_id;
    std::string name;
};

struct ModelRequest {
    std::string model_id;
    std::vector<ModelMessage> messages;
    float temperature{0.7f};
    uint32_t max_tokens{2048};
    std::vector<std::string> tools_json_schema;
    CancellationToken cancellation_token{CancellationToken::none()};
};

struct ModelResponse {
    std::string content;
    std::string finish_reason; // "stop", "tool_calls", "length"
    uint32_t prompt_tokens{0};
    uint32_t completion_tokens{0};
    uint32_t total_tokens{0};
    uint32_t ttft_ms{0};
};

using StreamChunkCallback = std::function<void(const std::string& chunk, bool is_final)>;

class ModelProvider {
public:
    virtual ~ModelProvider() = default;

    [[nodiscard]] virtual ModelCapabilities capabilities() const = 0;

    virtual Result<ModelResponse> generate(const ModelRequest& request) = 0;

    virtual Result<void> stream(
        const ModelRequest& request,
        StreamChunkCallback chunk_cb
    ) = 0;
};

using ModelProviderPtr = std::shared_ptr<ModelProvider>;

} // namespace vani::contracts
