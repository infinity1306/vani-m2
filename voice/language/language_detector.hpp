#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace vani::voice::language {

enum class LanguageCode : uint8_t {
    English,
    Hindi,
    Hinglish,
    Mixed,
    Unknown
};

struct LanguageDetectionResult {
    LanguageCode code{LanguageCode::Unknown};
    std::string iso_code{"unknown"};
    float confidence{0.0f};
    bool is_hinglish{false};
};

class LanguageDetector {
public:
    static LanguageDetectionResult detect(const std::string& text) {
        if (text.empty()) {
            return {LanguageCode::Unknown, "unknown", 0.0f, false};
        }

        std::string lower = text;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Check for Devanagari script (Hindi)
        bool has_devanagari = false;
        for (size_t i = 0; i < text.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(text[i]);
            if (c >= 0xE0 && c <= 0xEF) { // Basic UTF-8 range for Indic/Devanagari
                has_devanagari = true;
                break;
            }
        }

        if (has_devanagari) {
            return {LanguageCode::Hindi, "hi", 0.95f, false};
        }

        // Check for Hinglish marker words
        static const std::vector<std::string> hinglish_markers = {
            "mera", "meri", "karo", "karde", "kardo", "karna", "hai", "hain", "kya", "kaise",
            "wala", "wali", "batao", "dikhao", "chalao", "kholo", "bhai", "yaar", "namaste",
            "lekin", "kyunki", "chahiye", "accha", "theek", "aur", "pe", "par", "se"
        };

        size_t match_count = 0;
        for (const auto& marker : hinglish_markers) {
            if (lower.find(marker) != std::string::npos) {
                match_count++;
            }
        }

        if (match_count >= 1) {
            return {LanguageCode::Hinglish, "hinglish", 0.90f, true};
        }

        return {LanguageCode::English, "en", 0.85f, false};
    }
};

} // namespace vani::voice::language
