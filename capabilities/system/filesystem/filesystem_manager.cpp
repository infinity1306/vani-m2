#include "filesystem_manager.hpp"

namespace vani::capabilities::system {

FilesystemManager::FilesystemManager(
    adapters::system::SystemAdapterPtr adapter,
    FileTransactionManagerPtr tx_manager,
    FileWatcherPtr watcher
) : adapter_(std::move(adapter)),
    tx_manager_(tx_manager ? std::move(tx_manager) : std::make_shared<FileTransactionManager>()),
    watcher_(watcher ? std::move(watcher) : std::make_shared<FileWatcher>()) {}

contracts::Result<std::string> FilesystemManager::read_file(
    const std::string& raw_path,
    const std::string& authorized_scope
) {
    if (PathSecurity::is_traversal_attack(raw_path)) {
        return contracts::Result<std::string>::failure(
            contracts::ErrorCode::SecurityViolation, "Path traversal attack detected"
        );
    }

    std::string canonical = PathSecurity::canonicalize_path(raw_path);
    if (!PathSecurity::is_within_scope(canonical, authorized_scope)) {
        return contracts::Result<std::string>::failure(
            contracts::ErrorCode::PermissionDenied, "Path outside authorized scope"
        );
    }

    if (PathSecurity::is_sensitive_location(canonical)) {
        return contracts::Result<std::string>::failure(
            contracts::ErrorCode::SecurityViolation, "Reading sensitive OS path is denied"
        );
    }

    if (!adapter_) {
        return contracts::Result<std::string>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    return adapter_->read_file(canonical);
}

contracts::Result<void> FilesystemManager::write_file(
    const std::string& raw_path,
    const std::string& content,
    const std::string& authorized_scope,
    const std::string& transaction_id
) {
    if (PathSecurity::is_traversal_attack(raw_path)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::SecurityViolation, "Path traversal attack detected"
        );
    }

    std::string canonical = PathSecurity::canonicalize_path(raw_path);
    if (!PathSecurity::is_within_scope(canonical, authorized_scope)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::PermissionDenied, "Path outside authorized scope"
        );
    }

    if (PathSecurity::is_sensitive_location(canonical)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::SecurityViolation, "Writing to sensitive OS path is denied"
        );
    }

    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    // Record snapshot if in transaction
    if (!transaction_id.empty() && tx_manager_) {
        FileSnapshot snapshot;
        snapshot.path = canonical;
        auto existing = adapter_->read_file(canonical);
        if (existing.is_success()) {
            snapshot.existed_previously = true;
            snapshot.previous_content = existing.value();
        } else {
            snapshot.existed_previously = false;
        }
        tx_manager_->record_snapshot(transaction_id, snapshot);
    }

    auto res = adapter_->write_file(canonical, content);
    if (res.is_success() && watcher_) {
        watcher_->notify_event(canonical, contracts::FileWatchEvent::Modified);
    }
    return res;
}

contracts::Result<void> FilesystemManager::create_directory(
    const std::string& raw_path,
    const std::string& authorized_scope
) {
    if (PathSecurity::is_traversal_attack(raw_path)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::SecurityViolation, "Path traversal attack detected"
        );
    }

    std::string canonical = PathSecurity::canonicalize_path(raw_path);
    if (!PathSecurity::is_within_scope(canonical, authorized_scope)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::PermissionDenied, "Path outside authorized scope"
        );
    }

    if (PathSecurity::is_sensitive_location(canonical)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::SecurityViolation, "Creating directory in sensitive OS path is denied"
        );
    }

    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    auto res = adapter_->create_directory(canonical);
    if (res.is_success() && watcher_) {
        watcher_->notify_event(canonical, contracts::FileWatchEvent::Created);
    }
    return res;
}

contracts::Result<void> FilesystemManager::move_file(
    const std::string& source_path,
    const std::string& dest_path,
    const std::string& authorized_scope,
    const std::string& transaction_id
) {
    if (PathSecurity::is_traversal_attack(source_path) || PathSecurity::is_traversal_attack(dest_path)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::SecurityViolation, "Path traversal attack detected"
        );
    }

    std::string c_src = PathSecurity::canonicalize_path(source_path);
    std::string c_dest = PathSecurity::canonicalize_path(dest_path);

    if (!PathSecurity::is_within_scope(c_src, authorized_scope) || !PathSecurity::is_within_scope(c_dest, authorized_scope)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::PermissionDenied, "Path outside authorized scope"
        );
    }

    if (PathSecurity::is_sensitive_location(c_src) || PathSecurity::is_sensitive_location(c_dest)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::SecurityViolation, "Sensitive OS path access is denied"
        );
    }

    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    if (!transaction_id.empty() && tx_manager_) {
        FileSnapshot snapshot;
        snapshot.path = c_src;
        snapshot.target_path = c_dest;
        snapshot.existed_previously = true;
        auto existing = adapter_->read_file(c_src);
        if (existing.is_success()) {
            snapshot.previous_content = existing.value();
        }
        tx_manager_->record_snapshot(transaction_id, snapshot);
    }

    auto res = adapter_->move_file(c_src, c_dest);
    if (res.is_success() && watcher_) {
        watcher_->notify_event(c_src, contracts::FileWatchEvent::Deleted);
        watcher_->notify_event(c_dest, contracts::FileWatchEvent::Created);
    }
    return res;
}

