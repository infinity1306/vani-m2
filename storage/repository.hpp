#pragma once

#include "../contracts/common/result.hpp"
#include <string>
#include <vector>
#include <optional>

namespace vani::storage {

template <typename Entity, typename IdType>
class IRepository {
public:
    virtual ~IRepository() = default;

    virtual contracts::Result<void> save(const Entity& entity) = 0;
    [[nodiscard]] virtual std::optional<Entity> find_by_id(const IdType& id) const = 0;
    [[nodiscard]] virtual std::vector<Entity> find_all() const = 0;
    virtual contracts::Result<void> remove(const IdType& id) = 0;
    [[nodiscard]] virtual size_t count() const = 0;
};

} // namespace vani::storage
