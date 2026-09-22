#pragma once

#include "repository.hpp"
#include "../contracts/tasks/task.hpp"
#include <memory>

namespace vani::storage {

class ITaskRepository : public IRepository<contracts::Task, contracts::TaskId> {
public:
    [[nodiscard]] virtual std::vector<contracts::Task> find_by_session(const contracts::SessionId& session_id) const = 0;
    [[nodiscard]] virtual std::vector<contracts::Task> find_by_state(contracts::TaskState state) const = 0;
};

using TaskRepositoryPtr = std::shared_ptr<ITaskRepository>;

} // namespace vani::storage
