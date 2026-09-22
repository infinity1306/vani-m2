#include "event_bus.hpp"
#include <iostream>

namespace vani::runtime {

EventBus::EventBus(size_t worker_threads, size_t max_queue_size)
    : max_queue_size_(max_queue_size > 0 ? max_queue_size : 1000),
      worker_thread_count_(worker_threads > 0 ? worker_threads : 1) {}

EventBus::~EventBus() {
    stop();
}

contracts::Result<void> EventBus::start() {
    if (running_.exchange(true)) {
        return contracts::Result<void>::ok();
    }

    for (size_t i = 0; i < worker_thread_count_; ++i) {
        workers_.emplace_back(&EventBus::worker_loop, this);
    }
    return contracts::Result<void>::ok();
}

contracts::Result<void> EventBus::stop() {
    if (!running_.exchange(false)) {
        return contracts::Result<void>::ok();
    }

    cv_.notify_all();
    for (auto& w : workers_) {
        if (w.joinable()) {
            w.join();
        }
    }
    workers_.clear();

    return contracts::Result<void>::ok();
}

contracts::Result<void> EventBus::publish(
    contracts::EventPtr event,
    DeliveryGuarantee guarantee,
    BackpressureStrategy strategy
) {
    if (!event) {
        return contracts::Result<void>::err(
            contracts::ErrorCode::ValidationError,
            "Cannot publish null event",
            "vani.runtime.event_bus",
            false,
            contracts::ErrorCategory::Validation
        );
    }

    {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        // Critical events are never rejected
        if (guarantee == DeliveryGuarantee::Critical) {
            event_queue_.push_back(EventEnvelope{
                .event = std::move(event),
                .guarantee = guarantee,
                .strategy = strategy
            });
            published_count_.fetch_add(1, std::memory_order_relaxed);
            lock.unlock();
            cv_.notify_one();
            return contracts::Result<void>::ok();
        }

        // Apply backpressure strategy if queue is full
        if (event_queue_.size() >= max_queue_size_) {
            switch (strategy) {
                case BackpressureStrategy::DropOldest:
                    if (!event_queue_.empty()) {
                        event_queue_.pop_front();
                        dropped_count_.fetch_add(1, std::memory_order_relaxed);
                    }
                    event_queue_.push_back(EventEnvelope{
                        .event = std::move(event),
                        .guarantee = guarantee,
                        .strategy = strategy
                    });
                    published_count_.fetch_add(1, std::memory_order_relaxed);
                    break;

                case BackpressureStrategy::DropNewest:
                    dropped_count_.fetch_add(1, std::memory_order_relaxed);
                    return contracts::Result<void>::ok();

                case BackpressureStrategy::Fail:
                    return contracts::Result<void>::err(
                        contracts::ErrorCode::ResourceExhausted,
                        "Event queue full; backpressure strategy FAIL triggered",
                        "vani.runtime.event_bus",
                        true,
                        contracts::ErrorCategory::Resource
                    );

                case BackpressureStrategy::Block:
                    cv_.wait(lock, [this] {
                        return event_queue_.size() < max_queue_size_ || !running_.load(std::memory_order_relaxed);
                    });
                    if (!running_.load(std::memory_order_relaxed)) {
                        return contracts::Result<void>::err(
                            contracts::ErrorCode::Cancelled,
                            "Runtime stopped while blocking on event queue",
                            "vani.runtime.event_bus",
                            false,
                            contracts::ErrorCategory::Cancelled
                        );
                    }
                    event_queue_.push_back(EventEnvelope{
                        .event = std::move(event),
                        .guarantee = guarantee,
                        .strategy = strategy
                    });
                    published_count_.fetch_add(1, std::memory_order_relaxed);
                    break;

                default:
                    // Coalesce / Persist fallback
                    if (!event_queue_.empty()) {
                        event_queue_.pop_front();
                    }
                    event_queue_.push_back(EventEnvelope{
                        .event = std::move(event),
                        .guarantee = guarantee,
                        .strategy = strategy
                    });
                    published_count_.fetch_add(1, std::memory_order_relaxed);
                    break;
            }
        } else {
            event_queue_.push_back(EventEnvelope{
                .event = std::move(event),
                .guarantee = guarantee,
                .strategy = strategy
            });
            published_count_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    cv_.notify_one();
    return contracts::Result<void>::ok();
}

EventSubscriptionId EventBus::subscribe(EventFilter filter, EventHandler handler) {
    std::lock_guard<std::mutex> lock(sub_mutex_);
    const auto sub_id = next_sub_id_++;
    subscriptions_[sub_id] = Subscription{
        .id = sub_id,
        .filter = std::move(filter),
        .handler = std::move(handler)
    };
    return sub_id;
}

void EventBus::unsubscribe(EventSubscriptionId subscription_id) {
    std::lock_guard<std::mutex> lock(sub_mutex_);
    subscriptions_.erase(subscription_id);
}

size_t EventBus::pending_events_count() const noexcept {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return event_queue_.size();
}

size_t EventBus::subscribers_count() const noexcept {
    std::lock_guard<std::mutex> lock(sub_mutex_);
    return subscriptions_.size();
}

uint64_t EventBus::dropped_events_count() const noexcept {
    return dropped_count_.load(std::memory_order_relaxed);
}

uint64_t EventBus::published_events_count() const noexcept {
    return published_count_.load(std::memory_order_relaxed);
}

bool EventBus::matches_filter(const contracts::Event& event, const EventFilter& filter) const noexcept {
    if (filter.event_type.has_value() && *filter.event_type != event.event_type()) {
        return false;
    }
    if (filter.category.has_value() && *filter.category != event.category()) {
        return false;
    }
    if (filter.session_id.has_value() && *filter.session_id != event.session_id()) {
        return false;
    }
    if (filter.task_id.has_value() && *filter.task_id != event.task_id()) {
        return false;
    }
    return true;
}

void EventBus::worker_loop() {
    while (running_.load(std::memory_order_relaxed)) {
        EventEnvelope envelope;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this] {
                return !event_queue_.empty() || !running_.load(std::memory_order_relaxed);
            });

            if (!running_.load(std::memory_order_relaxed) && event_queue_.empty()) {
                break;
            }

            if (!event_queue_.empty()) {
                envelope = std::move(event_queue_.front());
                event_queue_.pop_front();
            }
        }

        if (envelope.event) {
            std::vector<EventHandler> matching_handlers;
            {
                std::lock_guard<std::mutex> lock(sub_mutex_);
                for (const auto& [id, sub] : subscriptions_) {
                    if (matches_filter(*envelope.event, sub.filter)) {
                        matching_handlers.push_back(sub.handler);
                    }
                }
            }

            for (const auto& handler : matching_handlers) {
                if (handler) {
                    try {
                        handler(envelope.event);
                    } catch (const std::exception& e) {
                        std::cerr << "[EventBus] Error in subscriber callback: " << e.what() << "\n";
                    } catch (...) {
                        std::cerr << "[EventBus] Unknown exception in subscriber callback\n";
                    }
                }
            }
        }
    }
}

} // namespace vani::runtime
