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
            VerboseBlock("{}.interruptible_task::promise_type::promise_type()", Ptr(this));
        }

        template <typename ..._Args>
        explicit promise_type(_Args&... args)
            : promise_type{}
        {
            VerboseBlock("{}.interruptible_task::promise_type::promise_type(...)", Ptr(this));

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
            VerboseBlock("{}.interruptible_task::promise_type::get_return_object()", Ptr(this));

            return interruptible_task{ state };
        }

        std::suspend_never initial_suspend() noexcept
        {
            Verbose("{}.interruptible_task::promise_type::initial_suspend()", Ptr(this));

            return {};
        }

        std::suspend_never final_suspend() noexcept
        {
            Verbose("{}.interruptible_task::promise_type::final_suspend()", Ptr(this));

            return {};
        }

        void return_void()
        {
            Verbose("{}.interruptible_task::promise_type::return_void()", Ptr(this));
        }

        void unhandled_exception()
        {
            Error("{}.interruptible_task::promise_type::unhandled_exception()", Ptr(this));
        }

        template <typename _Awaitable>
        auto await_transform(_Awaitable&& awaitable)
        {
            VerboseBlock("{}.interruptible_task::promise_type::await_transform()", Ptr(this));

            struct [[nodiscard]] wrapper
            {
                shared_state::ptr state;
                _Awaitable awaitable;

                // The purpose of the await_ready() method is to allow you to avoid the cost of the <suspend-coroutine> operation in cases 
                // where it is known that the operation will complete synchronously without needing to suspend.
                bool await_ready()
                {
                    VerboseBlock("{}.interruptible_task::promise_type::wrapper::await_ready()", Ptr(this));

                    return awaitable.await_ready();
                }

                // It is the responsibility of the await_suspend() method to schedule the coroutine for resumption(or destruction) at some point 
                // in the future once the operation has completed.Note that returning false from await_suspend() counts as scheduling the coroutine 
                // for immediate resumption on the current thread.
                auto await_suspend(std::coroutine_handle<> coro)
                {
                    VerboseBlock("{}.interruptible_task::promise_type::wrapper::await_suspend()", Ptr(this));

                    state->on_terminate = [this](boost::system::error_code ec)
                    {
                        awaitable.on_terminate(ec);
                    };

                    return awaitable.await_suspend(coro);
                }

                // The return-value of the await_resume() method call becomes the result of the co_await expression.
                // The await_resume() method can also throw an exception in which case the exception propagates out of the co_await expression.
                auto await_resume()
                {
                    VerboseBlock("{}.interruptible_task::promise_type::wrapper::await_resume()", Ptr(this));

                    return awaitable.await_resume();
                }
            };

            return wrapper{ state, std::forward<_Awaitable>(awaitable) };
        }
    };

    void terminate(boost::system::error_code ec = boost::asio::error::interrupted)
    {
        VerboseBlock("{}.interruptible_task::terminate()", Ptr(this));

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
        Verbose("{}.interruptible_task::~interruptible_task()", Ptr(this));
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
        Verbose("{}.interruptible_task::interruptible_task()", Ptr(this));
    }
};





