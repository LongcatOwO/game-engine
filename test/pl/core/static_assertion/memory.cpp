module;
#include <memory>
#include <numeric>
#include <pl/test_macro.hpp>

module pl.core.test;

import pl.core;

namespace pl_test
{
namespace
{
using namespace pl;
PL_STATIC_ASSERTION_TEST(test_isValidAlignment)
{
    static_assert(!isValidAlignment(0));
    static_assert(!isValidAlignment(3));
    static_assert(!isValidAlignment(15));
    static_assert(!isValidAlignment(14));

    static_assert(isValidAlignment(1));
    static_assert(isValidAlignment(2));
    static_assert(isValidAlignment(4));
    static_assert(isValidAlignment(8));
    static_assert(isValidAlignment(16));
}

PL_STATIC_ASSERTION_TEST(test_allocSingle)
{
    constexpr auto result = []
    {
        Mallocator a;
        auto p = *alloc<int>(a);
        construct_at(p, 42);
        int r = *p;
        free(a, p);
        return r;
    }();

    static_assert(result == 42);
}

PL_STATIC_ASSERTION_TEST(test_allocDynamicSize)
{
    constexpr auto result = []
    {
        Mallocator a;
        auto p = *alloc<int>(a, 5);
        std::construct_at(p.data(), 34);
        std::construct_at(p.data() + 1, 12);
        std::construct_at(p.data() + 2, 25);
        auto sum = std::accumulate(p.begin(), p.begin() + 3, 0);
        free(a, p);
        return sum;
    }();

    static_assert(result == 71);
}

PL_STATIC_ASSERTION_TEST(test_allocStaticSize)
{
    constexpr auto result = []
    {
        Mallocator a;
        auto p = *alloc<int, 7>(a);
        for (auto it = p.begin(); auto i : {43, 23, 54, 65, 12, 1, 5})
        {
            std::construct_at(std::to_address(it++), i);
        }
        auto result = std::accumulate(p.begin(), p.end(), 0);
        free(a, p);
        return result;
    }();

    static_assert(result == 203);
}

PL_STATIC_ASSERTION_TEST(test_newSingle)
{
    constexpr auto result = []
    {
        Mallocator a;
        auto p = *new_<int>(a, 6);
        auto result = *p;
        delete_(a, p);
        return result;
    }();

    static_assert(result == 6);
}

PL_STATIC_ASSERTION_TEST(test_newArr)
{
    constexpr auto result = []
    {
        Mallocator a;
        SideEffectResult result;
        auto sideEffects = *new_<SideEffects>[5](a, [&](SideEffects *e){ std::construct_at(e, result); });

        sideEffects[2] = {result};
        delete_(a, sideEffects);
        return result;
    }();

    static_assert(result.numRegularConstructorCalls() == 6);
    static_assert(result.numDestructorCalls() == 6);
    static_assert(result.numMoveAssignmentCalls() == 1);
}
} // namespace
} // namespace pl_test
