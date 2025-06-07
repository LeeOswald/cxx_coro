#pragma once

#include "common.hxx"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/experimental/coro.hpp>


namespace echo_server
{

namespace util
{

constexpr auto nbeswap(std::integral auto val) noexcept
{
    if constexpr (std::endian::native == std::endian::big)
        return val;

    return std::byteswap(val);
}

} // namespace util {}

boost::asio::experimental::coro<std::string> reader(boost::asio::ip::tcp::socket& s)
{
    VerboseBlock("reader()");

    try
    {
        std::vector<char> data;
        for (;;)
        {
            Verbose("receiving...");
            std::uint16_t size = 0;
            co_await boost::asio::async_read(s, boost::asio::buffer(&size, sizeof(size)), boost::asio::deferred);

            size = util::nbeswap(size);
            Verbose("receiving {} bytes...", size);
            data.resize(size);
            co_await boost::asio::async_read(s, boost::asio::buffer(data), boost::asio::deferred);

            Info("received [{}]", cxx_coro::binaryToAscii({ data.data(), size }));

            co_yield std::string{ data.data(), size };
        }
    }
    catch (std::exception& e)
    {
        Error("Caught [{}]", e.what());
    }
}


boost::asio::awaitable<void> client_handler(boost::asio::ip::tcp::socket s)
{
    VerboseBlock("client_handler()");

    try
    {
        auto r = reader(s);

        for (;;)
        {
            auto msg = co_await r.async_resume(boost::asio::use_awaitable);
            if (msg.has_value())
            {
                auto& v = *msg;

                Verbose("sending...");
                
                auto size = util::nbeswap(static_cast<std::uint16_t>(v.length()));
                std::array seq
                { 
                    boost::asio::buffer(&size, sizeof(size)), 
                    boost::asio::buffer(v) 
                };

                co_await boost::asio::async_write(s, seq, boost::asio::deferred);
            }
        }
    }
    catch (std::exception& e)
    {
        Error("Caught [{}]", e.what());
    }
}

boost::asio::awaitable<void> listener(boost::asio::ip::tcp::endpoint ep)
{
    VerboseBlock("listener()");

    auto executor{ co_await boost::asio::this_coro::executor };

    boost::asio::ip::tcp::acceptor acceptor{ executor, ep };

    for (;;)
    {
        Verbose("accepting...");
        boost::asio::ip::tcp::socket socket{ co_await acceptor.async_accept(boost::asio::deferred) };

        Info("new connection started");
        boost::asio::co_spawn(executor, client_handler(std::move(socket)), boost::asio::detached);
    }
}

void accept(boost::asio::execution::executor auto ex, std::string_view host, std::string_view port)
{
    VerboseBlock("accept()");

    boost::asio::ip::tcp::resolver resolver{ ex };

    for (auto re : resolver.resolve(host, port))
    {
        auto ep{ re.endpoint() };

        Info("Listening on: {}:{}", ep.address().to_string(), ep.port());

        boost::asio::co_spawn(ex, listener(std::move(ep)), boost::asio::detached);
    }
}



} // namespace echo_server {}
