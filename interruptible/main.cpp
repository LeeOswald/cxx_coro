#include "interruptible.hxx"



namespace
{

template <typename _Executor, typename _Duration>
auto sleep(_Executor&& executor, _Duration&& duration)
{
    VerboseBlock("sleep()");

    struct [[nodiscard]] awaitable
    {
        boost::asio::steady_timer timer;
        boost::system::error_code error = {};

        bool await_ready()
        {
            Verbose("sleep::awaitable::await_ready()");

            return false;
        }

        void await_suspend(std::coroutine_handle<> coro)
        {
            VerboseBlock("sleep::awaitable::await_suspend()");

            timer.async_wait([this, coro](auto ec) mutable
            {
                VerboseBlock("sleep::awaitable::timer_callback()");

                if (!error)
                {
                    error = ec;
                }

                coro.resume();
            });
        }

        void on_terminate(boost::system::error_code ec)
        {
            VerboseBlock("sleep::awaitable::on_terminate()");

            error = ec;
            timer.cancel();
        }

        void await_resume()
        {
            VerboseBlock("sleep::awaitable::await_resume()");

            if (error)
            {
                Verbose("Rethrowing [{}]", error.message());
                throw boost::system::system_error(error);
            }
        }
    };

    return awaitable{ boost::asio::steady_timer{ std::forward<_Executor>(executor), std::forward<_Duration>(duration) } };
}


template <typename _Executor>
interruptible_task do_sleep(_Executor&& executor, interruptible_task::shared_state::ptr state) // promise_type ctor receives these args
{
    VerboseBlock("do_sleep()");

    using namespace std::chrono_literals;

    try
    {
        Info("Sleeping for 10 s, but you may press Ctrl-C to continue...");
        co_await sleep(executor, 10s); // <expr> -> promise.await_transform() -> <awaitable> -> awaitable.operator co_await() -> <awaiter>
        Info("Continuing...");
    }
    catch (std::exception& e)
    {
        // catch exceptions thrown from sleep::awaitable::await_resume()
        Info("Caught [{}]", e.what());
    }


    Info("Sleeping for 10 s, but you can not interrupt...");

    const std::lock_guard g(*state);
    co_await sleep(executor, 10s); // cannot be interrupted

    Info("All done. Press Ctrl-C to exit.");
}


} // namespace {}



int main()
{
    VerboseBlock("main()");

    boost::asio::io_context context;
    boost::asio::signal_set signals{ context, SIGINT };

    auto job = do_sleep(context.get_executor(), std::make_shared<interruptible_task::shared_state>());

    signals.async_wait([&job](auto ec, auto code)
    {
        VerboseBlock("main::SIGINT()");

        job.terminate();
    });

    context.run();
 
    return 0;
}