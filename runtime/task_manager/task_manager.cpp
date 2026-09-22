#include "task_manager.hpp"
#include <chrono>

namespace vani::runtime {

TaskManager::TaskManager(
    EventBusPtr event_bus,
    storage::TaskRepositoryPtr repository,
    RecoveryManagerPtr recovery_manager
) : event_bus_(std::move(event_bus)),
    repository_(std::move(repository)),
    recovery_manager_(std::move(recovery_manager)) {}

TaskManager::~TaskManager() = default;

contracts::Result<contracts::TaskId> TaskManager::create_task(const contracts::TaskSpecification& spec) {
    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::lock_guard<std::mutex> lock(mutex_);
    const auto task_id = spec.task_id.empty() 
        ? "task_" + std::to_string(now_ms) + "_" + std::to_string(task_counter_++) 
        : spec.task_id;

    contracts::Task task{
        .spec = spec,
        .state = contracts::TaskState::Created,
        .progress_percent = 0,
        .created_at_ms = now_ms,
        .updated_at_ms = now_ms,
        .completed_at_ms = std::nullopt,
        .result = std::nullopt,
        .error = std::nullopt,
        .checkpoint = std::nullopt
    };
    task.spec.task_id = task_id;

    active_tasks_[task_id] = task;
    cancellation_sources_[task_id] = contracts::CancellationSource();
    task_attempts_[task_id] = 1;

    if (repository_) {
        repository_->save(task);
    }

    emit_task_event(task_id, "task.created", task);

    return contracts::Result<contracts::TaskId>(task_id);
}

std::optional<contracts::Task> TaskManager::get_task(const contracts::TaskId& task_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_tasks_.find(task_id);
    if (it != active_tasks_.end()) {
        return it->second;
    }
    if (repository_) {
        return repository_->find_by_id(task_id);
    }
    return std::nullopt;
}

std::vector<contracts::Task> TaskManager::list_tasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<contracts::Task> list;
    list.reserve(active_tasks_.size());
    for (const auto& [id, task] : active_tasks_) {
        list.push_back(task);
    }
    return list;
}

std::vector<contracts::Task> TaskManager::list_tasks_by_session(const contracts::SessionId& session_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<contracts::Task> list;
    for (const auto& [id, task] : active_tasks_) {
        if (task.spec.session_id == session_id) {
            list.push_back(task);
        }
    }
    return list;
}

bool TaskManager::is_valid_transition(contracts::TaskState current, contracts::TaskState target) const noexcept {
    if (current == target) return true;

    switch (current) {
        case contracts::TaskState::Created:
            return target == contracts::TaskState::Planning ||
                   target == contracts::TaskState::Ready ||
                   target == contracts::TaskState::WaitingPermission ||
                   target == contracts::TaskState::Cancelled ||
                   target == contracts::TaskState::Failed;

        case contracts::TaskState::Planning:
            return target == contracts::TaskState::Ready ||
                   target == contracts::TaskState::WaitingPermission ||
                   target == contracts::TaskState::Cancelled ||
                   target == contracts::TaskState::Failed;

        case contracts::TaskState::Ready:
            return target == contracts::TaskState::WaitingPermission ||
                   target == contracts::TaskState::Running ||
                   target == contracts::TaskState::Cancelled ||
                   target == contracts::TaskState::Failed;

        case contracts::TaskState::WaitingPermission:
            return target == contracts::TaskState::Running ||
                   target == contracts::TaskState::Cancelled ||
                   target == contracts::TaskState::Failed;

        case contracts::TaskState::Running:
            return target == contracts::TaskState::WaitingInput ||
                   target == contracts::TaskState::Paused ||
                   target == contracts::TaskState::Verifying ||
                   target == contracts::TaskState::Recovering ||
                   target == contracts::TaskState::Completed ||
                   target == contracts::TaskState::Failed ||
                   target == contracts::TaskState::Cancelled;

        case contracts::TaskState::WaitingInput:
            return target == contracts::TaskState::Running ||
                   target == contracts::TaskState::Cancelled ||
                   target == contracts::TaskState::Failed;

        case contracts::TaskState::Paused:
            return target == contracts::TaskState::Running ||
                   target == contracts::TaskState::Cancelled ||
                   target == contracts::TaskState::Failed;

        case contracts::TaskState::Verifying:
            return target == contracts::TaskState::Completed ||
                   target == contracts::TaskState::Recovering ||
                   target == contracts::TaskState::Failed ||
                   target == contracts::TaskState::Cancelled;

        case contracts::TaskState::Recovering:
            return target == contracts::TaskState::Ready ||
                   target == contracts::TaskState::Running ||
                   target == contracts::TaskState::Failed ||
                   target == contracts::TaskState::Cancelled;

        case contracts::TaskState::Completed:
        case contracts::TaskState::Failed:
        case contracts::TaskState::Cancelled:
            return false; // Terminal states cannot transition to anything else
    }
    return false;
}

