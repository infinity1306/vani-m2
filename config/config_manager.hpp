#pragma once

#include "../contracts/common/result.hpp"
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <optional>

namespace vani::config {

struct RuntimeProfile {
    std::string profile_name{"local-first"};
    std::string default_stt_engine{"mock_stt"};
    std::string default_tts_engine{"mock_tts"};
    std::string default_model_provider{"mock_model"};
    std::string default_coding_agent{"mock_odysseus"};
    bool local_only_mode{false};
    uint32_t default_task_timeout_sec{300};
    uint32_t ipc_port{8765};
    std::string storage_path{"./vani_data.db"};
};

class ConfigManager {
public:
    explicit ConfigManager(RuntimeProfile profile = RuntimeProfile{});

    [[nodiscard]] const RuntimeProfile& profile() const noexcept;

    void set_override(const std::string& key, const std::string& value);

    [[nodiscard]] std::optional<std::string> get_override(const std::string& key) const;

    contracts::Result<void> validate() const;

private:
    RuntimeProfile profile_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> overrides_;
};

using ConfigManagerPtr = std::shared_ptr<ConfigManager>;

} // namespace vani::config
