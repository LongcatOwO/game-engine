module;
#include <algorithm>
#include <iterator>
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
namespace rng = std::ranges;

class TestInputIterator
{
public:
    using iterator_concept = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = unsigned;

    TestInputIterator() = default;
    constexpr TestInputIterator(unsigned begin) noexcept : _value(begin) {}

    constexpr unsigned operator*() const noexcept
    {
        return _value;
    }

    constexpr TestInputIterator &operator++() noexcept
    {
        ++_value;
        return *this;
    }

    constexpr void operator++(int) noexcept
    {
        ++_value;
    }

private:
    friend class TestInputIteratorSentinel;

    unsigned _value = 0;
};

class TestInputIteratorSentinel
{
public:
    TestInputIteratorSentinel() = default;
    constexpr TestInputIteratorSentinel(unsigned cap) noexcept : _cap(cap) {}

    constexpr bool operator==(TestInputIterator const &it) const noexcept
    {
        return it._value == _cap;
    }
private:
    unsigned _cap;
};

static_assert(std::input_iterator<TestInputIterator>);
static_assert(std::sentinel_for<TestInputIteratorSentinel, TestInputIterator>);

PL_STATIC_ASSERTION_TEST(test_insertTrivial)
{
    constexpr auto result = []
    {
        ArrayList<int> arr;
        int a[] = {53, 43};
        (void) arr.insert(arr.end(), std::begin(a), std::end(a));
        int b[] = {12, 22, 67, 87, 34, 65};
        (void) arr.insert(arr.end(), std::begin(b), std::end(b));
        return 
            rng::equal(std::begin(a), std::end(a), arr.begin(), arr.begin() + std::size(a))
         && rng::equal(std::begin(b), std::end(b), arr.begin() + std::size(a), arr.end());
    }();
    static_assert(result);
}

PL_STATIC_ASSERTION_TEST(test_insertNonTrivial)
{
    constexpr auto result = []
    {
        SideEffectResult result;
        {
            ArrayList<SideEffects> arr;
            for (unsigned i = 0; i < 10; ++i)
            {
                Mallocator a;
                auto effects = *new_<SideEffects>[2 * i](a, SideEffectsConstructor(result));

                (void) arr.insert(arr.begin() + arr.size() / 2, effects.begin(), effects.end());
                delete_(a, effects);
            }
        }
        return result;
    }();
    static_assert(result.numTotalConstructorCalls() == result.numDestructorCalls());
}

PL_STATIC_ASSERTION_TEST(test_insertInputIterator)
{
    constexpr auto result = []
    {
        ArrayList<unsigned> arr;
        for (unsigned i = 0; i < 3; ++i)
        {
            (void) arr.insert(
                arr.begin() + arr.size() / 2,
                TestInputIterator(i * 10),
                TestInputIteratorSentinel((i + 1) * 10));
        }
        return std::accumulate(arr.begin(), arr.end(), 0);
    }();
    static_assert(result == 29 * 15);
}

PL_STATIC_ASSERTION_TEST(test_insert_01)
{
    constexpr auto result = []
    {
        ArrayList<int> arr;

        int a1[] = {1, 2, 3};
        (void) arr.insert(arr.end(), std::begin(a1), std::end(a1));

        int a2[] = {4, 5, 6};
        (void) arr.insert(arr.begin() + 1, std::begin(a2), std::end(a2));

        int a3[] = {7, 8, 9};
        (void) arr.insert(arr.begin() + 3, std::begin(a3), std::end(a3));

        int answer[] = {1, 4, 5, 7, 8, 9, 6, 2, 3};

        return rng::equal(arr, answer);
    }();
    static_assert(result);
}

PL_STATIC_ASSERTION_TEST(test_assign_01)
{
    constexpr auto result = []
    {
        SideEffectResult result;
        {
            Mallocator a;
            auto effects = *new_<SideEffects>[13](a, SideEffectsConstructor(result));

            ArrayList<SideEffects> arr;
            (void) arr.assign(effects.begin(), effects.end());
            delete_(a, effects);
        }
        return result;
    }();

    static_assert(result.numTotalConstructorCalls() == result.numDestructorCalls());
}

PL_STATIC_ASSERTION_TEST(test_assign_02)
{
    constexpr auto result = []
    {
        SideEffectResult result;
        SideEffectsConstructor ctor = result;
        {
            ArrayList<SideEffects> arr;
            Mallocator a;
            auto effects = *new_<SideEffects>[3](a, ctor);
            (void) arr.assign(effects.begin(), effects.end());

            delete_(a, effects);
            effects = *new_<SideEffects>[8](a, ctor);

            (void) arr.assign(effects.begin(), effects.end());

            delete_(a, effects);
            effects = *new_<SideEffects>[20](a, ctor);

            (void) arr.assign(effects.begin(), effects.end());

            delete_(a, effects);
            effects = *new_<SideEffects>[50](a, ctor);

            (void) arr.assign(effects.begin(), effects.end());

            delete_(a, effects);
            effects = *new_<SideEffects>[5](a, ctor);

            (void) arr.assign(effects.begin(), effects.end());

            delete_(a, effects);

        }
        return result;
    }();

    static_assert(result.numTotalConstructorCalls() == result.numDestructorCalls());
}

PL_STATIC_ASSERTION_TEST(test_assign_03)
{
    constexpr auto result = []
    {
        ArrayList<unsigned> arr;
        (void) arr.assign(TestInputIterator(), TestInputIteratorSentinel(10));

        return std::accumulate(arr.begin(), arr.end(), 0);
    }();

    static_assert(result == 45);
}
} // namespace
} // namespace pl_test
