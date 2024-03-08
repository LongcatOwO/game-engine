module;
#include <cassert>
#include <cstddef>
#include <iterator>
// #include <memory>
// #include <ranges>
#include <type_traits>
// 
// #include <vector>


export module pl.core:span;

export import :tags;

import :concepts;
import :iterator;
import :traits;


export namespace pl
{
template<class T, std::size_t Extent>
class Span;
} // export namespace pl

namespace pl
{
template<class T, std::size_t Extent>
class SpanIterator
{
public:
    using iterator_concept = std::contiguous_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using element_type = T;
    using reference = T &;
    using pointer = T *;
    
    SpanIterator()                                = default;
    SpanIterator(SpanIterator const &)            = default;
    SpanIterator &operator=(SpanIterator const &) = default;

    constexpr SpanIterator(
        T *p
#ifndef NDEBUG
        ,
        Span<T, Extent> const &s
#endif
    ) noexcept
    :
        _p(p)
#ifndef NDEBUG
        ,
        _begin(s.data()),
        _end(initEnd(s))
#endif
    {}

    constexpr SpanIterator &operator++() noexcept
    {
        assert(isOffsetInValidRange(1));
        ++_p;
        return *this;
    }

    constexpr SpanIterator operator++(int) noexcept
    {
        assert(isOffsetInValidRange(1));
        auto copy = *this;
        ++_p;
        return copy;
    }

    constexpr SpanIterator &operator--() noexcept
    {
        assert(isOffsetInValidRange(-1));
        --_p;
        return *this;
    }

    constexpr SpanIterator operator--(int) noexcept
    {
        assert(isOffsetInValidRange(-1));
        auto copy = *this;
        --_p;
        return copy;
    }

    constexpr SpanIterator &operator+=(difference_type d) noexcept
    {
        assert(isOffsetInValidRange(d));
        _p += d;
        return *this;
    }

    constexpr SpanIterator &operator-=(difference_type d) noexcept
    {
        assert(isOffsetInValidRange(-d));
        _p -= d;
        return *this;
    }

    friend constexpr SpanIterator operator+(SpanIterator i, difference_type d) noexcept
    {
        assert(i.isOffsetInValidRange(d));
        i._p += d;
        return i;
    }

    friend constexpr SpanIterator operator+(difference_type d, SpanIterator i) noexcept
    {
        assert(i.isOffsetInValidRange(d));
        i._p += d;
        return i;
    }

    friend constexpr SpanIterator operator-(SpanIterator i, difference_type d) noexcept
    {
        assert(i.isOffsetInValidRange(-d));
        i._p -= d;
        return i;
    }

    friend constexpr difference_type operator-(SpanIterator a, SpanIterator b) noexcept
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

    constexpr bool operator==(SpanIterator const &other) const noexcept
    {
        assert(_begin == other._begin);
        return _p == other._p;
    }

    constexpr auto operator<=>(SpanIterator const &other) const noexcept
    {
        assert(_begin == other._begin);
        return _p <=> other._p;
    }
    
private:
    static constexpr auto initEnd(Span<T, Extent> const &s) noexcept
    {
        if constexpr (Extent == tags::dynamic_extent)
            return s.data() + s.size();
        else
            return tags::none;
    }

#ifndef NDEBUG
    constexpr std::size_t size() const noexcept
    {
        if constexpr (Extent == tags::dynamic_extent)
            return std::size_t(_end - _begin);
        else
            return Extent;
    }

    constexpr bool isOffsetInValidRange(difference_type d) const noexcept
    {
        auto new_index = _p - _begin + d;
        return 0 <= new_index && (std::size_t) new_index <= size();
    }

    constexpr bool isOffsetInReferencableRange(difference_type d) const noexcept
    {
        auto new_index = _p - _begin + d;
        return 0 <= new_index && (std::size_t) new_index < size();
    }
#endif

    T *_p = {};
#ifndef NDEBUG
    T *_begin = {};

    [[no_unique_address]]
    std::conditional_t<
        Extent == tags::dynamic_extent,
        T *,
        tags::none_t>
    _end = {};
#endif
};

static_assert(std::contiguous_iterator<SpanIterator<int, 5>>);
static_assert(std::contiguous_iterator<const_iterator<SpanIterator<int, 5>>>);
} // namespace pl

export namespace pl
{
template<class T, std::size_t Extent>
class Span;

template<class T>
struct is_span : std::false_type {};

template<class T, std::size_t Extent>
struct is_span<Span<T, Extent>> : std::true_type {};

template<class T>
constexpr bool is_span_v = is_span<T>::value;


template<class T, std::size_t Extent = tags::dynamic_extent>
class Span
{
public:
    using element_type           = T;
    using value_type             = std::remove_cv_t<T>;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using pointer                = T       *;
    using const_pointer          = T const *;
    using reference              = T       &;
    using const_reference        = T const &;
    using iterator               = SpanIterator<T, Extent>;
    using const_iterator         = const_iterator<iterator>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr std::size_t extent = Extent;

