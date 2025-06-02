#include "common.hxx"

#include <generator>

namespace
{

template<typename T>
struct Tree
{
    T value;
    Tree* left = nullptr;
    Tree* right = nullptr;

    std::generator<const T&> traverse() const
    {
        if (left)
            co_yield std::ranges::elements_of(left->traverse());

        co_yield value;

        if (right)
            co_yield std::ranges::elements_of(right->traverse());
    }
};


} // namespace {}



int main()
{
    Tree<char> tree[]
    {
                            {'D', tree + 1, tree + 2},
        //                            │
        //            ┌───────────────┴────────────────┐
        //            │                                │
                    {'B', tree + 3, tree + 4},       {'F', tree + 5, tree + 6},
        //            │                                │
        //  ┌─────────┴─────────────┐      ┌───────────┴─────────────┐
        //  │                       │      │                         │
          {'A'},                  {'C'}, {'E'},                    {'G'}
    };

    for (char x : tree->traverse())
        Info("{}", x);
        
    return 0;
}