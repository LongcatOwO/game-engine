module;
#include <type_traits>
#include <utility>

export module pl.core:handle;

import :null;
import :optional;

export namespace pl
{
template<class T, class Deleted>
concept deleter = requires (T del, Deleted obj)
{
    { del(obj) };
};

// TODO
template<class T, deleter<T> D>
class UHandle
{
public:
    [[nodiscard]]
    explicit constexpr UHandle(T &&handle, D &&deleter = D())
    noexcept(
        std::is_nothrow_move_constructible_v<T>
     && std::is_nothrow_move_constructible_v<D>)
    requires(
        std::move_constructible<T>
     && std::move_constructible<D>)
    :
        _handle(std::move(handle)),
        _deleter(std::move(deleter))
    {}

    // TODO
    template<class U, class E>
    [[nodiscard]]
    explicit constexpr UHandle(U &&u, E &&e);

    [[nodiscard]]
    explicit(
        !std::is_convertible_v<T, T>
     || !std::is_convertible_v<D, D>)
    constexpr UHandle(UHandle &&other)
    noexcept(
        std::is_nothrow_move_constructible_v<T>
     && std::is_nothrow_move_constructible_v<D>)
    requires(
        std::is_move_constructible_v<T>
     && std::is_move_constructible_v<D>)
    : 
        _handle(std::exchange(other._handle, {})),
        _deleter(std::move(other._deleter))
    {}

    template<class U, class E>
    [[nodiscard]]
    explicit(
        !std::is_convertible_v<U, T>
     || !std::is_convertible_v<E, D>)
    constexpr UHandle(UHandle<U, E> &&other);

    constexpr UHandle &operator=(UHandle &&other)
    noexcept(
        std::is_nothrow_move_constructible_v<T>
     && std::is_nothrow_move_assignable_v<T>
     && std::is_nothrow_move_assignable_v<D>)
    {
        if (_handle) _deleter(*_handle);

        _handle = std::exchange(other._handle, {});
        _deleter = std::move(other._deleter);
        return *this;
    }

    constexpr ~UHandle()
    {
        if (_handle) _deleter(*_handle);
    }

    [[nodiscard]] constexpr operator T const &() const & noexcept
    {
        return *_handle;
    }

    [[nodiscard]] constexpr operator T const &&() const && noexcept
    {
        return *std::move(_handle);
    }
private:
    template<class U, deleter<U> E>
    friend class UHandle;

    Opt<T>                  _handle;
    [[no_unique_address]] D _deleter;
};
} // export namespace pl
