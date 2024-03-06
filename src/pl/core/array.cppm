module;
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

#include <pl/macro.hpp>

export module pl.core:array;

import :iterator;
import :tags;


namespace pl
{
template<class T, std::size_t N>
struct array_storage
{
    using type = T[N];
};

template<class T>
struct array_storage<T, 0>
{
    using type = tags::none_t;
};

template<class T, std::size_t N>
using array_storage_t = array_storage<T, N>::type;

template<class T, std::size_t N>
class ArrayIterator
{
public:
    using iterator_concept = std::contiguous_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using element_type = T;
    using reference = T &;
    using pointer = T *;

    ArrayIterator           ()                      = default;
    ArrayIterator           (ArrayIterator const &) = default;
    ArrayIterator &operator=(ArrayIterator const &) = default;

    constexpr ArrayIterator(
        T *p
#ifndef NDEBUG
        ,
        T *begin
#endif
    ) noexcept
    :
        _p(p)
#ifndef NDEBUG
        ,
        _begin(begin)
#endif
    {}

    constexpr ArrayIterator &operator++() noexcept
    {
        assert(isOffsetInValidRange(1));
        ++_p;
        return *this;
    }

    constexpr ArrayIterator operator++(int) noexcept
    {
        assert(isOffsetInValidRange(1));
        auto copy = *this;
        ++_p;
        return copy;
    }

    constexpr ArrayIterator &operator--() noexcept
    {
        assert(isOffsetInValidRange(-1));
        --_p;
        return *this;
    }

    constexpr ArrayIterator operator--(int) noexcept
    {
        assert(isOffsetInValidRange(-1));
        auto copy = *this;
        --_p;
        return copy;
    }

    constexpr ArrayIterator &operator+=(difference_type d) noexcept
    {
        assert(isOffsetInValidRange(d));
        _p += d;
        return *this;
    }

    constexpr ArrayIterator &operator-=(difference_type d) noexcept
    {
        assert(isOffsetInValidRange(-d));
        _p -= d;
        return *this;
    }

    friend constexpr ArrayIterator operator+(ArrayIterator i, difference_type d) noexcept
    {
        assert(i.isOffsetInValidRange(d));
        i._p += d;
        return i;
    }

    friend constexpr ArrayIterator operator+(difference_type d, ArrayIterator i) noexcept
    {
        assert(i.isOffsetInValidRange(d));
        i._p += d;
        return i;
    }

    friend constexpr ArrayIterator operator-(ArrayIterator i, difference_type d) noexcept
    {
        assert(i.isOffsetInValidRange(-d));
        i._p -= d;
        return i;
    }

    friend constexpr difference_type operator-(ArrayIterator a, ArrayIterator b) noexcept
    {
        assert(a._begin == b._begin);
        return a._p - b._p;
    }

    constexpr reference operator*() const noexcept
    {
        assert(isOffsetInReferencableRange(0));
        return *_p;
    }

    constexpr pointer operator->() const noexcept
    {
        return _p;
    }

    constexpr reference operator[](difference_type d) const noexcept
    {
        assert(isOffsetInReferencableRange(d));
        return _p[d];
    }

    constexpr bool operator==(ArrayIterator const &other) const noexcept
    {
        assert(_begin == other._begin);
        return _p == other._p;
    }

    constexpr auto operator<=>(ArrayIterator const &other) const noexcept
    {
        assert(_begin == other._begin);
        return _p <=> other._p;
    }

private:
#ifndef NDEBUG
    constexpr bool isOffsetInValidRange(difference_type d) const noexcept
    {
        auto new_index = _p - _begin + d;
        return 0 <= new_index && (std::size_t) new_index <= N;
    }

    constexpr bool isOffsetInReferencableRange(difference_type d) const noexcept
    {
        auto new_index = _p - _begin + d;
        return 0 <= new_index && (std::size_t) new_index < N;
    }
#endif

    T *_p = {};
#ifndef NDEBUG
    T *_begin = {};
#endif
};
} // namespace pl

export namespace pl
{
template<class T, std::size_t N>
struct Array
{
    using value_type             = T;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = value_type       &;
    using const_reference        = value_type const &;
    using pointer                = value_type       *;
    using const_pointer          = value_type const *;
    using iterator               = ArrayIterator<T, N>;
    using const_iterator         = const_iterator<iterator>;
    using reverse_iterator       = std::reverse_iterator<      iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    [[nodiscard]] constexpr reference operator[](size_type pos) noexcept
    {
        assert(isIndexInRange(pos));
        if constexpr (N == 0) { PL_UNREACHABLE(); }
        else return _array[pos];
    }

