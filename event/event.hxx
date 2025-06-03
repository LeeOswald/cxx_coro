#pragma once

#include "common.hxx"

#include <atomic>

class async_event
{
public:
    async_event(bool set = false) noexcept
        : state_(set ? this : nullptr)
    {
    }

    async_event(const async_event&) = delete;
    async_event(async_event&&) = delete;
    async_event& operator=(const async_event&) = delete;
    async_event& operator=(async_event&&) = delete;

    bool is_set() const noexcept
    {
        return state_.load(std::memory_order_acquire) == this;
    }

    struct awaiter;
    awaiter operator co_await() const noexcept;

    void set() noexcept;

    void reset() noexcept
    {
        void* oldValue = this;
        state_.compare_exchange_strong(oldValue, nullptr, std::memory_order_acquire);
    }

private:
    friend struct awaiter;

    // == this => set state
    // otherwise => not set, head of linked list of awaiter*.
    mutable std::atomic<void*> state_;
};

struct async_event::awaiter
{
    awaiter(const async_event& event) noexcept
        : event_(event)
    {
    }

    bool await_ready() const noexcept
    {
        return event_.is_set();
    }

    bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
    {
        // Special state_ value that indicates the event is in the 'set' state.
        const void* const setState = &event_;

        // Stash the handle of the awaiting coroutine.
        awaiting_coro_ = awaitingCoroutine;

        // Try to atomically push this awaiter onto the front of the list.
        void* oldValue = event_.state_.load(std::memory_order_acquire);
        do
        {
            // Resume immediately if already in 'set' state.
            if (oldValue == setState) return false;

            // Update linked list to point at current head.
            next_ = static_cast<awaiter*>(oldValue);

            // Finally, try to swap the old list head, inserting this awaiter
            // as the new list head.
        } while (!event_.state_.compare_exchange_weak(oldValue, this, std::memory_order_release, std::memory_order_acquire));

        // Successfully enqueued. Remain suspended.
        return true;
    }

    void await_resume() noexcept {}

private:
    friend struct async_event;

    const async_event& event_;
    std::coroutine_handle<> awaiting_coro_;
    awaiter* next_;
};


inline async_event::awaiter async_event::operator co_await() const noexcept
{
    return awaiter{ *this };
}

inline void async_event::set() noexcept
{
    // Needs to be 'release' so that subsequent 'co_await' has
    // visibility of our prior writes.
    // Needs to be 'acquire' so that we have visibility of prior
    // writes by awaiting coroutines.
    void* oldValue = state_.exchange(this, std::memory_order_acq_rel);
    if (oldValue != this)
    {
        // Wasn't already in 'set' state.
        // Treat old value as head of a linked-list of waiters
        // which we have now acquired and need to resume.
        auto* waiters = static_cast<awaiter*>(oldValue);
        while (waiters != nullptr)
        {
            // Read next_ before resuming the coroutine as resuming
            // the coroutine will likely destroy the awaiter object.
            auto* next = waiters->next_;
            waiters->awaiting_coro_.resume();
            waiters = next;
        }
    }
}


// A simple task-class for void-returning coroutines.
struct task
{
    struct promise_type
    {
        task get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {}
    };
};

task example(async_event& event)
{
    co_await event;
}