    [[nodiscard]] Span()                        = default;
    [[nodiscard]] Span           (Span const &) = default;
                  Span &operator=(Span const &) = default;

    template<std::contiguous_iterator It>
    [[nodiscard]]
    explicit(extent != tags::dynamic_extent)
    constexpr Span(It first, size_type count)
    requires traits::is_qualification_convertible_v<std::iter_reference_t<It>, element_type>
    : Span(tags::impl, std::to_address(first), count) {}

    template<std::contiguous_iterator It, std::sized_sentinel_for<It> End>
    [[nodiscard]]
    explicit(extent != tags::dynamic_extent)
    constexpr Span(It first, End last)
    requires(
        !std::is_convertible_v<End, size_type>
     && traits::is_qualification_convertible_v<std::iter_reference_t<It>, element_type>)
    : Span(tags::impl, std::to_address(first), last - first)
    {}

    template<std::size_t N>
    [[nodiscard]]
    constexpr Span(std::type_identity_t<element_type> (&arr)[N]) noexcept
    requires(
        (extent == tags::dynamic_extent || extent == N)
     && traits::is_qualification_convertible_v<std::remove_pointer_t<decltype(std::data(arr))>, element_type>)
    : Span(tags::impl, std::data(arr), N)
    {}

    template<class R>
    [[nodiscard]]
    explicit(extent != tags::dynamic_extent && !static_sized<R>)
    constexpr Span(R &&range)
    requires(
        std::ranges::contiguous_range<R>
     && std::ranges::sized_range<R>
     && std::ranges::borrowed_range<R>
     && !is_span_v<std::remove_cvref_t<R>>
     && !std::is_array_v<std::remove_cvref_t<R>>
     && traits::is_qualification_convertible_v<std::ranges::range_reference_t<R>, element_type>
     && (extent == tags::dynamic_extent || !static_sized<R> || static_size_of<R, extent>))
    : Span(tags::impl, std::ranges::data(range), std::ranges::size(range))
    {}

    template<class U, std::size_t N>
    [[nodiscard]]
    explicit(extent != tags::dynamic_extent && N == tags::dynamic_extent)
    constexpr Span(Span<U, N> const &other) noexcept
    requires(
        (extent == tags::dynamic_extent || N == tags::dynamic_extent || N == extent)
     && traits::is_qualification_convertible_v<U, element_type>)
    : Span(tags::impl, other.data(), other.size())
    {}

    [[nodiscard]] constexpr iterator begin() const noexcept
    {
        return
        {
            _begin,
#ifndef NDEBUG
            *this,
#endif
        };
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept
    {
        return const_iterator(begin());
    }

    [[nodiscard]] constexpr iterator end() const noexcept
    {
        return
        {
            dataEnd(),
#ifndef NDEBUG
            *this,
#endif
        };
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept
    {
        return const_iterator(end());
    }

    [[nodiscard]] constexpr reverse_iterator rbegin() const noexcept
    {
        return reverse_iterator(end());
    }

    [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }

    [[nodiscard]] constexpr reverse_iterator rend() const noexcept
    {
        return reverse_iterator(begin());
    }

    [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

    [[nodiscard]] constexpr reference front() const noexcept
    {
        assert(size() != 0);
        return *_begin;
    }

    [[nodiscard]] constexpr reference back() const noexcept
    {
        assert(size() != 0);
        return dataEnd()[-1];
    }

    [[nodiscard]] constexpr reference operator[](size_type idx) const noexcept
    {
        assert(idx < size());
        return _begin[idx];
    }

    // Non-empty Span will NEVER have nullptr as data
    [[nodiscard]] constexpr pointer data() const noexcept
    {
        return _begin;
    }

    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return (size_type)(dataEnd() - _begin);
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return size() == 0;
    }

private:
    [[nodiscard]]
    constexpr Span(tags::impl_t, T *begin, size_type count) noexcept
    :
        _begin(begin),
        _end([&]
        {
            if constexpr (extent == tags::dynamic_extent)
                return _begin + count;
            else
            {
                assert(extent == count);
                return tags::none;
            }
        }())
    {}

    [[nodiscard]]
    constexpr pointer dataEnd() const noexcept
    {
        if constexpr (extent == tags::dynamic_extent)
            return _end;
        else
            return _begin ? _begin + extent : nullptr;
    }

    T *_begin = {};

    [[no_unique_address]]
    std::conditional_t<
        extent == tags::dynamic_extent,
        T *,
        tags::none_t>
    _end = {};
};

template<class T, std::size_t N>
Span(T (&)[N]) -> Span<T, N>;
} // export namespace pl

export namespace std
{
template<class T, size_t Extent>
constexpr bool ranges::enable_borrowed_range<::pl::Span<T, Extent>> = true;

template<class T, size_t Extent>
constexpr bool ranges::enable_view<::pl::Span<T, Extent>> = true;
} // export namespace std