contracts::Result<void> TaskManager::transition_task_state(
    const contracts::TaskId& task_id,
    contracts::TaskState target_state,
    std::optional<std::string>
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_tasks_.find(task_id);
    if (it == active_tasks_.end()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::NotFound,
            "Task not found: " + task_id,
            "vani.runtime.task_manager",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    const auto current = it->second.state;
    if (!is_valid_transition(current, target_state)) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::Failure,
            "Illegal task transition from " + std::string(contracts::to_string(current)) +
            " to " + std::string(contracts::to_string(target_state)),
            "vani.runtime.task_manager",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    it->second.state = target_state;
    it->second.updated_at_ms = now_ms;

    if (target_state == contracts::TaskState::Completed ||
        target_state == contracts::TaskState::Failed ||
        target_state == contracts::TaskState::Cancelled) {
        it->second.completed_at_ms = now_ms;
    }

    if (repository_) {
        repository_->save(it->second);
    }

    emit_task_event(task_id, "task.state_changed", it->second);

    return contracts::Result<void>::ok();
}

contracts::Result<void> TaskManager::update_progress(
    const contracts::TaskId& task_id,
    uint8_t progress_percent,
    std::optional<std::string>
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_tasks_.find(task_id);
    if (it == active_tasks_.end()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::NotFound,
            "Task not found: " + task_id,
            "vani.runtime.task_manager",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    it->second.progress_percent = progress_percent > 100 ? 100 : progress_percent;
    it->second.updated_at_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    if (repository_) {
        repository_->save(it->second);
    }

    emit_task_event(task_id, "task.progress_updated", it->second);

    return contracts::Result<void>::ok();
}

contracts::Result<void> TaskManager::complete_task(
    const contracts::TaskId& task_id,
    contracts::TaskResult result
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_tasks_.find(task_id);
    if (it == active_tasks_.end()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::NotFound,
            "Task not found: " + task_id,
            "vani.runtime.task_manager",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    if (!is_valid_transition(it->second.state, contracts::TaskState::Completed)) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::Failure,
            "Cannot complete task from state " + std::string(contracts::to_string(it->second.state)),
            "vani.runtime.task_manager",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    it->second.state = contracts::TaskState::Completed;
    it->second.progress_percent = 100;
    it->second.result = std::move(result);
    it->second.updated_at_ms = now_ms;
    it->second.completed_at_ms = now_ms;

    if (repository_) {
        repository_->save(it->second);
    }

    timeout_manager_.unregister_task(task_id);
    emit_task_event(task_id, "task.completed", it->second);

    return contracts::Result<void>::ok();
}

contracts::Result<void> TaskManager::fail_task(
    const contracts::TaskId& task_id,
    contracts::Error error
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_tasks_.find(task_id);
    if (it == active_tasks_.end()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::NotFound,
            "Task not found: " + task_id,
            "vani.runtime.task_manager",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    it->second.state = contracts::TaskState::Failed;
    it->second.error = error;
    it->second.updated_at_ms = now_ms;
    it->second.completed_at_ms = now_ms;

    if (repository_) {
        repository_->save(it->second);
    }

    timeout_manager_.unregister_task(task_id);
    emit_task_event(task_id, "task.failed", it->second);

    return contracts::Result<void>::ok();
}

contracts::Result<void> TaskManager::request_cancel(
    const contracts::TaskId& task_id,
    std::string_view reason
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_tasks_.find(task_id);
    if (it == active_tasks_.end()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::NotFound,
            "Task not found: " + task_id,
            "vani.runtime.task_manager",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    if (it->second.is_terminal()) {
        return contracts::Result<void>::ok(); // Already finished
    }

    auto src_it = cancellation_sources_.find(task_id);
    if (src_it != cancellation_sources_.end()) {
        src_it->second.cancel();
    }

    const auto now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    it->second.state = contracts::TaskState::Cancelled;
    it->second.error = contracts::Error::make(
        contracts::ErrorCode::Cancelled,
        reason,
        "vani.runtime.task_manager",
        false,
        contracts::ErrorCategory::Cancelled
    );
    it->second.updated_at_ms = now_ms;
    it->second.completed_at_ms = now_ms;

    if (repository_) {
        repository_->save(it->second);
    }

    timeout_manager_.unregister_task(task_id);
    emit_task_event(task_id, "task.cancelled", it->second);

    return contracts::Result<void>::ok();
}

contracts::CancellationToken TaskManager::get_cancellation_token(const contracts::TaskId& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cancellation_sources_.find(task_id);
    if (it != cancellation_sources_.end()) {
        return it->second.token();
    }
    return contracts::CancellationToken::none();
}

void TaskManager::set_checkpoint(const contracts::TaskId& task_id, std::string checkpoint_data) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_tasks_.find(task_id);
    if (it != active_tasks_.end()) {
        it->second.checkpoint = std::move(checkpoint_data);
        if (repository_) {
            repository_->save(it->second);
        }
    }
}

void TaskManager::check_all_timeouts(uint64_t current_time_ms) {
    timeout_manager_.check_timeouts(current_time_ms);
}

void TaskManager::emit_task_event(
    const contracts::TaskId& task_id,
    const std::string& event_type,
    const contracts::Task& task
) {
    if (!event_bus_) return;

    auto event = std::make_shared<contracts::Event>(
        contracts::EventHeader{
            .event_id = "evt_task_" + task_id + "_" + std::to_string(task.updated_at_ms),
            .event_type = event_type,
            .event_version = {1, 0, 0},
            .timestamp_ms = task.updated_at_ms,
            .source = "vani.runtime.task_manager",
            .session_id = task.spec.session_id,
            .task_id = task_id,
            .correlation_id = "corr_" + task_id,
            .category = contracts::EventCategory::Task
        }
    );

    event_bus_->publish(event, DeliveryGuarantee::Durable);
}

} // namespace vani::runtime
