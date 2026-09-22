#pragma once

#include "error_code.hpp"
#include <string>
#include <string_view>
#include <variant>
#include <optional>
#include <utility>
#include <stdexcept>

namespace vani::contracts {

struct Error {
    ErrorCode code{ErrorCode::Failure};
    ErrorCategory category{ErrorCategory::Internal};
    std::string message;
    std::string source{"vani.core"};
    bool retryable{false};
    ErrorSeverity severity{ErrorSeverity::Error};
    std::string details;
    std::string correlation_id;

    [[nodiscard]] static Error make(
        ErrorCode c,
        std::string_view msg,
        std::string_view src = "vani.core",
        bool retry = false,
        ErrorCategory cat = ErrorCategory::Internal,
        ErrorSeverity sev = ErrorSeverity::Error,
        std::string_view det = "",
        std::string_view corr = ""
    ) {
        return Error{
            .code = c,
            .category = cat,
            .message = std::string(msg),
            .source = std::string(src),
            .retryable = retry,
            .severity = sev,
            .details = std::string(det),
            .correlation_id = std::string(corr)
        };
    }
};

template <typename T = void>
class Result {
public:
    Result(const T& val) : data_(val) {}
    Result(T&& val) : data_(std::move(val)) {}
    Result(const Error& err) : data_(err) {}
    Result(Error&& err) : data_(std::move(err)) {}

    [[nodiscard]] static Result<T> ok(const T& val) { return Result<T>(val); }
    [[nodiscard]] static Result<T> ok(T&& val) { return Result<T>(std::move(val)); }
    [[nodiscard]] static Result<T> success(const T& val) { return Result<T>(val); }
    [[nodiscard]] static Result<T> success(T&& val) { return Result<T>(std::move(val)); }

    [[nodiscard]] static Result<T> err(const Error& e) { return Result<T>(e); }
    [[nodiscard]] static Result<T> failure(const Error& e) { return Result<T>(e); }
    [[nodiscard]] static Result<T> err(
        ErrorCode c,
        std::string_view msg,
        std::string_view src = "vani.core",
        bool retry = false,
        ErrorCategory cat = ErrorCategory::Internal,
        ErrorSeverity sev = ErrorSeverity::Error,
        std::string_view det = "",
        std::string_view corr = ""
    ) {
        return Result<T>(Error::make(c, msg, src, retry, cat, sev, det, corr));
    }
    [[nodiscard]] static Result<T> failure(
        ErrorCode c,
        std::string_view msg,
        std::string_view src = "vani.core",
        bool retry = false,
        ErrorCategory cat = ErrorCategory::Internal,
        ErrorSeverity sev = ErrorSeverity::Error,
        std::string_view det = "",
        std::string_view corr = ""
    ) {
        return Result<T>(Error::make(c, msg, src, retry, cat, sev, det, corr));
    }

    [[nodiscard]] bool is_ok() const noexcept {
        return std::holds_alternative<T>(data_);
    }
    [[nodiscard]] bool is_success() const noexcept {
        return is_ok();
    }

    [[nodiscard]] bool is_err() const noexcept {
        return std::holds_alternative<Error>(data_);
    }
    [[nodiscard]] bool is_failure() const noexcept {
        return is_err();
    }

    explicit operator bool() const noexcept {
        return is_ok();
    }

    [[nodiscard]] const T& value() const & {
        if (is_err()) {
            throw std::runtime_error("Attempted to access value of failed Result: " + error().message);
        }
        return std::get<T>(data_);
    }

    [[nodiscard]] T& value() & {
        if (is_err()) {
            throw std::runtime_error("Attempted to access value of failed Result: " + error().message);
        }
        return std::get<T>(data_);
    }

    [[nodiscard]] T value_or(T fallback) const {
        if (is_ok()) return std::get<T>(data_);
        return fallback;
    }

    [[nodiscard]] const Error& error() const {
        if (is_ok()) {
            throw std::runtime_error("Attempted to access error of successful Result");
        }
        return std::get<Error>(data_);
    }

private:
    std::variant<T, Error> data_;
};

// Specialization for void Result
template <>
class Result<void> {
public:
    Result() : error_(std::nullopt) {}
    Result(const Error& err) : error_(err) {}
    Result(Error&& err) : error_(std::move(err)) {}

    [[nodiscard]] static Result<void> ok() noexcept {
        return Result<void>();
    }
    [[nodiscard]] static Result<void> success() noexcept {
        return Result<void>();
    }

    [[nodiscard]] static Result<void> err(const Error& e) {
        return Result<void>(e);
    }
    [[nodiscard]] static Result<void> failure(const Error& e) {
        return Result<void>(e);
    }

    [[nodiscard]] static Result<void> err(
        ErrorCode c,
        std::string_view msg,
        std::string_view src = "vani.core",
        bool retry = false,
        ErrorCategory cat = ErrorCategory::Internal,
        ErrorSeverity sev = ErrorSeverity::Error,
        std::string_view det = "",
        std::string_view corr = ""
    ) {
        return Result<void>(Error::make(c, msg, src, retry, cat, sev, det, corr));
    }
    [[nodiscard]] static Result<void> failure(
        ErrorCode c,
        std::string_view msg,
        std::string_view src = "vani.core",
        bool retry = false,
        ErrorCategory cat = ErrorCategory::Internal,
        ErrorSeverity sev = ErrorSeverity::Error,
        std::string_view det = "",
        std::string_view corr = ""
    ) {
        return Result<void>(Error::make(c, msg, src, retry, cat, sev, det, corr));
    }

    [[nodiscard]] bool is_ok() const noexcept {
        return !error_.has_value();
    }
    [[nodiscard]] bool is_success() const noexcept {
        return is_ok();
    }

    [[nodiscard]] bool is_err() const noexcept {
        return error_.has_value();
    }
    [[nodiscard]] bool is_failure() const noexcept {
        return is_err();
    }

    explicit operator bool() const noexcept {
        return is_ok();
    }

    [[nodiscard]] const Error& error() const {
        if (is_ok()) {
            throw std::runtime_error("Attempted to access error of successful Result<void>");
        }
        return *error_;
    }

private:
    std::optional<Error> error_;
};

inline Result<void> Ok() noexcept {
    return Result<void>::ok();
}

template <typename T>
inline Result<std::decay_t<T>> Ok(T&& val) {
    return Result<std::decay_t<T>>::ok(std::forward<T>(val));
}

inline Error Fail(
    ErrorCode code,
    std::string_view message,
    std::string_view source = "vani.core"
) {
    return Error::make(code, message, source);
}

} // namespace vani::contracts
