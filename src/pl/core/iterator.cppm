module;
#include <iterator>
#include <type_traits>

#include <pl/macro.hpp>

export module pl.core:iterator;

import :traits;

namespace pl
{
template<std::indirectly_readable T>
using iter_const_reference_t = std::common_reference_t<std::iter_value_t<T> const &&,
                                                       std::iter_reference_t<T>>;
template<class T>
concept constant_iterator = std::input_iterator<T> && std::same_as<iter_const_reference_t<T>,
                                                                   std::iter_reference_t<T>>;

namespace basic_const_iterator_
{
template<class Iter>
struct trait_iterator_category {};

template<std::forward_iterator Iter>
struct trait_iterator_category<Iter>
{
    using iterator_category = std::iterator_traits<Iter>::iterator_category;
};

template<class Iter>
struct trait_iterator_concept
{
    using iterator_concept = std::input_iterator_tag;
};

template<std::contiguous_iterator Iter>
struct trait_iterator_concept<Iter>
{
    using iterator_concept = std::contiguous_iterator_tag;
};

template<std::random_access_iterator Iter>
struct trait_iterator_concept<Iter>
{
    using iterator_concept = std::random_access_iterator_tag;
};

template<std::bidirectional_iterator Iter>
struct trait_iterator_concept<Iter>
{
    using iterator_concept = std::bidirectional_iterator_tag;
};

template<std::forward_iterator Iter>
struct trait_iterator_concept<Iter>
{
    using iterator_concept = std::forward_iterator_tag;
};

template<class Iter>
struct PL_API_ATTR_EMPTY_BASES traits :
    trait_iterator_category<Iter>,
    trait_iterator_concept<Iter>
{
    using value_type = std::iter_value_t<Iter>;
    using difference_type = std::iter_difference_t<Iter>;
    using reference = iter_const_reference_t<Iter>;
};
} // namespace basic_const_iterator_
} // namespace pl

export namespace pl
{
template<std::input_iterator Iter>
class basic_const_iterator : public basic_const_iterator_::traits<Iter>
{
private:
    using Traits = basic_const_iterator_::traits<Iter>;

public:
    using typename Traits::difference_type;
    using typename Traits::value_type;
    using typename Traits::reference;

    using rvalue_reference = std::common_reference_t<
            std::iter_value_t<Iter> const &&,
            std::iter_rvalue_reference_t<Iter>>;


    basic_const_iterator() requires std::default_initializable<Iter> = default;
    basic_const_iterator(basic_const_iterator const &)               = default;
    basic_const_iterator(basic_const_iterator &&)                    = default;

    constexpr basic_const_iterator(Iter it) : _it(std::move(it)) {}

    template<std::convertible_to<Iter> U>
    constexpr basic_const_iterator(basic_const_iterator<U> other) : _it(std::move(other._it)) {}

    template<class T>
    requires (!traits::specialization_of<std::remove_cvref_t<T>, basic_const_iterator>)
    constexpr basic_const_iterator(T &&it) : _it(std::forward<T>(it)) {}

    basic_const_iterator &operator=(basic_const_iterator const &) = default;
    basic_const_iterator &operator=(basic_const_iterator &&) = default;

    constexpr Iter const &base() const & noexcept { return _it; }
    constexpr Iter base() && { return std::move(_it); }

    constexpr reference operator*() const
    {
        return static_cast<reference>(*base());
    }

    constexpr auto const *operator->() const
    requires std::is_lvalue_reference_v<std::iter_reference_t<Iter>>
          && std::same_as<std::remove_cvref_t<std::iter_reference_t<Iter>>, value_type>
    {
        if constexpr (std::contiguous_iterator<Iter>)
            return std::to_address(base());
        else
            return std::addressof(*base());
    }

    constexpr reference operator[](difference_type d) const
    requires std::random_access_iterator<Iter>
    {
        return static_cast<reference>(base()[d]);
    }

    constexpr basic_const_iterator &operator++()
    {
        ++_it;
        return *this;
    }

    constexpr void operator++(int)
    {
        ++_it;
    }

    constexpr basic_const_iterator operator++(int)
    requires std::forward_iterator<Iter>
    {
        auto copy = *this;
        ++_it;
        return copy;
    }

    constexpr basic_const_iterator &operator--()
    requires std::bidirectional_iterator<Iter>
    {
        --_it;
        return *this;
    }

    constexpr basic_const_iterator operator--(int)
    {
        auto copy = *this;
        --_it;
        return copy;
    }

    constexpr basic_const_iterator &operator+=(difference_type d)
    requires std::random_access_iterator<Iter>
    {
        _it += d;
        return *this;
    }

    constexpr basic_const_iterator &operator-=(difference_type d)
    requires std::random_access_iterator<Iter>
    {
        _it -= d;
        return *this;
    }

    template<std::sentinel_for<Iter> S>
    constexpr bool operator==(S const &s) const
    {
        return base() == s;
    }

