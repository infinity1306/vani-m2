#pragma once

#include "scheduler_repository.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <mutex>

namespace vani::storage {

namespace fs = std::filesystem;

class FileSchedulerRepository : public ISchedulerRepository {
public:
    explicit FileSchedulerRepository(std::string storage_path = "storage/scheduler/scheduler_jobs.json")
        : storage_path_(std::move(storage_path)) {
        load_from_disk();
    }

    contracts::Result<void> save(const runtime::ScheduledJob& job) override {
        std::lock_guard<std::mutex> lock(mutex_);
        jobs_[job.job_id] = job;
        save_to_disk();
        return contracts::Result<void>::ok();
    }

    [[nodiscard]] std::optional<runtime::ScheduledJob> find_by_id(const std::string& id) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = jobs_.find(id);
        if (it != jobs_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::vector<runtime::ScheduledJob> find_all() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<runtime::ScheduledJob> result;
        result.reserve(jobs_.size());
        for (const auto& [id, job] : jobs_) {
            result.push_back(job);
        }
        return result;
    }

    [[nodiscard]] std::vector<runtime::ScheduledJob> find_due_jobs(uint64_t current_time_ms) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<runtime::ScheduledJob> result;
        for (const auto& [id, job] : jobs_) {
            if (job.is_enabled && job.target_time_ms <= current_time_ms) {
                result.push_back(job);
            }
        }
        return result;
    }

    contracts::Result<void> remove(const std::string& id) override {
        std::lock_guard<std::mutex> lock(mutex_);
        jobs_.erase(id);
        save_to_disk();
        return contracts::Result<void>::ok();
    }

    [[nodiscard]] size_t count() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return jobs_.size();
    }

private:
    static std::string escape_json(const std::string& str) {
        std::string out;
        out.reserve(str.size() + 16);
        for (char c : str) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else out += c;
        }
        return out;
    }

    static std::string extract_field(const std::string& obj, const std::string& key) {
        std::string pattern = "\"" + key + "\":";
        auto pos = obj.find(pattern);
        if (pos == std::string::npos) return "";
        pos += pattern.size();
        while (pos < obj.size() && (obj[pos] == ' ' || obj[pos] == '\t')) pos++;
        if (pos >= obj.size()) return "";

        if (obj[pos] == '"') {
            auto end_pos = obj.find('"', pos + 1);
            while (end_pos != std::string::npos && obj[end_pos - 1] == '\\') {
                end_pos = obj.find('"', end_pos + 1);
            }
            if (end_pos == std::string::npos) return "";
            return obj.substr(pos + 1, end_pos - pos - 1);
        } else {
            auto end_pos = obj.find_first_of(",}\r\n", pos);
            if (end_pos == std::string::npos) end_pos = obj.size();
            std::string val = obj.substr(pos, end_pos - pos);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t") + 1);
            return val;
        }
    }

    void load_from_disk() {
        jobs_.clear();
        if (!fs::exists(storage_path_)) return;

        std::ifstream file(storage_path_);
        if (!file.is_open()) return;

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        size_t pos = 0;
        while ((pos = content.find('{', pos)) != std::string::npos) {
            size_t end_pos = content.find('}', pos);
            if (end_pos == std::string::npos) break;

            std::string obj = content.substr(pos, end_pos - pos + 1);
            std::string job_id = extract_field(obj, "job_id");
            if (!job_id.empty()) {
                runtime::ScheduledJob job;
                job.job_id = job_id;
                job.task_spec.title = extract_field(obj, "title");
                job.task_spec.description = extract_field(obj, "description");
                std::string target_str = extract_field(obj, "target_time_ms");
                try { job.target_time_ms = std::stoull(target_str); } catch (...) {}
                std::string interval_str = extract_field(obj, "interval_ms");
                try { job.interval_ms = std::stoull(interval_str); } catch (...) {}
                job.is_recurring = (extract_field(obj, "is_recurring") == "true" || extract_field(obj, "is_recurring") == "1");
                job.is_enabled = (extract_field(obj, "is_enabled") == "true" || extract_field(obj, "is_enabled") == "1");
                std::string cr_str = extract_field(obj, "created_at_ms");
                try { job.created_at_ms = std::stoull(cr_str); } catch (...) {}
                std::string la_str = extract_field(obj, "last_executed_ms");
                try { job.last_executed_ms = std::stoull(la_str); } catch (...) {}

                jobs_[job_id] = job;
            }
            pos = end_pos + 1;
        }
    }

    void save_to_disk() {
        try {
            fs::path p(storage_path_);
            if (p.has_parent_path() && !fs::exists(p.parent_path())) {
                fs::create_directories(p.parent_path());
            }

            std::ofstream file(storage_path_, std::ios::trunc);
            if (!file.is_open()) return;

            file << "[\n";
            bool first = true;
            for (const auto& [id, job] : jobs_) {
                if (!first) file << ",\n";
                first = false;
                file << "  {\n";
                file << "    \"job_id\": \"" << escape_json(job.job_id) << "\",\n";
                file << "    \"title\": \"" << escape_json(job.task_spec.title) << "\",\n";
                file << "    \"description\": \"" << escape_json(job.task_spec.description) << "\",\n";
                file << "    \"target_time_ms\": " << job.target_time_ms << ",\n";
                file << "    \"interval_ms\": " << job.interval_ms << ",\n";
                file << "    \"is_recurring\": " << (job.is_recurring ? "true" : "false") << ",\n";
                file << "    \"is_enabled\": " << (job.is_enabled ? "true" : "false") << ",\n";
                file << "    \"created_at_ms\": " << job.created_at_ms << ",\n";
                file << "    \"last_executed_ms\": " << job.last_executed_ms << "\n";
                file << "  }";
            }
            file << "\n]\n";
        } catch (...) {}
    }

    std::string storage_path_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, runtime::ScheduledJob> jobs_;
};

} // namespace vani::storage
