module;
#include <concepts>
#include <type_traits>

export module pl.core:singleton;

import :traits;

namespace pl::tags
{
struct singleton_t {} constexpr singleton;
}

export namespace pl
{
class Singleton
{
public:
    constexpr Singleton(tags::singleton_t) noexcept {}
    Singleton(Singleton const &) = delete;
    Singleton &operator=(Singleton const &) = delete;
};
}

namespace pl
{
template<std::derived_from<Singleton> T>
class SingletonImpl : public T
{
public:
    constexpr SingletonImpl(): T(tags::singleton) {}
};
}

export namespace pl
{
template<std::derived_from<Singleton> T>
[[nodiscard]] constexpr T const &getSingleton()
{
    if constexpr (traits::is_constexpr_constructible_v<SingletonImpl<T>>)
    {
        static constexpr SingletonImpl<T> singleton;
        return singleton;
    }
    else
    {
        static SingletonImpl<T> const singleton;
        return singleton;
    }
}

template<std::derived_from<Singleton> T>
[[nodiscard]] T &getSingletonMut()
{
    static SingletonImpl<T> singleton;
    return singleton;
}
} // export namespace pl
