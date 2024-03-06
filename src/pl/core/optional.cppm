module;
#include <cassert>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

export module pl.core:optional;

import :null;
import :tags;
import :traits;

export namespace pl
{
template<class T>
class Optional 
{
public:
    [[nodiscard]] 
    constexpr Optional() noexcept: _none(), _hasValue(false) {}

    [[nodiscard]] 
    constexpr Optional(tags::nullopt_t) noexcept: _none(), _hasValue(false) {}

    template<class U = T>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<U, T>)
    constexpr Optional(U &&u)
    noexcept(std::is_nothrow_constructible_v<T, U>)
    requires(
        std::is_constructible_v<T, U>
     && !traits::match_any_v<std::remove_cvref_t<U>, tags::nullopt_t, tags::in_place_t, Optional>)
    : _obj(std::forward<U>(u)), _hasValue(true)
    {}

    template<class ...Args>
    [[nodiscard]] 
    explicit(!traits::is_implicitly_constructible_v<T, Args...>)
    constexpr Optional(tags::in_place_t, Args &&...args)
    noexcept(std::is_nothrow_constructible_v<T, Args...>)
    requires std::is_constructible_v<T, Args...>
    : _obj(std::forward<Args>(args)...), _hasValue(true)
    {}

    template<class U, class ...Args>
    [[nodiscard]] 
    explicit(!traits::is_implicitly_constructible_v<T, std::initializer_list<U>, Args...>)
    constexpr Optional(tags::in_place_t, std::initializer_list<U> ilist, Args &&...args)
    noexcept(std::is_nothrow_constructible_v<T, std::initializer_list<U>, Args...>)
    requires std::is_constructible_v<T, std::initializer_list<U>, Args...>
    : _obj(std::move(ilist), std::forward<Args>(args)...), _hasValue(true)
    {}

    [[nodiscard]] 
    explicit(!std::is_convertible_v<T const &, T>)
    constexpr Optional(Optional const &other)
    noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires std::is_copy_constructible_v<T>
    : _none(), _hasValue(other.has_value())
    {
        if (other) std::construct_at(&_obj, *other);
    }

    [[nodiscard]] 
    explicit(!std::is_convertible_v<T, T>)
    constexpr Optional(Optional &&other)
    noexcept(std::is_nothrow_move_constructible_v<T>)
    requires std::is_move_constructible_v<T>
    : _none(), _hasValue(other.has_value())
    {
        if (other) std::construct_at(&_obj, *std::move(other));
    }

