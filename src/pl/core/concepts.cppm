module;
#include <concepts>
#include <type_traits>

export module pl.core:concepts;

export namespace pl
{
template<class T>
concept static_sized = []
{
    using U = std::remove_reference_t<T>;
    return requires { typename std::integral_constant<decltype(U::size()), U::size()>; };
}();

template<static_sized T>
consteval auto static_size() noexcept
{
    return std::remove_reference_t<T>::size();
}

template<class T, auto Size>
concept static_size_of = []
{
    if constexpr (static_sized<T>)
    {
        return static_size<T>() == Size;
    }
    else
        return false;
}();
} // export namespace pl
