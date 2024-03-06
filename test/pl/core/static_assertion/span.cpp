module;
#include <cstddef>
#include <numeric>

#include <pl/test_macro.hpp>

module pl.core.test;

import pl.core;

namespace pl_test
{
namespace
{
using namespace pl;
PL_STATIC_ASSERTION_TEST(test_read)
{
    static constexpr int a[5] = {41, 32, 13, 6, 47};
    constexpr Span s = a;
    static_assert(*s.begin() == 41);
    static_assert(s.begin()[4] == 47);
    static_assert(*(s.begin() + 2) == 13);
    static_assert(std::accumulate(s.begin() + 1, s.end() - 1, 0) == 51);
    static_assert(std::accumulate(s.rbegin(), s.rend() - 2, 0) == 66);
    static_assert(s.front() == 41);
    static_assert(s.back() == 47);
    static_assert(s[3] == 6);
    static_assert(s[2] == 13);
    static_assert(s.data()[3] == 6);
    static_assert(s.size() == 5);
    static_assert(!s.empty());
}
} // namespace
} // namespace pl_test
