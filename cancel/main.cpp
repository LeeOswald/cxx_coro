#include "cancel.hxx"

#include <boost/asio/experimental/awaitable_operators.hpp>

using namespace boost::asio::experimental::awaitable_operators;


int main(int argc, char** argv)
{
    VerboseBlock("main()");

    boost::asio::io_context io;
    boost::asio::signal_set signals{ io, SIGINT };

    signals.async_wait([&io](auto ec, auto code)
    {
        VerboseBlock("main::SIGINT()");

        io.stop();
    });

    boost::asio::system_timer timer{ io };
    timer.expires_at(std::chrono::system_clock::time_point::max());

    co_spawn(
        io, 
        cancellable::periodic_work() || 
        cancellable::cancel(timer), 
        [](auto, auto) 
        { 
            Info("Stopped. Press Ctrl-C to exit.");
        }
    );

    co_spawn(io, cancellable::run_for(timer, std::chrono::seconds(5)), boost::asio::detached);

    io.run();
     
    return 0;
}