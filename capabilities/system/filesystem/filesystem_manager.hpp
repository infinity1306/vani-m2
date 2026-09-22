#pragma once

#include "path_security.hpp"
#include "file_transaction.hpp"
#include "file_watcher.hpp"
#include "../../../adapters/system/system_adapter.hpp"
#include "../../../contracts/common/result.hpp"
#include <memory>
#include <string>
#include <vector>

namespace vani::capabilities::system {

class FilesystemManager {
public:
    FilesystemManager(
        adapters::system::SystemAdapterPtr adapter,
        FileTransactionManagerPtr tx_manager = nullptr,
        FileWatcherPtr watcher = nullptr
    );
    ~FilesystemManager() = default;

    contracts::Result<std::string> read_file(const std::string& raw_path, const std::string& authorized_scope = "*");
    contracts::Result<void> write_file(
        const std::string& raw_path,
        const std::string& content,
        const std::string& authorized_scope = "*",
        const std::string& transaction_id = ""
    );
    contracts::Result<void> create_directory(const std::string& raw_path, const std::string& authorized_scope = "*");
    contracts::Result<void> move_file(
        const std::string& source_path,
        const std::string& dest_path,
        const std::string& authorized_scope = "*",
        const std::string& transaction_id = ""
    );
    contracts::Result<void> copy_file(
        const std::string& source_path,
        const std::string& dest_path,
        const std::string& authorized_scope = "*"
    );
    contracts::Result<void> rename_file(
        const std::string& old_path,
        const std::string& new_path,
        const std::string& authorized_scope = "*",
        const std::string& transaction_id = ""
    );
    contracts::Result<void> delete_file(
        const std::string& raw_path,
        bool use_trash = true,
        const std::string& authorized_scope = "*",
        const std::string& transaction_id = ""
    );
    contracts::Result<contracts::FileMetadata> get_metadata(
        const std::string& raw_path,
        const std::string& authorized_scope = "*"
    );
    contracts::Result<std::vector<contracts::FileMetadata>> search_files(
        const std::string& root_path,
        const std::string& pattern,
        const std::string& authorized_scope = "*"
    );

    contracts::Result<void> rollback_transaction(const std::string& transaction_id);

    [[nodiscard]] FileTransactionManagerPtr transaction_manager() const noexcept { return tx_manager_; }
    [[nodiscard]] FileWatcherPtr watcher() const noexcept { return watcher_; }

private:
    adapters::system::SystemAdapterPtr adapter_;
    FileTransactionManagerPtr tx_manager_;
    FileWatcherPtr watcher_;
};

using FilesystemManagerPtr = std::shared_ptr<FilesystemManager>;

} // namespace vani::capabilities::system
