#include "context_manager.hpp"

namespace vani::runtime {

ContextManager::ContextManager() = default;
ContextManager::~ContextManager() = default;

std::string ContextManager::make_key(ContextScopeType scope_type, const std::string& scope_id, const std::string& key) const {
    return std::string(to_string(scope_type)) + ":" + scope_id + ":" + key;
}

contracts::Result<void> ContextManager::set(
    ContextScopeType scope_type,
    const std::string& scope_id,
    const std::string& key,
    const std::string& value,
    bool is_sensitive
) {
    if (key.empty()) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::ValidationError,
            "Context key cannot be empty",
            "vani.runtime.context",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto storage_key = make_key(scope_type, scope_id, key);
    entries_[storage_key] = ContextEntry{
        .scope_type = scope_type,
        .scope_id = scope_id,
        .key = key,
        .value = value,
        .is_sensitive = is_sensitive
    };

    return contracts::Result<void>::ok();
}

std::optional<std::string> ContextManager::get(
    ContextScopeType scope_type,
    const std::string& scope_id,
    const std::string& key
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto storage_key = make_key(scope_type, scope_id, key);
    auto it = entries_.find(storage_key);
    if (it != entries_.end()) {
        return it->second.value;
    }
    return std::nullopt;
}

contracts::Result<void> ContextManager::remove(
    ContextScopeType scope_type,
    const std::string& scope_id,
    const std::string& key
) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto storage_key = make_key(scope_type, scope_id, key);
    entries_.erase(storage_key);
    return contracts::Result<void>::ok();
}

contracts::Result<void> ContextManager::clear_scope(
    ContextScopeType scope_type,
    const std::string& scope_id
) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto prefix = std::string(to_string(scope_type)) + ":" + scope_id + ":";
    for (auto it = entries_.begin(); it != entries_.end();) {
        if (it->first.starts_with(prefix)) {
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
    return contracts::Result<void>::ok();
}

std::unordered_map<std::string, std::string> ContextManager::get_scoped_context(
    ContextScopeType scope_type,
    const std::string& scope_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, std::string> result;
    const auto prefix = std::string(to_string(scope_type)) + ":" + scope_id + ":";
    for (const auto& [storage_key, entry] : entries_) {
        if (storage_key.starts_with(prefix)) {
            result[entry.key] = entry.value;
        }
    }
    return result;
}

} // namespace vani::runtime