    template<class U>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<U const &, T>)
    constexpr Optional(Optional<U> const &other)
    noexcept(std::is_nothrow_constructible_v<T, U const &>)
    requires(
        std::is_nothrow_constructible_v<T, U const &>
     && !traits::is_convertible_from_wrapper_v<T, Optional<U>>)
    : _none(), _hasValue(other.has_value())
    {
        if (other) std::construct_at(&_obj, *other);
    }

    template<class U>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<U, T>)
    constexpr Optional(Optional<U> &&other)
    noexcept(std::is_nothrow_constructible_v<T, U>)
    requires(
        std::is_constructible_v<T, U>
     && !traits::is_convertible_from_wrapper_v<T, Optional<U>>)
    : _none(), _hasValue(other.has_value())
    {
        if (other) std::construct_at(&_obj, *std::move(other));
    }

    constexpr Optional &operator=(tags::nullopt_t) noexcept
    {
        if (_hasValue)
        {
            std::destroy_at(&_obj);
            std::construct_at(&_none);
            _hasValue = false;
        }
        return *this;
    }

    template<class U = T>
    constexpr Optional &operator=(U &&u)
    noexcept(
        std::is_nothrow_constructible_v<T, U>
     && std::is_nothrow_assignable_v<T &, U>)
    requires(
        std::is_constructible_v<T, U>
     && std::is_assignable_v<T &, U>
     && !traits::match_any_v<std::remove_cvref_t<U>, tags::nullopt_t, Optional>)
    {
        if (_hasValue)
            _obj = std::forward<U>(u);
        else
        {
            std::construct_at(&_obj, std::forward<U>(u));
            _hasValue = true;
        }
        return *this;
    }

    constexpr Optional &operator=(Optional const &other)
    noexcept(
        std::is_nothrow_copy_constructible_v<T>
     && std::is_nothrow_copy_assignable_v<T>)
    requires(
        std::is_copy_constructible_v<T>
     && std::is_copy_assignable_v<T>)
    {
        if (_hasValue)
        {
            if (other)
                _obj = *other;
            else
            {
                std::destroy_at(&_obj);
                std::construct_at(&_none);
                _hasValue = false;
            }
        }
        else
        {
            if (other)
            {
                std::construct_at(&_obj, *other);
                _hasValue = true;
            }
        }
        return *this;
    }

    constexpr Optional &operator=(Optional &&other)
    noexcept(
        std::is_nothrow_move_constructible_v<T>
     && std::is_nothrow_move_assignable_v<T>)
    requires(
        std::is_move_constructible_v<T>
     && std::is_move_assignable_v<T>)
    {
        if (_hasValue)
        {
            if (other)
                _obj = *std::move(other);
            else
            {
                std::destroy_at(&_obj);
                std::construct_at(&_none);
                _hasValue = false;
            }
        }
        else
        {
            if (other)
            {
                std::construct_at(&_obj, *std::move(other));
                _hasValue = true;
            }
        }
        return *this;
    }

    template<class U>
    constexpr Optional &operator=(Optional<U> const &other)
    noexcept(
        std::is_nothrow_constructible_v<T, U const &>
     && std::is_nothrow_assignable_v<T &, U const &>)
    requires(
        std::is_constructible_v<T, U const &>
     && std::is_assignable_v<T &, U const &>
     && !traits::is_convertible_from_wrapper_v<T, Optional<U>>
     && !traits::is_assignable_from_wrapper_v<T, Optional<U>>)
    {
        if (_hasValue)
        {
            if (other)
                _obj = *other;
            else
            {
                std::destroy_at(&_obj);
                std::construct_at(&_none);
                _hasValue = false;
            }
        }
        else
        {
            if (other)
            {
                std::construct_at(&_obj, *other);
                _hasValue = true;
            }
        }
        return *this;
    }

    template<class U>
    constexpr Optional &operator=(Optional<U> &&other)
    noexcept(
        std::is_nothrow_constructible_v<T, U>
     && std::is_nothrow_assignable_v<T &, U>)
    requires(
        std::is_constructible_v<T, U>
     && std::is_assignable_v<T &, U>
     && !traits::is_convertible_from_wrapper_v<T, Optional<U>>
     && !traits::is_assignable_from_wrapper_v<T, Optional<U>>)
    {
        if (_hasValue)
        {
            if (other)
                _obj = *std::move(other);
            else
            {
                std::destroy_at(&_obj);
                std::construct_at(&_none);
                _hasValue = false;
            }
        }
        else
        {
            if (other)
            {
                std::construct_at(&_obj, *std::move(other));
                _hasValue = true;
            }
        }
        return *this;
    }

    constexpr ~Optional()
    {
        if (_hasValue) std::destroy_at(&_obj);
    }

    [[nodiscard]] constexpr T &operator*() & noexcept
    {
        assert(_hasValue);
        return _obj;
    }

    [[nodiscard]] constexpr T const &operator*() const & noexcept
    {
        assert(_hasValue);
        return _obj;
    }

    [[nodiscard]] constexpr T &&operator*() && noexcept
    {
        assert(_hasValue);
        return std::move(_obj);
    }

    [[nodiscard]] constexpr T const &&operator*() const && noexcept
    {
        assert(_hasValue);
        return std::move(_obj);
    }

    [[nodiscard]] constexpr T *operator->() noexcept
    {
        assert(_hasValue);
        return &_obj;
    }

    [[nodiscard]] constexpr T const *operator->() const noexcept
    {
        assert(_hasValue);
        return &_obj;
    }

    [[nodiscard]] explicit constexpr operator bool() const noexcept
    {
        return _hasValue;
    }

    [[nodiscard]] constexpr bool has_value() const noexcept
    {
        return _hasValue;
    }

private:
    union
    {
        tags::none_t _none;
        T _obj;
    };
    bool _hasValue;
};

template<hidden_nullable T>
class Optional<T>
{
private:
    using null_trait = decltype(plHiddenNullable(std::declval<T const &>()));

public:
    [[nodiscard]] 
    constexpr Optional()
    noexcept(std::is_nothrow_move_constructible_v<T>)
    requires std::is_move_constructible_v<T>
    : _obj(null_trait::null())
    {}

    [[nodiscard]] 
    constexpr Optional(tags::nullopt_t) noexcept(Optional()): Optional() {}

