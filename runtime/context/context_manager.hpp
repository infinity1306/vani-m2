#pragma once

#include "context_scope.hpp"
#include "../../contracts/common/result.hpp"
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <optional>

namespace vani::runtime {

struct ContextEntry {
    ContextScopeType scope_type;
    std::string scope_id;
    std::string key;
    std::string value;
    bool is_sensitive{false};
};

class ContextManager {
public:
    ContextManager();
    ~ContextManager();

    contracts::Result<void> set(
        ContextScopeType scope_type,
        const std::string& scope_id,
        const std::string& key,
        const std::string& value,
        bool is_sensitive = false
    );

    [[nodiscard]] std::optional<std::string> get(
        ContextScopeType scope_type,
        const std::string& scope_id,
        const std::string& key
    ) const;

    contracts::Result<void> remove(
        ContextScopeType scope_type,
        const std::string& scope_id,
        const std::string& key
    );

    contracts::Result<void> clear_scope(
        ContextScopeType scope_type,
        const std::string& scope_id
    );

    [[nodiscard]] std::unordered_map<std::string, std::string> get_scoped_context(
        ContextScopeType scope_type,
        const std::string& scope_id
    ) const;

private:
    [[nodiscard]] std::string make_key(ContextScopeType scope_type, const std::string& scope_id, const std::string& key) const;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, ContextEntry> entries_;
};

using ContextManagerPtr = std::shared_ptr<ContextManager>;

} // namespace vani::runtime
