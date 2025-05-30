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


} // namespace {}



int main()
{
    generate_from_strings();
    Info("---------------------");
    iterate_thru();
 
    return 0;
}