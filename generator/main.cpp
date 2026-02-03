#include "generator.hxx"

#include <vector>

namespace
{


void generate_from_strings()
{
    VerboseBlock("generate_from_strings()");

    std::vector<std::string> v = {
        "little",
        "Mary",
        "had",
        "a",
        "little",
        "lamb"
    };

    auto g = make_generator_from(std::move(v));

    while (auto x = g.next())
    {
        Info("[{}]", *x);
    }
}

void iterate_thru()
{
    VerboseBlock("iterate_thru()");

    std::vector<int> v = { 0, 2, 4, 8, 16, 32, 64, 128, 256, 512 };

    auto g = make_generator_from(std::move(v));

    for (auto& x : g)
    {
        Info("[{}]", x);
    }
}


class coro_exception
    : public std::exception
{
public:
    coro_exception(std::string_view msg)
        : what_(msg)
    {
    }

    const char* what() const noexcept override
    {
        return what_.c_str();
    }

private:
    std::string what_;
};

generator<int> generate_exception()
{
    VerboseBlock("generate_exception()");

    throw coro_exception("Lol this is uncaught");
    co_return;
}

void test_exception()
{
    VerboseBlock("test_exception()");

    auto g = generate_exception();

    try
    {
        [[maybe_unused]] auto v = g.next();
    }
    catch (std::exception& e)
    {
        Error("Caught [{}]", e.what());
    }
}


} // namespace {}



int main()
{
    generate_from_strings();
    Info("---------------------");
    iterate_thru();
    Info("---------------------");
    test_exception();
 
    return 0;
}
