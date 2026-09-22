#pragma once

#include "../../contracts/tasks/task.hpp"
#include "../../contracts/common/result.hpp"
#include "../../contracts/common/cancellation_token.hpp"
#include "../event_bus/event_bus.hpp"
#include "../../storage/task_repository.hpp"
#include "timeout_manager.hpp"
#include "recovery_manager.hpp"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <optional>
#include <vector>

namespace vani::runtime {

class TaskManager {
public:
    explicit TaskManager(
        EventBusPtr event_bus = nullptr,
        storage::TaskRepositoryPtr repository = nullptr,
        RecoveryManagerPtr recovery_manager = nullptr
    );
    ~TaskManager();

    contracts::Result<contracts::TaskId> create_task(const contracts::TaskSpecification& spec);

    [[nodiscard]] std::optional<contracts::Task> get_task(const contracts::TaskId& task_id) const;
    [[nodiscard]] std::vector<contracts::Task> list_tasks() const;
    [[nodiscard]] std::vector<contracts::Task> list_tasks_by_session(const contracts::SessionId& session_id) const;

    contracts::Result<void> transition_task_state(
        const contracts::TaskId& task_id,
        contracts::TaskState target_state,
        std::optional<std::string> reason = std::nullopt
    );

    contracts::Result<void> update_progress(
        const contracts::TaskId& task_id,
        uint8_t progress_percent,
        std::optional<std::string> stage_description = std::nullopt
    );

    contracts::Result<void> complete_task(
        const contracts::TaskId& task_id,
        contracts::TaskResult result
    );

    contracts::Result<void> fail_task(
        const contracts::TaskId& task_id,
        contracts::Error error
    );

    contracts::Result<void> request_cancel(
        const contracts::TaskId& task_id,
        std::string_view reason = "User requested cancellation"
    );

    [[nodiscard]] contracts::CancellationToken get_cancellation_token(const contracts::TaskId& task_id);

    void set_checkpoint(const contracts::TaskId& task_id, std::string checkpoint_data);

    void check_all_timeouts(uint64_t current_time_ms);

private:
    [[nodiscard]] bool is_valid_transition(contracts::TaskState current, contracts::TaskState target) const noexcept;
    void emit_task_event(const contracts::TaskId& task_id, const std::string& event_type, const contracts::Task& task);

    mutable std::mutex mutex_;
    EventBusPtr event_bus_;
    storage::TaskRepositoryPtr repository_;
    RecoveryManagerPtr recovery_manager_;
    TimeoutManager timeout_manager_;

    std::unordered_map<contracts::TaskId, contracts::Task> active_tasks_;
    std::unordered_map<contracts::TaskId, contracts::CancellationSource> cancellation_sources_;
    std::unordered_map<contracts::TaskId, uint32_t> task_attempts_;
    uint64_t task_counter_{1};
};

using TaskManagerPtr = std::shared_ptr<TaskManager>;

} // namespace vani::runtime
