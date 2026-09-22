#pragma once

#include "../../contracts/events/event.hpp"
#include "../../contracts/common/result.hpp"
#include "delivery_guarantee.hpp"
#include "backpressure_strategy.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <memory>
#include <deque>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <optional>

namespace vani::runtime {

using EventSubscriptionId = uint64_t;
using EventHandler = std::function<void(contracts::EventPtr event)>;

struct EventFilter {
    std::optional<std::string> event_type{std::nullopt};
    std::optional<contracts::EventCategory> category{std::nullopt};
    std::optional<contracts::SessionId> session_id{std::nullopt};
    std::optional<contracts::TaskId> task_id{std::nullopt};
};

struct EventEnvelope {
    contracts::EventPtr event;
    DeliveryGuarantee guarantee{DeliveryGuarantee::Ephemeral};
    BackpressureStrategy strategy{BackpressureStrategy::DropOldest};
};

class EventBus {
public:
    explicit EventBus(size_t worker_threads = 2, size_t max_queue_size = 1000);
    ~EventBus();

    contracts::Result<void> start();
    contracts::Result<void> stop();

    contracts::Result<void> publish(
        contracts::EventPtr event,
        DeliveryGuarantee guarantee = DeliveryGuarantee::Ephemeral,
        BackpressureStrategy strategy = BackpressureStrategy::DropOldest
    );

    EventSubscriptionId subscribe(
        EventFilter filter,
        EventHandler handler
    );

    void unsubscribe(EventSubscriptionId subscription_id);

    [[nodiscard]] size_t pending_events_count() const noexcept;
    [[nodiscard]] size_t subscribers_count() const noexcept;
    [[nodiscard]] uint64_t dropped_events_count() const noexcept;
    [[nodiscard]] uint64_t published_events_count() const noexcept;

private:
    struct Subscription {
        EventSubscriptionId id{0};
        EventFilter filter;
        EventHandler handler;
    };

    void worker_loop();
    [[nodiscard]] bool matches_filter(const contracts::Event& event, const EventFilter& filter) const noexcept;

    std::atomic<bool> running_{false};
    size_t max_queue_size_;
    std::atomic<uint64_t> published_count_{0};
    std::atomic<uint64_t> dropped_count_{0};

    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::deque<EventEnvelope> event_queue_;

    mutable std::mutex sub_mutex_;
    EventSubscriptionId next_sub_id_{1};
    std::unordered_map<EventSubscriptionId, Subscription> subscriptions_;

    std::vector<std::thread> workers_;
    size_t worker_thread_count_;
};

using EventBusPtr = std::shared_ptr<EventBus>;

} // namespace vani::runtime
