#include "persistent_memory_provider.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>

namespace vani::adapters::memory {

namespace fs = std::filesystem;

static uint64_t current_time_ms() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

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

static std::string unescape_json(const std::string& str) {
    std::string out;
    out.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            char next = str[i + 1];
            if (next == '"') { out += '"'; ++i; }
            else if (next == '\\') { out += '\\'; ++i; }
            else if (next == 'n') { out += '\n'; ++i; }
            else if (next == 'r') { out += '\r'; ++i; }
            else if (next == 't') { out += '\t'; ++i; }
            else { out += str[i]; }
        } else {
            out += str[i];
        }
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
        return unescape_json(obj.substr(pos + 1, end_pos - pos - 1));
    } else {
        auto end_pos = obj.find_first_of(",}\r\n", pos);
        if (end_pos == std::string::npos) end_pos = obj.size();
        std::string val = obj.substr(pos, end_pos - pos);
        // trim whitespace
        val.erase(0, val.find_first_not_of(" \t"));
        val.erase(val.find_last_not_of(" \t") + 1);
        return val;
    }
}

PersistentMemoryProvider::PersistentMemoryProvider(const std::string& storage_path)
    : storage_path_(storage_path) {
    load_from_disk();
}

void PersistentMemoryProvider::load_from_disk() {
    std::lock_guard<std::mutex> lock(mutex_);
    items_.clear();

    if (!fs::exists(storage_path_)) {
        return;
    }

    std::ifstream file(storage_path_);
    if (!file.is_open()) return;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Parse array of objects
    size_t pos = 0;
    while ((pos = content.find('{', pos)) != std::string::npos) {
        size_t end_pos = content.find('}', pos);
        if (end_pos == std::string::npos) break;

        std::string obj = content.substr(pos, end_pos - pos + 1);

        contracts::MemoryItem item;
        item.id = extract_field(obj, "id");
        if (!item.id.empty()) {
            std::string tier_str = extract_field(obj, "tier");
            try { item.tier = static_cast<contracts::MemoryTier>(std::stoi(tier_str)); } catch (...) {}
            item.key = extract_field(obj, "key");
            item.content = extract_field(obj, "content");
            item.source = extract_field(obj, "source");
            std::string conf_str = extract_field(obj, "confidence");
            try { item.confidence = std::stof(conf_str); } catch (...) { item.confidence = 1.0f; }
            std::string cr_str = extract_field(obj, "created_at_ms");
            try { item.created_at_ms = std::stoull(cr_str); } catch (...) {}
            std::string la_str = extract_field(obj, "last_accessed_ms");
            try { item.last_accessed_ms = std::stoull(la_str); } catch (...) {}
            item.is_pinned = (extract_field(obj, "is_pinned") == "true" || extract_field(obj, "is_pinned") == "1");
            item.transparency_reason = extract_field(obj, "transparency_reason");

            items_[item.id] = item;
        }

        pos = end_pos + 1;
    }
}

void PersistentMemoryProvider::save_to_disk() {
    try {
        fs::path p(storage_path_);
        if (p.has_parent_path() && !fs::exists(p.parent_path())) {
            fs::create_directories(p.parent_path());
        }

        std::ofstream file(storage_path_, std::ios::trunc);
        if (!file.is_open()) return;

        file << "[\n";
        bool first = true;
        for (const auto& [id, item] : items_) {
            if (!first) file << ",\n";
            first = false;
            file << "  {\n";
            file << "    \"id\": \"" << escape_json(item.id) << "\",\n";
            file << "    \"tier\": " << static_cast<int>(item.tier) << ",\n";
            file << "    \"key\": \"" << escape_json(item.key) << "\",\n";
            file << "    \"content\": \"" << escape_json(item.content) << "\",\n";
            file << "    \"source\": \"" << escape_json(item.source) << "\",\n";
            file << "    \"confidence\": " << item.confidence << ",\n";
            file << "    \"created_at_ms\": " << item.created_at_ms << ",\n";
            file << "    \"last_accessed_ms\": " << item.last_accessed_ms << ",\n";
            file << "    \"is_pinned\": " << (item.is_pinned ? "true" : "false") << ",\n";
            file << "    \"transparency_reason\": \"" << escape_json(item.transparency_reason) << "\"\n";
            file << "  }";
        }
        file << "\n]\n";
    } catch (const std::exception& e) {
        std::cerr << "[Memory] Failed to save memory store to disk: " << e.what() << "\n";
    }
}

