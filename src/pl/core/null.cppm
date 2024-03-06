module;
#include <cassert>
#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

export module pl.core:null;

import :error;
import :result_error;

export namespace pl
{
template<class T>
struct PointerNullTrait
{
    [[nodiscard]] static constexpr bool isNull(T *ptr) noexcept { return ptr == nullptr; }
    [[nodiscard]] static constexpr T     *null()       noexcept { return nullptr;        }
};

template<class T>
PointerNullTrait<T> plNullable(T*);

template<class Trait, class T>
concept null_trait = requires (T const ct)
{
    { Trait::isNull(ct) } noexcept -> std::convertible_to<bool>;
    { Trait::null()     } noexcept -> std::same_as<T>;
};

template<class T>
concept nullable = requires (T const ct)
{
    { plNullable(ct) } -> null_trait<T>;
};

template<class Trait, class T>
concept hidden_null_trait = requires(T t, T const ct)
{
    { Trait::isNull(ct)    } noexcept -> std::convertible_to<bool>;
    { Trait::null()        } noexcept -> std::same_as<T>;
    { Trait::assignNull(t) } noexcept;
};

template<class T>
concept hidden_nullable = requires (T const ct)
{
    { plHiddenNullable(ct) } -> null_trait<T>;
};

class NullError : public SuberrorType<>
{
public:
    using SuberrorType::SuberrorType;

    [[nodiscard]] 
    constexpr char const *name() const noexcept final
    {
        return "NullError";
    }
};

template<nullable T>
class NonNull;

template<nullable T>
struct NonNullHiddenNullTrait;

template<nullable T>
requires std::is_scalar_v<T>
class NonNull<T>
{
public:
    using null_trait = decltype(plNullable(std::declval<T const &>()));

    [[nodiscard]] 
    static constexpr RE<NonNull, SimpleError> make(T obj) noexcept
    {
        if (null_trait::isNull(obj))
            return {tags::error, getSingleton<NullError>()};
        else
            return NonNull(obj);
    }

    [[nodiscard]] 
    static constexpr NonNull make_Unchecked(T obj) noexcept
    {
        assert(!null_trait::isNull(obj));
        return NonNull(obj);
    }

    [[nodiscard]] NonNull           (NonNull const  &) = default;
    [[nodiscard]] NonNull           (NonNull       &&) = default;
                  NonNull &operator=(NonNull const  &) = default;
                  NonNull &operator=(NonNull       &&) = default;

    template<class U>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<U const &, T>)
    constexpr NonNull(NonNull<U> const &other)
    noexcept(std::is_nothrow_constructible_v<T, U const &>)
    requires std::is_constructible_v<T, U const &>
    : _obj(static_cast<U const &>(other))
    {}

    template<class U>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<U &&, T>)
    constexpr NonNull(NonNull<U> &&other)
    noexcept(std::is_nothrow_constructible_v<T, U>)
    requires std::is_constructible_v<T, U>
    : _obj(static_cast<U &&>(std::move(other)))
    {}

    template<class U>
    constexpr NonNull &operator=(NonNull<U> const &other)
    noexcept(std::is_nothrow_assignable_v<T &, U const &>)
    requires std::is_assignable_v<T &, U const &>
    {
        _obj = static_cast<U const &>(other);
        return *this;
    }

    template<class U>
    constexpr NonNull &operator=(NonNull<U> &&other)
    noexcept(std::is_nothrow_assignable_v<T &, U>)
    requires std::is_assignable_v<T &, U>
    {
        _obj = static_cast<U &&>(std::move(other));
        return *this;
    }

    [[nodiscard]] 
    constexpr operator T() const noexcept { return _obj; }

private:
    friend struct NonNullHiddenNullTrait<T>;

    [[nodiscard]] 
    explicit constexpr NonNull() noexcept(noexcept(null_trait::null())) : _obj(null_trait::null()) {}

    [[nodiscard]] 
    explicit constexpr NonNull(T obj) noexcept : _obj(obj) {}

    constexpr NonNull &operator=(T obj) noexcept
    {
        _obj = obj;
        return *this;
    }

    T _obj;
};

template<nullable T>
requires std::is_class_v<T>
class NonNull<T> : public T
{
public:
    using null_trait = decltype(plNullable(std::declval<T const &>()));

