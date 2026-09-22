#include "process_manager.hpp"

namespace vani::capabilities::system {

ProcessManager::ProcessManager(adapters::system::SystemAdapterPtr adapter)
    : adapter_(std::move(adapter)) {}

contracts::Result<std::vector<contracts::ProcessMetadata>> ProcessManager::list_processes() {
    if (!adapter_) {
        return contracts::Result<std::vector<contracts::ProcessMetadata>>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->list_processes();
}

contracts::Result<contracts::ProcessMetadata> ProcessManager::inspect_process(uint32_t pid) {
    if (!adapter_) {
        return contracts::Result<contracts::ProcessMetadata>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->inspect_process(pid);
}

contracts::Result<uint32_t> ProcessManager::start_process(
    const std::string& command,
    const std::string& working_dir,
    const std::unordered_map<std::string, std::string>& env
) {
    if (!adapter_) {
        return contracts::Result<uint32_t>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->start_process(command, working_dir, env);
}

contracts::Result<void> ProcessManager::stop_process(uint32_t pid) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->stop_process(pid, false);
}

contracts::Result<void> ProcessManager::terminate_process(uint32_t pid) {
    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }
    return adapter_->stop_process(pid, true);
}

contracts::Result<uint32_t> ProcessManager::restart_process(uint32_t pid) {
    auto inspect_res = inspect_process(pid);
    if (!inspect_res.is_success()) {
        return contracts::Result<uint32_t>::failure(inspect_res.error());
    }
    const auto& proc = inspect_res.value();
    auto stop_res = stop_process(pid);
    if (!stop_res.is_success()) {
        return contracts::Result<uint32_t>::failure(stop_res.error());
    }
    return start_process(proc.name, proc.path);
}

} // namespace vani::capabilities::system
