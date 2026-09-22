#pragma once

#include <string>
#include <string_view>
#include <mutex>
#include <iostream>
#include <chrono>
#include <format>

namespace vani::observability {

enum class LogLevel : uint8_t {
    Debug = 0,
    Info = 1,
    Warn = 2,
    Error = 3,
    Fatal = 4
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void set_level(LogLevel level) noexcept {
        level_ = level;
    }

    void log(
        LogLevel level,
        std::string_view component,
        std::string_view message,
        std::string_view correlation_id = ""
    ) {
        if (level < level_) return;

        const auto now = std::chrono::system_clock::now();
        const auto time_t_now = std::chrono::system_clock::to_time_t(now);

        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "[" << level_string(level) << "]"
                  << " [" << component << "]"
                  << (correlation_id.empty() ? "" : " [corr:" + std::string(correlation_id) + "]")
                  << " " << message << "\n";
    }

    void debug(std::string_view comp, std::string_view msg, std::string_view corr = "") {
        log(LogLevel::Debug, comp, msg, corr);
    }
    void info(std::string_view comp, std::string_view msg, std::string_view corr = "") {
        log(LogLevel::Info, comp, msg, corr);
    }
    void warn(std::string_view comp, std::string_view msg, std::string_view corr = "") {
        log(LogLevel::Warn, comp, msg, corr);
    }
    void error(std::string_view comp, std::string_view msg, std::string_view corr = "") {
        log(LogLevel::Error, comp, msg, corr);
    }

private:
    Logger() : level_(LogLevel::Info) {}

    static constexpr std::string_view level_string(LogLevel l) noexcept {
        switch (l) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warn: return "WARN";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Fatal: return "FATAL";
            default: return "LOG";
        }
    }

    LogLevel level_;
    std::mutex mutex_;
};

} // namespace vani::observability
