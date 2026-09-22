#pragma once

#include <atomic>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>

namespace vani::contracts {

class CancellationState {
public:
    CancellationState() : cancelled_(false) {}

    void request_cancellation() noexcept {
        bool expected = false;
        if (cancelled_.compare_exchange_strong(expected, true)) {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& cb : callbacks_) {
                if (cb) {
                    try { cb(); } catch (...) {}
                }
            }
            callbacks_.clear();
        }
    }

    [[nodiscard]] bool is_cancelled() const noexcept {
        return cancelled_.load(std::memory_order_relaxed);
    }

    void register_callback(std::function<void()> cb) {
        if (is_cancelled()) {
            if (cb) cb();
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_cancelled()) {
            if (cb) cb();
            return;
        }
        callbacks_.push_back(std::move(cb));
    }

private:
    std::atomic<bool> cancelled_;
    std::mutex mutex_;
    std::vector<std::function<void()>> callbacks_;
};

class CancellationToken {
public:
    CancellationToken() : state_(std::make_shared<CancellationState>()) {}
    explicit CancellationToken(std::shared_ptr<CancellationState> state) : state_(std::move(state)) {}

    [[nodiscard]] static CancellationToken none() {
        return CancellationToken(std::make_shared<CancellationState>());
    }

    [[nodiscard]] bool is_cancelled() const noexcept {
        return state_ ? state_->is_cancelled() : false;
    }

    void register_callback(std::function<void()> cb) const {
        if (state_) {
            state_->register_callback(std::move(cb));
        }
    }

    void throw_if_cancelled() const {
        if (is_cancelled()) {
            throw std::runtime_error("Operation was cancelled.");
        }
    }

private:
    std::shared_ptr<CancellationState> state_;
};

class CancellationSource {
public:
    CancellationSource() : state_(std::make_shared<CancellationState>()) {}

    void cancel() noexcept {
        if (state_) {
            state_->request_cancellation();
        }
    }

    [[nodiscard]] CancellationToken token() const noexcept {
        return CancellationToken(state_);
    }

    [[nodiscard]] bool is_cancelled() const noexcept {
        return state_ ? state_->is_cancelled() : false;
    }

private:
    std::shared_ptr<CancellationState> state_;
};

} // namespace vani::contracts