    template<class U = T>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<U, T>)
    constexpr Optional(U &&u)
    noexcept(std::is_nothrow_constructible_v<T, U>)
    requires(
        std::is_constructible_v<T, U>
     && !traits::match_any_v<std::remove_cvref_t<U>, tags::nullopt_t, tags::in_place_t, Optional>)
    : _obj(std::forward<U>(u))
    {}

    template<class ...Args>
    [[nodiscard]] 
    explicit(!traits::is_implicitly_constructible_v<T, Args...>)
    constexpr Optional(tags::in_place_t, Args &&...args)
    noexcept(std::is_nothrow_constructible_v<T, Args...>)
    requires std::is_constructible_v<T, Args...>
    : _obj(std::forward<Args>(args)...)
    {}

    template<class U, class ...Args>
    [[nodiscard]] 
    explicit(!traits::is_implicitly_constructible_v<T, std::initializer_list<U>, Args...>)
    constexpr Optional(tags::in_place_t, std::initializer_list<U> ilist, Args &&...args)
    noexcept(std::is_nothrow_constructible_v<T, std::initializer_list<U>, Args...>)
    requires std::is_constructible_v<T, std::initializer_list<U>, Args...>
    : _obj(std::move(ilist), std::forward<Args>(args)...)
    {}

    [[nodiscard]] 
    explicit(!std::is_convertible_v<T const &, T>)
    constexpr Optional(Optional const &other)
    noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires std::is_copy_constructible_v<T>
    : _obj(*other)
    {}

    [[nodiscard]] 
    explicit(!std::is_convertible_v<T, T>)
    constexpr Optional(Optional &&other)
    noexcept(std::is_nothrow_move_constructible_v<T>)
    requires std::is_move_constructible_v<T>
    : _obj(*std::move(other))
    {}

    template<class U>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<U const &, T>)
    constexpr Optional(Optional<U> const &other)
    noexcept(std::is_nothrow_constructible_v<T, U const &>)
    requires(
        std::is_constructible_v<T, U const &>
     && !traits::is_convertible_from_wrapper_v<T, Optional<U>>)
    : _obj(other ? T(*other) : null_trait::null())
    {}

    template<class U>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<U, T>)
    constexpr Optional(Optional<U> &&other)
    noexcept(std::is_nothrow_constructible_v<T, U>)
    requires(
        std::is_constructible_v<T, U>
     && !traits::is_convertible_from_wrapper_v<T, Optional<U>>)
    : _obj(other ? T(*std::move(other)) : null_trait::null())
    {}

    constexpr Optional &operator=(tags::nullopt_t) noexcept
    {
        null_trait::assignNull(_obj);
        return *this;
    }

    template<class U = T>
    constexpr Optional &operator=(U &&u)
    noexcept(std::is_nothrow_assignable_v<T &, U>)
    requires(
        std::is_assignable_v<T &, U>
     && !traits::match_any_v<std::remove_cvref_t<U>, tags::nullopt_t, Optional>)
    {
        _obj = std::forward<U>(u);
        return *this;
    }

    constexpr Optional &operator=(Optional const &other)
    noexcept(std::is_nothrow_copy_assignable_v<T>)
    requires std::is_copy_assignable_v<T>
    {
        if (other)
            _obj = *other;
        else
            null_trait::assignNull(_obj);
        return *this;
    }

    constexpr Optional &operator=(Optional &&other)
    noexcept(std::is_nothrow_move_assignable_v<T>)
    requires std::is_move_assignable_v<T>
    {
        if (other)
            _obj = *std::move(other);
        else
            null_trait::assignNull(_obj);
        return *this;
    }

    template<class U>
    constexpr Optional &operator=(Optional<U> const &other)
    noexcept(std::is_nothrow_assignable_v<T &, U const &>)
    requires std::is_assignable_v<T &, U const &>
    {
        if (other)
            _obj = *other;
        else
            null_trait::assignNull(_obj);
        return *this;
    }

    template<class U>
    constexpr Optional &operator=(Optional<U> &&other)
    noexcept(std::is_nothrow_assignable_v<T &, U>)
    requires std::is_assignable_v<T &, U const &>
    {
        if (other)
            _obj = *std::move(other);
        else
            null_trait::assignNull(_obj);
        return *this;
    }

    [[nodiscard]] constexpr T &operator*() & noexcept
    {
        assert(hasValue());
        return _obj;
    }

    [[nodiscard]] constexpr T const &operator*() const & noexcept
    {
        assert(hasValue());
        return _obj;
    }

    [[nodiscard]] constexpr T &&operator*() && noexcept
    {
        assert(hasValue());
        return std::move(_obj);
    }

    [[nodiscard]] constexpr T const &&operator*() const && noexcept
    {
        assert(hasValue());
        return std::move(_obj);
    }

    [[nodiscard]] constexpr T *operator->() noexcept
    {
        assert(hasValue());
        return &_obj;
    }

    [[nodiscard]] constexpr T const *operator->() const noexcept
    {
        assert(hasValue());
        return &_obj;
    }

    [[nodiscard]] explicit constexpr operator bool() const noexcept
    {
        return null_trait::isNull(_obj);
    }

    [[nodiscard]] constexpr bool hasValue() const noexcept
    {
        return null_trait::isNull(_obj);
    }
private:
    T _obj;
};

template<class T>
Optional(T) -> Optional<T>;

template<class T>
using Opt = Optional<T>;
} // export namespace pl
namespace pl
{
static_assert(hidden_nullable<Ptr<int>>);
static_assert(sizeof(Opt<Ptr<int>>) == sizeof(int*));
}
