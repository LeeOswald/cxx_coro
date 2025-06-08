#pragma once

#include "common.hxx"

#ifndef NDEBUG
#define ASIO_ENABLE_HANDLER_TRACKING 1
#endif

#include <boost/asio.hpp>
#include <boost/asio/experimental/as_tuple.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>


namespace proxy_server
{
using namespace std::literals::chrono_literals;
using namespace boost::asio::experimental::awaitable_operators;

constexpr auto use_nothrow_awaitable = boost::asio::experimental::as_tuple(boost::asio::use_awaitable);


boost::asio::awaitable<void> transfer(boost::asio::ip::tcp::socket& from, boost::asio::ip::tcp::socket& to, std::chrono::steady_clock::time_point& deadline)
{
    VerboseBlock("transfer()");

    std::array<char, 1024> data;

    for (;;)
    {
        deadline = std::max(deadline, std::chrono::steady_clock::now() + 5s);

        Verbose("receiving...");

        auto [e1, n1] = co_await from.async_read_some(boost::asio::buffer(data), use_nothrow_awaitable);
        if (e1)
            co_return;

        Info("received [{}]", cxx_coro::binaryToAscii({ data.data(), n1 }));

        Verbose("sending...");

        auto [e2, n2] = co_await boost::asio::async_write(to, boost::asio::buffer(data, n1), use_nothrow_awaitable);
        if (e2)
            co_return;
    }
}

boost::asio::awaitable<void> watchdog(std::chrono::steady_clock::time_point& deadline)
{
    VerboseBlock("watchdog()");

    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);

    auto now = std::chrono::steady_clock::now();
    while (deadline > now)
    {
        timer.expires_at(deadline);
        co_await timer.async_wait(use_nothrow_awaitable);
        now = std::chrono::steady_clock::now();
    }
}

boost::asio::awaitable<void> proxy(boost::asio::ip::tcp::socket client, boost::asio::ip::tcp::endpoint target)
{
    VerboseBlock("proxy()");

    boost::asio::ip::tcp::socket server(client.get_executor());
    std::chrono::steady_clock::time_point client_to_server_deadline{};
    std::chrono::steady_clock::time_point server_to_client_deadline{};

    auto [e] = co_await server.async_connect(target, use_nothrow_awaitable);
    if (!e)
    {
        co_await(
            (transfer(client, server, client_to_server_deadline) || watchdog(client_to_server_deadline)) &&
            (transfer(server, client, server_to_client_deadline) || watchdog(server_to_client_deadline))
        );
    }
}

boost::asio::awaitable<void> listen(boost::asio::ip::tcp::acceptor& acceptor, boost::asio::ip::tcp::endpoint target)
{
    VerboseBlock("listen()");

    for (;;)
    {
        Verbose("accepting connections...");

        auto [e, client] = co_await acceptor.async_accept(use_nothrow_awaitable);
        if (e)
            break;

        Info("new connection started");

        auto ex = client.get_executor();
        boost::asio::co_spawn(ex, proxy(std::move(client), target), boost::asio::detached);
    }
}


} // namespace proxy_server {}
