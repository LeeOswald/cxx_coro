#pragma once

#include "common.hxx"

#include <boost/asio.hpp>


struct interruptible_task
{
    struct shared_state
    {
        bool locked = false; // cannot be terminated if locked
        std::function<void(boost::system::error_code)> on_terminate;

        void lock()
        {
            locked = true;
        }

        void unlock()
        {
            locked = false;
        }

        using ptr = std::shared_ptr<shared_state>;
    };

    struct promise_type
    {
        shared_state::ptr state;

        promise_type()
            : state{ std::make_shared<shared_state>() }
        {
            VerboseBlock("{}.interruptible_task::promise_type::promise_type()", fmt::ptr(this));
        }

        template <typename ..._Args>
        explicit promise_type(_Args&... args)
            : promise_type{}
        {
            VerboseBlock("{}.interruptible_task::promise_type::promise_type(...)", fmt::ptr(this));

            ([this](auto& param) {
                if constexpr (std::is_same_v<_Args, shared_state::ptr>)
                {
                    param = state;
                    return true;
                }
                return false;
            }(args) || ...);
        }

        auto get_return_object()
        {
            VerboseBlock("{}.interruptible_task::promise_type::get_return_object()", fmt::ptr(this));

            return interruptible_task{ state };
        }

        std::suspend_never initial_suspend() noexcept
        {
            Verbose("{}.interruptible_task::promise_type::initial_suspend()", fmt::ptr(this));

            return {};
        }

        std::suspend_never final_suspend() noexcept
        {
            Verbose("{}.interruptible_task::promise_type::final_suspend()", fmt::ptr(this));

            return {};
        }

        void return_void()
        {
            Verbose("{}.interruptible_task::promise_type::return_void()", fmt::ptr(this));
        }

        void unhandled_exception()
        {
            Error("{}.interruptible_task::promise_type::unhandled_exception()", fmt::ptr(this));
        }

        template <typename _Awaitable>
        auto await_transform(_Awaitable&& awaitable)
        {
            VerboseBlock("{}.interruptible_task::promise_type::await_transform()", fmt::ptr(this));

            struct [[nodiscard]] wrapper
            {
                shared_state::ptr state;
                _Awaitable awaitable;

                bool await_ready()
                {
                    VerboseBlock("{}.interruptible_task::promise_type::wrapper::await_ready()", fmt::ptr(this));

                    return awaitable.await_ready();
                }

                auto await_suspend(std::coroutine_handle<> coro)
                {
                    VerboseBlock("{}.interruptible_task::promise_type::wrapper::await_suspend()", fmt::ptr(this));

                    state->on_terminate = [this](boost::system::error_code ec)
                    {
                        awaitable.on_terminate(ec);
                    };

                    return awaitable.await_suspend(coro);
                }

                auto await_resume()
                {
                    VerboseBlock("{}.interruptible_task::promise_type::wrapper::await_resume()", fmt::ptr(this));

                    return awaitable.await_resume();
                }
            };

            return wrapper{ state, std::forward<_Awaitable>(awaitable) };
        }
    };

    void terminate(boost::system::error_code ec = boost::asio::error::interrupted)
    {
        VerboseBlock("{}.interruptible_task::terminate()", fmt::ptr(this));

        auto ptr = state_.lock();
        if (!ptr)
        {
            Verbose("Coro already destroyed");
            return; // coro already destroyed
        }

        if (!ptr->on_terminate)
        {
            Verbose("Coro has no termination callback");
            return;
        }

        if (ptr->locked)
        {
            Verbose("Coro is locked and cannot be terminated");
            return;
        }

        ptr->on_terminate(ec);
        ptr->on_terminate = nullptr;
    }

    ~interruptible_task()
    {
        Verbose("{}.interruptible_task::~interruptible_task()", fmt::ptr(this));
    }

    interruptible_task(const interruptible_task&) = delete;
    interruptible_task& operator=(const interruptible_task&) = delete;

    interruptible_task(interruptible_task&&) = default;
    interruptible_task& operator=(interruptible_task&&) = default;

private:
    std::weak_ptr<shared_state> state_;

    interruptible_task(std::weak_ptr<shared_state> state)
        : state_(std::move(state))
    {
        Verbose("{}.interruptible_task::interruptible_task()", fmt::ptr(this));
    }
};