    constexpr bool operator<(basic_const_iterator const &other) const
    requires std::random_access_iterator<Iter>
    {
        return base() < other.base();
    }

    constexpr bool operator>(basic_const_iterator const &other) const
    requires std::random_access_iterator<Iter>
    {
        return base() > other.base();
    }

    constexpr bool operator<=(basic_const_iterator const &other) const
    requires std::random_access_iterator<Iter>
    {
        return base() <= other.base();
    }

    constexpr bool operator>=(basic_const_iterator const &other) const
    requires std::random_access_iterator<Iter>
    {
        return base() >= other.base();
    }

    constexpr auto operator<=>(basic_const_iterator const &other) const
    requires std::random_access_iterator<Iter> && std::three_way_comparable<Iter>
    {
        return base() <=> other.base();
    }

    template<class T>
    constexpr bool operator<(T const &other) const
    requires (
        !std::same_as<T, basic_const_iterator> 
     && std::random_access_iterator<Iter>
     && std::totally_ordered_with<Iter, T>)
    {
        return base() < other;
    }

    template<class T>
    constexpr bool operator>(T const &other) const
    requires (
        !std::same_as<T, basic_const_iterator> 
     && std::random_access_iterator<Iter>
     && std::totally_ordered_with<Iter, T>)
    {
        return base() > other;
    }

    template<class T>
    constexpr bool operator<=(T const &other) const
    requires (
        !std::same_as<T, basic_const_iterator> 
     && std::random_access_iterator<Iter>
     && std::totally_ordered_with<Iter, T>)
    {
        return base() <= other;
    }

    template<class T>
    constexpr bool operator>=(T const &other) const
    requires (
        !std::same_as<T, basic_const_iterator> 
     && std::random_access_iterator<Iter>
     && std::totally_ordered_with<Iter, T>)
    {
        return base() >= other;
    }

    template<class T>
    constexpr auto operator<=>(T const &other) const
    requires (
        !std::same_as<T, basic_const_iterator> 
     && std::random_access_iterator<Iter>
     && std::totally_ordered_with<Iter, T>
     && std::three_way_comparable<Iter, T>)
    {
        return base() <=> other;
    }

    friend constexpr basic_const_iterator operator+(basic_const_iterator const &i, difference_type d)
    requires std::random_access_iterator<Iter>
    {
        return {i.base() + d};
    }

    friend constexpr basic_const_iterator operator+(difference_type d, basic_const_iterator const &i)
    requires std::random_access_iterator<Iter>
    {
        return {d + i.base()};
    }

    friend constexpr basic_const_iterator operator-(basic_const_iterator const &i, difference_type d)
    requires std::random_access_iterator<Iter>
    {
        return {i.base() - d};
    }

    friend constexpr difference_type operator-(
        basic_const_iterator const &a, basic_const_iterator const &b)
    {
        return a.base() - b.base();
    }

    friend constexpr rvalue_reference iter_move(basic_const_iterator const &i)
    noexcept(noexcept(static_cast<rvalue_reference>(std::ranges::iter_move(i.base()))))
    {
        return static_cast<rvalue_reference>(std::ranges::iter_move(i.base()));
    }

private:
    template<std::input_iterator>
    friend class basic_const_iterator;

    Iter _it;
};
} // export namespace pl

namespace pl
{
template<std::input_iterator I>
struct const_iterator_helper
{
    using type = basic_const_iterator<I>;
};

template<constant_iterator I>
struct const_iterator_helper<I>
{
    using type = I;
};
} // namespace pl

export namespace pl
{
template<std::input_iterator I>
using const_iterator = const_iterator_helper<I>::type;
} // export namespace pl

namespace pl
{
template<std::semiregular S>
struct const_sentinel_helper
{
    using type = S;
};

template<std::semiregular S>
requires std::input_iterator<S>
struct const_sentinel_helper<S>
{
    using type = const_iterator<S>;
};
} // namespace pl

export namespace pl
{
template<std::semiregular S>
using const_sentinel = const_sentinel_helper<S>::type;

template<std::input_iterator I>
constexpr const_iterator<I> make_const_iterator(I it) { return it; }

template<std::semiregular S>
constexpr const_sentinel<S> make_const_sentinel(S s) { return s; } 
} // export namespace pl

export namespace std
{
template<class T, common_with<T> U>
requires input_iterator<common_type<T, U>>
struct common_type<::pl::basic_const_iterator<T>, U>
{
    using type = ::pl::basic_const_iterator<common_type_t<T, U>>;
};

template<class T, common_with<T> U>
requires input_iterator<common_type<T, U>>
struct common_type<U, ::pl::basic_const_iterator<T>>
{
    using type = ::pl::basic_const_iterator<common_type_t<T, U>>;
};

template<class T, common_with<T> U>
requires input_iterator<common_type<T, U>>
struct common_type<::pl::basic_const_iterator<T>,
                   ::pl::basic_const_iterator<U>>
{
    using type = ::pl::basic_const_iterator<common_type_t<T, U>>;
};
} // export namespace std
