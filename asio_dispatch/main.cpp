#include "common.hxx"

#include <thread>

#include <boost/asio.hpp>


namespace
{



boost::asio::awaitable<void> f(auto strand) 
{
    VerboseBlock("f()");

    Info("IO Context Executor");
    co_await boost::asio::dispatch(boost::asio::bind_executor(strand, boost::asio::deferred));
    Info("Strand Executor");
    co_await boost::asio::dispatch(boost::asio::deferred);
    Info("Back on IO Context Executor");
}


} // namespace {}



int main()
{
    VerboseBlock("main()");

    boost::asio::io_context io;
    boost::asio::thread_pool tp(1);

    co_spawn(io, f(boost::asio::make_strand(tp)), boost::asio::detached);

    io.run();
        
    return 0;
}