contracts::Result<void> FilesystemManager::copy_file(
    const std::string& source_path,
    const std::string& dest_path,
    const std::string& authorized_scope
) {
    if (PathSecurity::is_traversal_attack(source_path) || PathSecurity::is_traversal_attack(dest_path)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::SecurityViolation, "Path traversal attack detected"
        );
    }

    std::string c_src = PathSecurity::canonicalize_path(source_path);
    std::string c_dest = PathSecurity::canonicalize_path(dest_path);

    if (!PathSecurity::is_within_scope(c_src, authorized_scope) || !PathSecurity::is_within_scope(c_dest, authorized_scope)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::PermissionDenied, "Path outside authorized scope"
        );
    }

    if (PathSecurity::is_sensitive_location(c_src) || PathSecurity::is_sensitive_location(c_dest)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::SecurityViolation, "Sensitive OS path access is denied"
        );
    }

    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    auto res = adapter_->copy_file(c_src, c_dest);
    if (res.is_success() && watcher_) {
        watcher_->notify_event(c_dest, contracts::FileWatchEvent::Created);
    }
    return res;
}

contracts::Result<void> FilesystemManager::rename_file(
    const std::string& old_path,
    const std::string& new_path,
    const std::string& authorized_scope,
    const std::string& transaction_id
) {
    return move_file(old_path, new_path, authorized_scope, transaction_id);
}

contracts::Result<void> FilesystemManager::delete_file(
    const std::string& raw_path,
    bool use_trash,
    const std::string& authorized_scope,
    const std::string& transaction_id
) {
    if (PathSecurity::is_traversal_attack(raw_path)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::SecurityViolation, "Path traversal attack detected"
        );
    }

    std::string canonical = PathSecurity::canonicalize_path(raw_path);
    if (!PathSecurity::is_within_scope(canonical, authorized_scope)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::PermissionDenied, "Path outside authorized scope"
        );
    }

    if (PathSecurity::is_sensitive_location(canonical)) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::SecurityViolation, "Deleting sensitive OS path is denied"
        );
    }

    if (!adapter_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    if (!transaction_id.empty() && tx_manager_) {
        FileSnapshot snapshot;
        snapshot.path = canonical;
        snapshot.existed_previously = true;
        auto existing = adapter_->read_file(canonical);
        if (existing.is_success()) {
            snapshot.previous_content = existing.value();
        }
        tx_manager_->record_snapshot(transaction_id, snapshot);
    }

    auto res = adapter_->delete_file(canonical, use_trash);
    if (res.is_success() && watcher_) {
        watcher_->notify_event(canonical, contracts::FileWatchEvent::Deleted);
    }
    return res;
}

contracts::Result<contracts::FileMetadata> FilesystemManager::get_metadata(
    const std::string& raw_path,
    const std::string& authorized_scope
) {
    if (PathSecurity::is_traversal_attack(raw_path)) {
        return contracts::Result<contracts::FileMetadata>::failure(
            contracts::ErrorCode::SecurityViolation, "Path traversal attack detected"
        );
    }

    std::string canonical = PathSecurity::canonicalize_path(raw_path);
    if (!PathSecurity::is_within_scope(canonical, authorized_scope)) {
        return contracts::Result<contracts::FileMetadata>::failure(
            contracts::ErrorCode::PermissionDenied, "Path outside authorized scope"
        );
    }

    if (!adapter_) {
        return contracts::Result<contracts::FileMetadata>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    return adapter_->get_file_metadata(canonical);
}

contracts::Result<std::vector<contracts::FileMetadata>> FilesystemManager::search_files(
    const std::string& root_path,
    const std::string& pattern,
    const std::string& authorized_scope
) {
    if (PathSecurity::is_traversal_attack(root_path)) {
        return contracts::Result<std::vector<contracts::FileMetadata>>::failure(
            contracts::ErrorCode::SecurityViolation, "Path traversal attack detected"
        );
    }

    std::string canonical = PathSecurity::canonicalize_path(root_path);
    if (!PathSecurity::is_within_scope(canonical, authorized_scope)) {
        return contracts::Result<std::vector<contracts::FileMetadata>>::failure(
            contracts::ErrorCode::PermissionDenied, "Path outside authorized scope"
        );
    }

    if (!adapter_) {
        return contracts::Result<std::vector<contracts::FileMetadata>>::failure(
            contracts::ErrorCode::Unavailable, "SystemAdapter unavailable"
        );
    }

    return adapter_->search_files(canonical, pattern);
}

contracts::Result<void> FilesystemManager::rollback_transaction(const std::string& transaction_id) {
    if (!tx_manager_) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::Unavailable, "Transaction manager not initialized"
        );
    }

    auto tx_opt = tx_manager_->get_transaction(transaction_id);
    if (!tx_opt) {
        return contracts::Result<void>::failure(
            contracts::ErrorCode::NotFound, "Transaction not found: " + transaction_id
        );
    }

    const auto& tx = *tx_opt;
    // Rollback in reverse order
    for (auto it = tx.snapshots.rbegin(); it != tx.snapshots.rend(); ++it) {
        const auto& snap = *it;
        if (!snap.target_path.empty()) {
            // Was a rename / move: move back
            adapter_->move_file(snap.target_path, snap.path);
        } else if (snap.existed_previously) {
            // Restore previous content
            adapter_->write_file(snap.path, snap.previous_content);
        } else {
            // Was created by the transaction: delete it
            adapter_->delete_file(snap.path, false);
        }
    }

    return contracts::Result<void>::success();
}

} // namespace vani::capabilities::system
