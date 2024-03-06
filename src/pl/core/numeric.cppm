module;
#include <cassert>
#include <concepts>
#include <type_traits>

export module pl.core:numeric;

import :error;
import :null;
import :optional;
import :result_error;

export namespace pl
{
template<std::integral T>
struct NonZeroHiddenNullTrait;

class ZeroError : public SuberrorType<>
{
public:
    using SuberrorType::SuberrorType;

    [[nodiscard]] 
    constexpr char const *name() const noexcept final
    {
        return "ZeroError";
    }
};

template<std::integral T>
class NonZero
{
public:
    template<T Value>
    [[nodiscard]] 
    static constexpr NonZero make() noexcept
    {
        return NonZero(Value);
    }

    [[nodiscard]] 
    static constexpr RE<NonZero, SimpleError> make(T value) noexcept
    {
        if (value == 0)
            return {tags::error, getSingleton<ZeroError>()};
        else
            return NonZero(value);
    }

    [[nodiscard]] 
    static constexpr NonZero make_Unchecked(T value) noexcept
    {
        assert(value != 0);
        return NonZero(value);
    }

    [[nodiscard]] NonZero(NonZero const &) = default;
                  NonZero &operator=(NonZero const &) = default;

    template<class U>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<U, T>)
    constexpr NonZero(NonZero<U> const &other) noexcept
    requires std::is_constructible_v<T, U>
    : _value(static_cast<T>(other)) {}

    [[nodiscard]] 
    constexpr operator T() const noexcept
    {
        return _value;
    }
private:
    friend struct NonZeroHiddenNullTrait<T>;    

    [[nodiscard]] explicit constexpr NonZero() noexcept: _value(0) {}
    [[nodiscard]] explicit constexpr NonZero(T value) noexcept: _value(value) {}

    T _value;
};

template<std::integral auto Value>
[[nodiscard]] 
constexpr NonZero<decltype(Value)> makeNonZero() noexcept
{
    return NonZero<decltype(Value)>::template make<Value>();
}

template<std::integral T>
[[nodiscard]] 
constexpr RE<NonZero<T>, SimpleError> makeNonZero(T value) noexcept
{
    return NonZero<T>::make(value);
}

template<std::integral T>
[[nodiscard]] 
constexpr NonZero<T> makeNonZero_Unchecked(T value) noexcept
{
    return NonZero<T>::make_Unchecked(value);
}

template<std::integral T>
struct NonZeroHiddenNullTrait
{
    [[nodiscard]] 
    static constexpr bool isNull(NonZero<T> v) noexcept
    {
        return v != 0;
    }

    [[nodiscard]] 
    static constexpr NonZero<T> null() noexcept
    {
        return NonZero<T>();
    }

    [[nodiscard]] 
    static constexpr NonZero<T> assignNull(NonZero<T> &v) noexcept
    {
        v = null();
    }
};

template<class T>
NonZeroHiddenNullTrait<T> plHiddenNullable(NonZero<T> const &);
} // export namespace pl
