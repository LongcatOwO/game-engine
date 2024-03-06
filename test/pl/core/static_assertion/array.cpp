module;
#include <algorithm>
#include <concepts>
#include <numeric>
#include <ranges>
#include <pl/test_macro.hpp>

module pl.core.test;

import pl.core;

namespace rng = std::ranges;
namespace vw = std::views;

namespace pl_test
{
namespace
{
using namespace pl;
PL_STATIC_ASSERTION_TEST(test_zeroLength)
{
    constexpr Array<int, 0> a;
    [[maybe_unused]] int b = a[6];
    b = a.front();
    b = a.back();
    static_assert(a.data() == nullptr);
    static_assert(a.begin() == a.end());
    static_assert(a.cbegin() == a.cend());
    static_assert(a.rbegin() == a.rend());
    static_assert(a.crbegin() == a.crend());
    static_assert(a.empty());
    static_assert(a.size() == 0);
    static_assert(a.max_size() == 0);
}

PL_STATIC_ASSERTION_TEST(test_read)
{
    constexpr Array a = {42, 56, 13, 723, 34};
    static_assert(a[0] == 42);
    static_assert(a[1] == 56);
    static_assert(a[2] == 13);
    static_assert(a[3] == 723);
    static_assert(a[4] == 34);
    static_assert(a.front() == 42);
    static_assert(a.back() == 34);
    static_assert(*a.data() == 42);
    static_assert(a.data()[3] == 723);
    static_assert(*a.begin() == 42);
    static_assert(a.begin()[4] == 34);
    static_assert(*(a.begin() + 2) == 13);
    static_assert(a.end()[-1] == 34);
    static_assert(!a.empty());
    static_assert(a.size() == 5);
    static_assert(a.max_size() == 5);
    static_assert(std::accumulate(a.begin(), a.end(), 0) == 868);
    static_assert(std::accumulate(a.rbegin(), a.rend(), 0) == 868);
}

PL_STATIC_ASSERTION_TEST(test_write)
{
    constexpr Array a = []
    {
        Array a = {-1, 2, 4, 7, 3};

        a[3] = 13;
        rng::copy(vw::transform(a, [](auto i){ return i * 2 - 1; }), a.begin());

        return a;
    }();

    static_assert(a == Array{-3, 3, 7, 25, 5});
}
} // namespace
} // namespace pl_test
