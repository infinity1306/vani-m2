#include "agent_memory.hpp"
#include <chrono>
#include <regex>
#include <sstream>

namespace vani::runtime::agent {

ShortTermTaskMemory::ShortTermTaskMemory(uint32_t max_tokens)
    : max_tokens_(max_tokens) {}

std::string ShortTermTaskMemory::scrub_sensitive_data(const std::string& text) {
    std::string scrubbed = text;
    // Patterns for passwords, bearer tokens, api keys
    static const std::vector<std::pair<std::regex, std::string>> patterns = {
        {std::regex(R"((password|passwd|pwd)\s*[:=]\s*["']?([^\s,"']+)["']?)", std::regex::icase), "$1=[REDACTED]"},
        {std::regex(R"((bearer\s+)([A-Za-z0-9\-\._~\+\/]+=*))", std::regex::icase), "$1[REDACTED_TOKEN]"},
        {std::regex(R"((api[_-]?key|secret|token)\s*[:=]\s*["']?([^\s,"']+)["']?)", std::regex::icase), "$1=[REDACTED]"},
        {std::regex(R"((ghp_[A-Za-z0-9]{36}))", std::regex::icase), "[REDACTED_GITHUB_TOKEN]"}
    };

    for (const auto& [re, repl] : patterns) {
        scrubbed = std::regex_replace(scrubbed, re, repl);
    }
    return scrubbed;
}

void ShortTermTaskMemory::add_entry(
    const std::string& key,
    const std::string& content,
    const std::string& step_id
) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t now_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::string safe_content = scrub_sensitive_data(content);
    entries_.push_back({key, safe_content, step_id, now_ms});
    compact_if_needed();
}

void ShortTermTaskMemory::add_observation(const contracts::AgentObservation& obs) {
    std::ostringstream ss;
    ss << "step=" << obs.step_id << " success=" << (obs.success ? "true" : "false");
    if (!obs.result_data.empty()) ss << " data=" << obs.result_data;
    if (!obs.evidence.empty()) ss << " evidence=" << obs.evidence;
    if (!obs.error.empty()) ss << " error=" << obs.error;

    add_entry("observation:" + obs.step_id, ss.str(), obs.step_id);
}

std::vector<MemoryEntry> ShortTermTaskMemory::entries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
}

size_t ShortTermTaskMemory::entry_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

uint32_t ShortTermTaskMemory::estimated_tokens() const {
    // Standard rule of thumb: ~4 characters per token
    size_t total_chars = 0;
    for (const auto& e : entries_) {
        total_chars += e.key.size() + e.content.size();
    }
    return static_cast<uint32_t>(total_chars / 4);
}

void ShortTermTaskMemory::compact_if_needed() {
    // If estimated tokens exceed max_tokens, compact older half of entries into a summary
    if (estimated_tokens() > max_tokens_ && entries_.size() > 4) {
        size_t to_compact = entries_.size() / 2;
        std::ostringstream summary;
        summary << "[COMPACTED SUMMARY of " << to_compact << " earlier entries]: ";

        for (size_t i = 0; i < to_compact; ++i) {
            summary << entries_[i].key << "; ";
        }

        std::vector<MemoryEntry> new_entries;
        new_entries.push_back({"compacted_summary", summary.str(), "compactor", entries_[0].timestamp_ms});

        for (size_t i = to_compact; i < entries_.size(); ++i) {
            new_entries.push_back(entries_[i]);
        }
        entries_ = std::move(new_entries);
    }
}

void ShortTermTaskMemory::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

} // namespace vani::runtime::agent