contracts::Result<void> PersistentMemoryProvider::store(const contracts::MemoryItem& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    contracts::MemoryItem to_save = item;

    if (to_save.id.empty()) {
        to_save.id = "mem_" + std::to_string(current_time_ms()) + "_" + std::to_string(next_id_seq_++);
    }
    if (to_save.created_at_ms == 0) {
        to_save.created_at_ms = current_time_ms();
    }
    to_save.last_accessed_ms = current_time_ms();

    items_[to_save.id] = to_save;
    save_to_disk();

    return contracts::Result<void>::success();
}

contracts::Result<std::vector<contracts::MemoryItem>> PersistentMemoryProvider::query(const contracts::MemoryQuery& query) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<float, contracts::MemoryItem>> scored_items;

    std::string lower_query = query.text_query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

    for (auto& [id, item] : items_) {
        // Check tier filtering
        if (!query.allowed_tiers.empty()) {
            bool tier_allowed = false;
            for (auto t : query.allowed_tiers) {
                if (t == item.tier) {
                    tier_allowed = true;
                    break;
                }
            }
            if (!tier_allowed) continue;
        }

        // Substring & relevance scoring
        std::string lower_key = item.key;
        std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);

        std::string lower_content = item.content;
        std::transform(lower_content.begin(), lower_content.end(), lower_content.begin(), ::tolower);

        float score = 0.0f;
        if (lower_query.empty()) {
            score = 1.0f;
        } else {
            if (lower_key == lower_query) score += 1.0f;
            else if (lower_key.find(lower_query) != std::string::npos) score += 0.8f;

            if (lower_content.find(lower_query) != std::string::npos) score += 0.7f;
        }

        if (item.is_pinned) score += 0.2f;

        if (score >= query.min_relevance || lower_query.empty()) {
            item.last_accessed_ms = current_time_ms();
            scored_items.emplace_back(score, item);
        }
    }

    std::sort(scored_items.begin(), scored_items.end(), [](const auto& a, const auto& b) {
        if (a.first != b.first) return a.first > b.first;
        return a.second.created_at_ms > b.second.created_at_ms;
    });

    std::vector<contracts::MemoryItem> results;
    size_t limit = std::min(static_cast<size_t>(query.max_results), scored_items.size());
    for (size_t i = 0; i < limit; ++i) {
        results.push_back(scored_items[i].second);
    }

    return contracts::Result<std::vector<contracts::MemoryItem>>::success(results);
}

contracts::Result<void> PersistentMemoryProvider::forget(const std::string& memory_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = items_.find(memory_id);
    if (it != items_.end()) {
        items_.erase(it);
        save_to_disk();
        return contracts::Result<void>::success();
    }
    return contracts::Result<void>::failure(
        contracts::ErrorCode::NotFound, "Memory item not found: " + memory_id
    );
}

contracts::Result<void> PersistentMemoryProvider::clear_tier(contracts::MemoryTier tier) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = items_.begin(); it != items_.end();) {
        if (it->second.tier == tier) {
            it = items_.erase(it);
        } else {
            ++it;
        }
    }
    save_to_disk();
    return contracts::Result<void>::success();
}

std::vector<contracts::MemoryItem> PersistentMemoryProvider::list_all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<contracts::MemoryItem> res;
    res.reserve(items_.size());
    for (const auto& [id, item] : items_) {
        res.push_back(item);
    }
    return res;
}

size_t PersistentMemoryProvider::count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return items_.size();
}

} // namespace vani::adapters::memory
