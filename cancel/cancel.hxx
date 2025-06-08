#pragma once

#include "common.hxx"


#include <boost/asio.hpp>



namespace cancellable
{


boost::asio::awaitable<void> periodic_work()
{
    VerboseBlock("periodic_work()");

    for (;;)
    {
        std::cout << "Hello" << std::endl;
        boost::asio::system_timer timer{ co_await boost::asio::this_coro::executor };
        timer.expires_after(std::chrono::seconds{ 1 });
        co_await timer.async_wait(boost::asio::use_awaitable);
    }
}

boost::asio::awaitable<void> run_for(boost::asio::system_timer& outer, std::chrono::milliseconds duration)
{
    VerboseBlock("run_for()");

    boost::asio::system_timer timer{ co_await boost::asio::this_coro::executor };
    timer.expires_after(duration);
    
    co_await timer.async_wait(boost::asio::use_awaitable);
    
    Verbose("Cancelling...");
    outer.cancel();
}

boost::asio::awaitable<void> cancel(boost::asio::system_timer& timer)
{
    VerboseBlock("cancel()");

    try
    {
        co_await timer.async_wait(boost::asio::use_awaitable);
        Verbose("Timer expired");
    }
    catch (boost::system::system_error& e)
    {
        if (e.code() == boost::system::errc::operation_canceled)
        {
            Verbose("Timer cancelled");
        }
        else
        {
            Error("Caught [{}]", e.what());
            std::abort();
        }
    }
}

} // namespace cancellable {}
