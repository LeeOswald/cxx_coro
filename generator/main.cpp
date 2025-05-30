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

    while (auto v = g.next())
    {
        Info("[{}]", *v);
    }
}


} // namespace {}



int main()
{
    generate_from_strings();
 
    return 0;
}