    [[nodiscard]] constexpr const_reference operator[](size_type pos) const noexcept
    {
        assert(isIndexInRange(pos));
        if constexpr (N == 0) { PL_UNREACHABLE(); }
        else return _array[pos];
    }

    [[nodiscard]] constexpr reference front() noexcept
    {
        if constexpr (N == 0) { PL_UNREACHABLE(); }
        else return _array[0];
    }

    [[nodiscard]] constexpr const_reference front() const noexcept
    {
        if constexpr (N == 0) { PL_UNREACHABLE(); }
        else return _array[0];
    }

    [[nodiscard]] constexpr reference back() noexcept
    {
        if constexpr (N == 0) { PL_UNREACHABLE(); }
        else return _array[N - 1];
    }

    [[nodiscard]] constexpr const_reference back() const noexcept
    {
        if constexpr (N == 0) { PL_UNREACHABLE(); }
        else return _array[N - 1];
    }

    [[nodiscard]] constexpr pointer data() noexcept
    {
        if constexpr (N == 0)
            return nullptr;
        else
            return _array;
    }

    [[nodiscard]] constexpr const_pointer data() const noexcept
    {
        if constexpr (N == 0)
            return nullptr;
        else
            return _array;
    }

    [[nodiscard]] constexpr iterator begin() noexcept
    {
        if constexpr (N == 0)
            return {};
        else
            return
            {
                _array,
#ifndef NDEBUG
                _array,
#endif
            };
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept
    {
        return {const_cast<Array *>(this)->begin()};
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept
    {
        return begin();
    }

    [[nodiscard]] constexpr iterator end() noexcept
    {
        if constexpr (N == 0)
            return {};
        else
            return
            {
                _array + N,
#ifndef NDEBUG
                _array,
#endif
            };
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept
    {
        return {const_cast<Array *>(this)->end()};
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept
    {
        return end();
    }

    [[nodiscard]] constexpr reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }

    [[nodiscard]] constexpr reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }

    [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

    [[nodiscard]] static constexpr bool empty() noexcept
    {
        return N == 0;
    }

    [[nodiscard]] static constexpr size_type size() noexcept
    {
        return N;
    }

    [[nodiscard]] static constexpr size_type max_size() noexcept
    {
        return N;
    }

    constexpr void swap(Array &other) noexcept(N == 0 || std::is_nothrow_swappable_v<T>)
    {
        std::ranges::swap(*this, other);
    }

    [[nodiscard]] bool operator==(Array const &) const = default;
    [[nodiscard]] auto operator<=>(Array const &) const = default;

    array_storage_t<T, N> _array;

private:
    [[nodiscard]] constexpr bool isIndexInRange(size_type pos) const noexcept
    {
        return pos < N;
    }
};

template<class T, class ...U>
Array(T, U...) -> Array<T, 1 + sizeof...(U)>;

template<std::size_t I, class T, std::size_t N>
requires (I < N)
[[nodiscard]] constexpr T &get(Array<T, N> &a) noexcept
{
    return a[I];
}

template<std::size_t I, class T, std::size_t N>
requires (I < N)
[[nodiscard]] constexpr T &&get(Array<T, N> &&a) noexcept
{
    return std::move(a[I]);
}

template<std::size_t I, class T, std::size_t N>
requires (I < N)
[[nodiscard]] constexpr T const &get(Array<T, N> const &a) noexcept
{
    return a[I];
}

template<std::size_t I, class T, std::size_t N>
requires (I < N)
[[nodiscard]] constexpr T const &&get(Array<T, N> const &&a) noexcept
{
    return a[I];
}
} // export namespace pl

template<class T, std::size_t N>
struct std::tuple_size<pl::Array<T, N>> : std::integral_constant<std::size_t, N> {};

template<std::size_t I, class T, std::size_t N>
struct std::tuple_element<I, pl::Array<T, N>>
{
    using type = T;
};

namespace std
{
export using std::operator+;
export using std::operator-;
export using std::operator==;
export using std::operator!=;
export using std::operator<;
export using std::operator>;
export using std::operator<=;
export using std::operator>=;
export using std::operator<=>;
} // namespace std

namespace pl
{
static_assert(std::contiguous_iterator<ArrayIterator<int, 5>>);
static_assert(std::contiguous_iterator<const_iterator<ArrayIterator<int, 5>>>);
static_assert(std::random_access_iterator<std::reverse_iterator<ArrayIterator<int, 5>>>);
static_assert(std::random_access_iterator<std::reverse_iterator<const_iterator<ArrayIterator<int, 5>>>>);
} // namespace pl
