#pragma once

#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>
#include <vector>
#include <string>

namespace vani::capabilities::system {

class ProcessManager {
public:
    explicit ProcessManager(adapters::system::SystemAdapterPtr adapter);
    ~ProcessManager() = default;

    contracts::Result<std::vector<contracts::ProcessMetadata>> list_processes();
    contracts::Result<contracts::ProcessMetadata> inspect_process(uint32_t pid);

    contracts::Result<uint32_t> start_process(
        const std::string& command,
        const std::string& working_dir = "",
        const std::unordered_map<std::string, std::string>& env = {}
    );

    contracts::Result<void> stop_process(uint32_t pid);      // Graceful
    contracts::Result<void> terminate_process(uint32_t pid); // Force terminate (High Risk)
    contracts::Result<uint32_t> restart_process(uint32_t pid);

private:
    adapters::system::SystemAdapterPtr adapter_;
};

using ProcessManagerPtr = std::shared_ptr<ProcessManager>;

} // namespace vani::capabilities::system
