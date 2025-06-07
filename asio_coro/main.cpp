#include "echo_server.hxx"



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

    if (argc > 2)
    {
        Info("Usage: {} [host[:port]]", argv[0]);
        std::exit(EXIT_FAILURE);
    }

    if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))
    {
        Info("Usage: {} [host[:port]]", argv[0]);
        std::exit(EXIT_SUCCESS);
    }

    boost::asio::io_context context;
    boost::asio::signal_set signals{ context, SIGINT };


    signals.async_wait([&context](auto ec, auto code)
    {
        VerboseBlock("main::SIGINT()");

        context.stop();
    });

    if (argc > 1) 
    {
        const auto [host, port] = get_host_port(argv[1]);

        echo_server::accept(context.get_executor(), host, port);
    }
    else 
    {
        echo_server::accept(context.get_executor(), "localhost", "8000");
    }

    context.run();
     
    return 0;
}