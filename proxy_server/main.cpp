#include "proxy_server.hxx"



namespace
{

std::pair<std::string_view, std::string_view> get_host_port(char* option) 
{
    char* colon{ strchr(option, ':') };
    if (colon)
        return { {option, (unsigned)(colon - option)}, colon + 1 };

    return { option, "" };
}


} // namespace {}



int main(int argc, char** argv)
{
    VerboseBlock("main()");

    try
    {
        if (argc != 3)
        {
            Info("Usage: {} listen_addr:listen_port target_addr:target_port", argv[0]);
            std::exit(EXIT_FAILURE);
        }

        const auto [listen_host, listen_port] = get_host_port(argv[1]);
        const auto [target_host, target_port] = get_host_port(argv[2]);


        boost::asio::io_context context;
        boost::asio::signal_set signals{ context, SIGINT };

        signals.async_wait([&context](auto ec, auto code)
        {
            VerboseBlock("main::SIGINT()");

            context.stop();
        });

        auto listen_endpoint =
            *boost::asio::ip::tcp::resolver(context).resolve(
            listen_host,
            listen_port,
            boost::asio::ip::tcp::resolver::passive
        );

        auto target_endpoint =
            *boost::asio::ip::tcp::resolver(context).resolve(
            target_host,
            target_port
        );

        boost::asio::ip::tcp::acceptor acceptor(context, listen_endpoint);

        co_spawn(context, proxy_server::listen(acceptor, target_endpoint), boost::asio::detached);

        context.run();
    }
    catch (std::exception& e)
    {
        Error("Caught [{}]", e.what());
    }
     
    return 0;
}