    [[nodiscard]] 
    static constexpr RE<NonNull, SimpleError> make(T const &obj)
    noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        if (null_trait::isNull(obj))
            return {tags::error, getSingleton<NullError>()};
        else
            return NonNull(obj);
    }

    [[nodiscard]] 
    static constexpr RE<NonNull, SimpleError> make(T &&obj)
    noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if (null_trait::isNull(obj))
            return {tags::error, getSingleton<NullError>()};
        else
            return NonNull(std::move(obj));
    }

    [[nodiscard]] 
    static constexpr NonNull make_Unchecked(T const &obj)
    noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        assert(!null_trait::isNull(obj));
        return NonNull(obj);
    }

    [[nodiscard]] 
    static constexpr NonNull make_Unchecked(T &&obj)
    noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        assert(!null_trait::isNull(obj));
        return NonNull(std::move(obj));
    }

    [[nodiscard]] NonNull           (NonNull const  &) = default;
    [[nodiscard]] NonNull           (NonNull       &&) = default;
                  NonNull &operator=(NonNull const  &) = default;
                  NonNull &operator=(NonNull       &&) = default;

    template<class U>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<U const &, T>)
    constexpr NonNull(NonNull<U> const &other)
    noexcept(std::is_nothrow_constructible_v<T, U const &>)
    requires std::is_constructible_v<T, U const &>
    : T(static_cast<U const &>(other))
    {}

    template<class U>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<U &&, T>)
    constexpr NonNull(NonNull<U> &&other)
    noexcept(std::is_nothrow_constructible_v<T, U>)
    requires std::is_constructible_v<T, U>
    : T(static_cast<U &&>(std::move(other)))
    {}

    template<class U>
    constexpr NonNull &operator=(NonNull<U> const &other)
    noexcept(std::is_nothrow_assignable_v<T &, U const &>)
    requires std::is_assignable_v<T &, U const &>
    {
        T::operator=(static_cast<U const &>(other));
        return *this;
    }

    template<class U>
    constexpr NonNull &operator=(NonNull<U> &&other)
    noexcept(std::is_nothrow_assignable_v<T &, U>)
    requires std::is_assignable_v<T &, U>
    {
        T::operator=(static_cast<U &&>(std::move(other)));
        return *this;
    }

private:
    friend struct NonNullHiddenNullTrait<T>;

    [[nodiscard]] 
    explicit constexpr NonNull()
    noexcept(noexcept(null_trait::null()) && std::is_nothrow_move_constructible_v<T>)
    requires std::is_move_constructible_v<T>
    : T(null_trait::null())
    {}

    [[nodiscard]] 
    explicit constexpr NonNull(T const &other)
    noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires std::is_copy_constructible_v<T>
    : T(other)
    {}

    [[nodiscard]] 
    explicit constexpr NonNull(T &&other)
    noexcept(std::is_nothrow_move_constructible_v<T>)
    requires std::is_move_constructible_v<T>
    : T(std::move(other))
    {}

    constexpr NonNull &operator=(T const &other)
    noexcept(std::is_nothrow_copy_assignable_v<T>)
    requires std::is_copy_assignable_v<T>
    {
        T::operator=(other);
        return *this;
    }

    constexpr NonNull &operator=(T &&other)
    noexcept(std::is_nothrow_move_assignable_v<T>)
    requires std::is_move_assignable_v<T>
    {
        T::operator=(std::move(other));
        return *this;
    }
};

template<class T>
[[nodiscard]] 
constexpr RE<NonNull<std::remove_cvref_t<T>>, SimpleError> makeNonNull(T &&obj)
noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<T>, T>)
requires nullable<std::remove_cvref_t<T>>
{
    return NonNull<std::remove_cvref_t<T>>::make(std::forward<T>(obj));
}

template<class T>
[[nodiscard]] 
constexpr NonNull<std::remove_cvref_t<T>> makeNonNull_Unchecked(T &&obj)
noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<T>, T>)
requires nullable<std::remove_cvref_t<T>>
{
    return NonNull<std::remove_cvref_t<T>>::make_Unchecked(std::forward<T>(obj));
}

template<nullable T>
struct NonNullHiddenNullTrait
{
    using null_trait = NonNull<T>::null_trait;

    [[nodiscard]] 
    static constexpr bool isNull(NonNull<T> const &n)
    noexcept(noexcept(null_trait::isNull(std::declval<T const &>())))
    {
        return null_trait::isNull(static_cast<T const &>(n));
    }

    [[nodiscard]] 
    static constexpr NonNull<T> null()
    noexcept(noexcept(null_trait::null()))
    {
        return NonNull<T>(null_trait::null());
    }

    static constexpr void assignNull(NonNull<T> &n)
    noexcept(noexcept(null_trait::null()))
    {
        n = null_trait::null();
    }
};

template<class T>
NonNullHiddenNullTrait<T> plHiddenNullable(NonNull<T> const &);

template<class T>
using Ptr = NonNull<T*>;

template<class T>
[[nodiscard]] 
constexpr Ptr<T> addr(T &t) noexcept
{
    return makeNonNull_Unchecked(std::addressof(t));
}
} // export namespace pl
