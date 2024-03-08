module;
#include <cassert>
#include <concepts>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

export module pl.core:result_error;

export import :tags;

import :traits;

export namespace pl
{
template<class Error>
class ErrorResult;
} // export namespace pl

namespace pl
{
template<class T>
struct is_error_result_specialization : std::false_type {};

template<class Error>
struct is_error_result_specialization<ErrorResult<Error>> : std::true_type {};

template<class T>
constexpr bool is_error_result_specialization_v = is_error_result_specialization<T>::value;
} // namespace pl

export namespace pl
{
template<class Error>
class [[nodiscard]] ErrorResult
{
public:
    [[nodiscard]] 
    explicit(!traits::is_implicitly_default_constructible_v<Error> && !std::is_void_v<Error>)
    constexpr ErrorResult()
    noexcept(traits::is_nothrow_default_constructible_or_void_v<Error>)
    requires traits::is_default_constructible_or_void_v<Error>
    : _error() {}

    template<class T = Error>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<T, Error>)
    constexpr ErrorResult(T &&error)
    noexcept(std::is_nothrow_constructible_v<Error, T>)
    requires(
        std::is_constructible_v<Error, T>
     && !std::same_as<ErrorResult, std::remove_cvref_t<T>>)
    : _error(std::forward<T>(error)) {}

    template<class ...Args>
    [[nodiscard]] 
    explicit(!traits::is_implicitly_constructible_v<Error, Args...>)
    constexpr ErrorResult(tags::in_place_t, Args &&...args)
    noexcept(std::is_nothrow_constructible_v<Error, Args...>)
    requires std::is_constructible_v<Error, Args...>
    : _error(std::forward<Args>(args)...) {}

    template<class T, class ...Args>
    [[nodiscard]] 
    explicit(!traits::is_implicitly_constructible_v<Error, std::initializer_list<T>, Args...>)
    constexpr ErrorResult(tags::in_place_t, std::initializer_list<T> ilist, Args &&...args)
    noexcept(std::is_nothrow_constructible_v<Error, std::initializer_list<T>, Args...>)
    requires std::is_constructible_v        <Error, std::initializer_list<T>, Args...>
    : _error(std::move(ilist), std::forward<Args>(args)...) {}

    [[nodiscard]] 
    explicit(!std::is_convertible_v<std::add_lvalue_reference_t<Error const>, Error>)
    ErrorResult(ErrorResult const &) = default;

    [[nodiscard]] 
    explicit(!std::is_convertible_v<Error, Error>)
    ErrorResult(ErrorResult &&) = default;

    template<class Error2>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<std::add_lvalue_reference_t<Error2 const>, Error>)
    constexpr ErrorResult(ErrorResult<Error2> const &other)
    noexcept(traits::is_nothrow_constructible_or_void_v<Error, std::add_lvalue_reference_t<Error2 const>>)
    requires(
        traits::is_constructible_or_void_v<Error, std::add_lvalue_reference_t<Error2 const>>
     && !traits::is_convertible_from_wrapper_v<Error, ErrorResult<Error2>>)
    : _error(other.error()) {}

    template<class Error2>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<Error2, Error>)
    constexpr ErrorResult(ErrorResult<Error2> &&other)
    noexcept(traits::is_nothrow_constructible_or_void_v<Error, Error2>)
    requires(
        traits::is_constructible_or_void_v<Error, Error2>
     && !traits::is_convertible_from_wrapper_v<Error, ErrorResult<Error2>>)
    : _error(std::move(other).error()) {}

    template<class T = Error>
    constexpr ErrorResult &operator=(T &&error)
    noexcept(std::is_nothrow_assignable_v<std::add_lvalue_reference_t<Error>, T>)
    requires(
        std::is_assignable_v<std::add_lvalue_reference_t<Error>, T>
     && !std::is_same_v<ErrorResult, std::remove_cvref_t<T>>)
    {
        _error = std::forward<T>(error);
        return *this;
    }

    ErrorResult &operator=(ErrorResult const  &) = default;
    ErrorResult &operator=(ErrorResult       &&) = default;

    template<class Error2>
    constexpr ErrorResult &operator=(ErrorResult<Error2> const &other)
    noexcept(
        traits::is_nothrow_assignable_or_void_v<
            std::add_lvalue_reference_t<Error>,
            std::add_lvalue_reference_t<Error2 const>>)
    requires(
        traits::is_assignable_or_void_v<
            std::add_lvalue_reference_t<Error>, 
            std::add_lvalue_reference_t<Error2 const>>
     && !traits::is_assignable_from_wrapper_v<Error, ErrorResult<Error2>>)
    {
        if constexpr (!std::is_void_v<Error>)
            _error = other.error();
        return *this;
    }

    template<class Error2>
    constexpr ErrorResult &operator=(ErrorResult<Error2> &&other)
    noexcept(
        traits::is_nothrow_assignable_or_void_v<
            std::add_lvalue_reference_t<Error>,
            Error2>)
    requires(
        traits::is_assignable_or_void_v<
            std::add_lvalue_reference_t<Error>, 
            Error2>
     && !traits::is_assignable_from_wrapper_v<Error, ErrorResult<Error2>>)
    {
        if constexpr (!std::is_void_v<Error>)
            _error = std::move(other).error();
        return *this;
    }

    [[nodiscard]] constexpr std::add_lvalue_reference_t<Error> error() & noexcept
    requires (!std::is_void_v<Error>)
    {
        return _error;
    }

    [[nodiscard]] constexpr std::add_lvalue_reference_t<Error const> error() const & noexcept
    requires (!std::is_void_v<Error>)
    {
        return _error;
    }

    [[nodiscard]] constexpr std::add_rvalue_reference_t<Error> error() && noexcept
    requires (!std::is_void_v<Error>)
    {
        return std::move(_error);
    }

    [[nodiscard]] constexpr std::add_rvalue_reference_t<Error const> error() const && noexcept
    requires (!std::is_void_v<Error>)
    {
        return std::move(_error);
    }

    constexpr void error() const noexcept
    requires std::is_void_v<Error>
    {
        return;
    }

private:
    traits::none_if_void_t<Error> _error;
};

template<class E>
ErrorResult(E) -> ErrorResult<E>;

template<class Result, class Error>
class [[nodiscard]] ResultError
{
public:
    [[nodiscard]]
    explicit(!traits::is_implicitly_default_constructible_v<Result> && !std::is_void_v<Result>)
    constexpr ResultError()
    noexcept(traits::is_nothrow_default_constructible_or_void_v<Result>)
    requires(traits::is_default_constructible_or_void_v<Result>)
    : _u{.result{}}, _hasValue(true) {}

    template<class T = Result>
    [[nodiscard]]
    explicit(!std::is_convertible_v<T, Result>)
    constexpr ResultError(T &&result)
    noexcept(std::is_nothrow_constructible_v<Result, T>)
    requires(
        std::is_constructible_v<Result, T>
     && !traits::match_any_v<std::remove_cvref_t<T>, tags::in_place_t, tags::error_t, ResultError>
     && !is_error_result_specialization_v<std::remove_cvref_t<T>>)
    : _u{.result{std::forward<T>(result)}}, _hasValue(true) {}

    template<class Error2>
    [[nodiscard]]
    explicit(!std::is_convertible_v<std::add_lvalue_reference_t<Error2 const>, Error>)
    constexpr ResultError(ErrorResult<Error2> const &e)
    noexcept(
        traits::is_nothrow_constructible_or_void_v<
            Error,
            std::add_lvalue_reference_t<Error2 const>>)
    requires(
        traits::is_constructible_or_void_v<
            Error,
            std::add_lvalue_reference_t<Error2 const>>)
    : _u{.none{}}, _hasValue(false)
    {
        if constexpr (std::is_void_v<Error>)
            std::construct_at(&_u.error);
        else
            std::construct_at(&_u.error, e.error());
    }

    template<class Error2>
    [[nodiscard]]
    explicit(!std::is_convertible_v<Error2, Error>)
    constexpr ResultError(ErrorResult<Error2> &&e)
    noexcept(
        traits::is_nothrow_constructible_or_void_v<
            Error,
            Error2>)
    requires(
        traits::is_constructible_or_void_v<
            Error, 
            Error2>)
    : _u{.none{}}, _hasValue(false)
    {
        if constexpr (std::is_void_v<Error>)
            std::construct_at(&_u.error);
        else
            std::construct_at(&_u.error, std::move(e).error());
    }

    template<class ...Args>
    [[nodiscard]]
    explicit(!traits::is_implicitly_constructible_v<Result, Args...>)
    constexpr ResultError(tags::in_place_t, Args &&...args)
    noexcept(std::is_nothrow_constructible_v<Result, Args...>)
    requires std::is_constructible_v<Result, Args...>
    : _u{.result{std::forward<Args>(args)...}}, _hasValue(true) {}

    template<class T, class ...Args>
    [[nodiscard]]
    explicit(!traits::is_implicitly_constructible_v<Result, std::initializer_list<T>, Args...>)
    constexpr ResultError(tags::in_place_t, std::initializer_list<T> ilist, Args &&...args)
    noexcept(std::is_nothrow_constructible_v<Result, std::initializer_list<T>, Args...>)
    requires std::is_constructible_v        <Result, std::initializer_list<T>, Args...>
    : _u{.result{std::move(ilist), std::forward<Args>(args)...}}, _hasValue(true) {}

    template<class ...Args>
    [[nodiscard]]
    explicit(!traits::is_implicitly_constructible_v<Error, Args...> && !std::is_void_v<Error>)
    constexpr ResultError(tags::error_t, Args &&...args)
    noexcept(std::is_nothrow_constructible_v<Error, Args...> || std::is_void_v<Error>)
    requires(
        std::is_constructible_v<Error, Args...>
     || (std::is_void_v<Error> && sizeof...(Args) == 0))
    : _u{.error{std::forward<Args>(args)...}}, _hasValue(false) {}

    template<class T, class ...Args>
    [[nodiscard]]
    explicit(!traits::is_implicitly_constructible_v<Error, std::initializer_list<T>, Args...>)
    constexpr ResultError(tags::error_t, std::initializer_list<T> ilist, Args &&...args)
    noexcept(std::is_nothrow_constructible_v<Error, std::initializer_list<T>, Args...>)
    requires std::is_constructible_v<Error, std::initializer_list<T>, Args...>
    : _u{.error{std::move(ilist), std::forward<Args>(args)...}}, _hasValue(false) {}

    [[nodiscard]]
    explicit(
        !std::is_convertible_v<std::add_lvalue_reference_t<Result const>, Result>
     || !std::is_convertible_v<std::add_lvalue_reference_t<Error  const>, Error >)
    constexpr ResultError(ResultError const &other)
    noexcept(
        traits::is_nothrow_copy_constructible_or_void_v<Result>
     && traits::is_nothrow_copy_constructible_or_void_v<Error >)
    requires(
        traits::is_copy_constructible_or_void_v<Result>
     && traits::is_copy_constructible_or_void_v<Error >)
    : _u{.none{}}, _hasValue(other.has_value())
    {
        if (other)
        {
            if constexpr (std::is_void_v<Result>)
                std::construct_at(&_u.result);
            else
                std::construct_at(&_u.result, *other);
        }
        else
        {
            if constexpr (std::is_void_v<Error>)
                std::construct_at(&_u.error);
            else
                std::construct_at(&_u.error, other.error());
        }
    }

    [[nodiscard]]
    explicit(
        !std::is_convertible_v<Result, Result>
     || !std::is_convertible_v<Error , Error >)
    constexpr ResultError(ResultError &&other)
    noexcept(
        traits::is_nothrow_move_constructible_or_void_v<Result>
     && traits::is_nothrow_move_constructible_or_void_v<Error>)
    requires(
        traits::is_move_constructible_or_void_v<Result>
     && traits::is_move_constructible_or_void_v<Error>)
    : _u{.none{}}, _hasValue(other.has_value())
    {
        if (other)
        {
            if constexpr (std::is_void_v<Result>)
                std::construct_at(&_u.result);
            else
                std::construct_at(&_u.result, *std::move(other));
        }
        else
        {
            if constexpr (std::is_void_v<Error>)
                std::construct_at(&_u.error);
            else
                std::construct_at(&_u.error, std::move(other).error());
        }
    }

    template<class Result2, class Error2>
    [[nodiscard]]
    explicit(
        !std::is_convertible_v<std::add_lvalue_reference_t<Result2 const>, Result>
     || !std::is_convertible_v<std::add_lvalue_reference_t<Error2 const>, Error>)
    constexpr ResultError(ResultError<Result2, Error2> const &other)
    noexcept(
        traits::is_nothrow_constructible_or_void_v<Result, std::add_lvalue_reference_t<Result2 const>>
     && traits::is_nothrow_constructible_or_void_v<Error, std::add_lvalue_reference_t<Error2 const>>)
    requires(
        traits::is_constructible_or_void_v<Result, std::add_lvalue_reference_t<Result2 const>>
     && traits::is_constructible_or_void_v<Error, std::add_lvalue_reference_t<Error2 const>>
     && !traits::is_convertible_from_wrapper_v<Result, ResultError<Result2, Error2>>)
    : _u{.none{}}, _hasValue(other.has_value())
    {
        if (other)
        {
            if constexpr (std::is_void_v<Result>)
                std::construct_at(&_u.result);
            else
                std::construct_at(&_u.result, *other);
        }
        else
        {
            if constexpr (std::is_void_v<Error>)
                std::construct_at(&_u.error);
            else
                std::construct_at(&_u.error, other.error());
        }
    }

    template<class Result2, class Error2>
    [[nodiscard]]
    explicit(
        !std::is_convertible_v<Result2, Result>
     && !std::is_convertible_v<Error2, Error>)
    constexpr ResultError(ResultError<Result2, Error2> &&other)
    noexcept(
        traits::is_nothrow_constructible_or_void_v<Result, Result2>
     && traits::is_nothrow_constructible_or_void_v<Error, Error2>)
    requires(
        traits::is_constructible_or_void_v<Result, Result2>
     && traits::is_constructible_or_void_v<Error, Error2>
     && !traits::is_convertible_from_wrapper_v<Result, ResultError<Result2, Error2>>)
    : _u{.none{}}, _hasValue(other.has_value())
    {
        if (other)
        {
            if constexpr (std::is_void_v<Result>)
                std::construct_at(&_u.result);
            else
                std::construct_at(&_u.result, *std::move(other));
        }
        else
        {
            if constexpr (std::is_void_v<Error>)
                std::construct_at(&_u.error);
            else
                std::construct_at(&_u.error, std::move(other).error());
        }
    }

    template<class T = Result>
    constexpr ResultError &operator=(T &&result)
    noexcept(
        std::is_nothrow_constructible_v<                            Result , T>
     && std::is_nothrow_assignable_v   <std::add_lvalue_reference_t<Result>, T>)
    requires(
        std::is_constructible_v<                            Result , T>
     && std::is_assignable_v   <std::add_lvalue_reference_t<Result>, T>
     && !std::is_same_v<ResultError, std::remove_cvref_t<T>>
     && is_error_result_specialization_v<std::remove_cvref_t<T>>)
    {
        if (_hasValue)
            _u.result = std::forward<T>(result);
        else
        {
            std::destroy_at(&_u.error);
            std::construct_at(&_u.result, std::forward<T>(result));
            _hasValue = true;
        }
        return *this;
    }

    template<class Error2>
    constexpr ResultError &operator=(ErrorResult<Error2> const &e)
    noexcept(
        traits::is_nothrow_constructible_or_void_v<Error, std::add_lvalue_reference_t<Error2 const>>
     && traits::is_nothrow_assignable_or_void_v<
            std::add_lvalue_reference_t<Error>, 
            std::add_lvalue_reference_t<Error2 const>>)
    requires(
        traits::is_constructible_or_void_v<Error, std::add_lvalue_reference_t<Error2 const>>
     && traits::is_assignable_or_void_v<
            std::add_lvalue_reference_t<Error>, 
            std::add_lvalue_reference_t<Error2 const>>)
    {
        if (_hasValue)
        {
            std::destroy_at(&_u.result);
            if constexpr (std::is_void_v<Error>)
                std::construct_at(&_u.error);
            else
                std::construct_at(&_u.error, e.error());
            _hasValue = false;
        }
        else
        {
            if constexpr (!std::is_void_v<Error>)
                _u.error = e.error();
        }
        return *this;
    }

    template<class Error2>
    constexpr ResultError &operator=(ErrorResult<Error2> &&e)
    noexcept(
        traits::is_nothrow_constructible_or_void_v<Error, Error2>
     && traits::is_nothrow_assignable_or_void_v<
            std::add_lvalue_reference_t<Error>, 
            Error2>)
    requires(
        traits::is_constructible_or_void_v<Error, Error2>
     && traits::is_assignable_or_void_v<
            std::add_lvalue_reference_t<Error>, 
            Error2>)
    {
        if (_hasValue)
        {
            std::destroy_at(&_u.result);
            if constexpr (std::is_void_v<Error>)
                std::construct_at(&_u.error);
            else
                std::construct_at(&_u.error, std::move(e).error());
            _hasValue = false;
        }
        else
        {
            if constexpr (!std::is_void_v<Error>)
                _u.error = std::move(e).error();
        }
        return *this;
    }

    constexpr ResultError &operator=(ResultError const &other)
    noexcept(
        traits::is_nothrow_copy_constructible_or_void_v<Result>
     && traits::is_nothrow_copy_assignable_or_void_v   <Result>
     && traits::is_nothrow_copy_constructible_or_void_v<Error >
     && traits::is_nothrow_copy_assignable_or_void_v   <Error >)
    requires(
        traits::is_copy_constructible_or_void_v<Result>
     && traits::is_copy_assignable_or_void_v   <Result>
     && traits::is_copy_constructible_or_void_v<Error >
     && traits::is_copy_assignable_or_void_v   <Error >)
    {
        if (_hasValue)
        {
            if (other)
            {
                if constexpr (!std::is_void_v<Result>)
                    _u.result = *other;
            }
            else
            {
                std::destroy_at(&_u.result);
                if constexpr (std::is_void_v<Error>)
                    std::construct_at(&_u.error);
                else
                    std::construct_at(&_u.error, other.error());
            }
        }
        else
        {
            if (other)
            {
                std::destroy_at(&_u.error);
                if constexpr (std::is_void_v<Result>)
                    std::construct_at(&_u.result);
                else
                    std::construct_at(&_u.result, *other);
            }
            else
            {
                if constexpr (!std::is_void_v<Error>)
                    _u.error = other.error();
            }
        }
        _hasValue = other.has_value();
        return *this;
    }

    constexpr ResultError &operator=(ResultError &&other)
    noexcept(
        traits::is_nothrow_move_constructible_or_void_v<Result>
     && traits::is_nothrow_move_constructible_or_void_v<Error >
     && traits::is_nothrow_move_assignable_or_void_v   <Result>
     && traits::is_nothrow_move_assignable_or_void_v   <Error >)
    requires(
        traits::is_move_constructible_or_void_v<Result>
     && traits::is_move_constructible_or_void_v<Error >
     && traits::is_move_assignable_or_void_v   <Result>
     && traits::is_move_assignable_or_void_v   <Error >)
    {
        if (_hasValue)
        {
            if (other)
            {
                if constexpr (!std::is_void_v<Result>)
                    _u.result = *std::move(other);
            }
            else
            {
                std::destroy_at(&_u.result);
                if constexpr (std::is_void_v<Error>)
                    std::construct_at(&_u.error);
                else
                    std::construct_at(&_u.error, std::move(other).error());
            }
        }
        else
        {
            if (other)
            {
                std::destroy_at(&_u.error);
                if constexpr (std::is_void_v<Result>)
                    std::construct_at(&_u.result);
                else
                    std::construct_at(&_u.result, *std::move(other));
            }
            else
            {
                if constexpr (!std::is_void_v<Error>)
                    _u.error = std::move(other).error();
            }
        }
        _hasValue = other.has_value();
        return *this;
    }

    template<class Result2, class Error2>
    constexpr ResultError &operator=(ResultError<Result2, Error2> const &other)
    noexcept(
        traits::is_nothrow_constructible_or_void_v<Result, std::add_lvalue_reference_t<Result2 const>>
     && traits::is_nothrow_constructible_or_void_v<Error, std::add_lvalue_reference_t<Error2  const>>
     && traits::is_nothrow_assignable_or_void_v<
            std::add_lvalue_reference_t<Result>,
            std::add_lvalue_reference_t<Result2 const>>
     && traits::is_nothrow_assignable_or_void_v<
            std::add_lvalue_reference_t<Error>,
            std::add_lvalue_reference_t<Error2 const>>)
    requires(
        traits::is_constructible_or_void_v<Result, std::add_lvalue_reference_t<Result2 const>>
     && traits::is_constructible_or_void_v<Error, std::add_lvalue_reference_t<Error2  const>>
     && traits::is_assignable_or_void_v<
            std::add_lvalue_reference_t<Result>,
            std::add_lvalue_reference_t<Result2 const>>
     && traits::is_assignable_or_void_v<
            std::add_lvalue_reference_t<Error>,
            std::add_lvalue_reference_t<Error2  const>>
     && !traits::is_convertible_from_wrapper_v<Result, ResultError<Result2, Error2>>
     && !traits::is_assignable_from_wrapper_v <Result, ResultError<Result2, Error2>>)
    {
        if (_hasValue)
        {
            if (other)
            {
                if constexpr (!std::is_void_v<Result>)
                    _u.result = *other;
            }
            else
            {
                std::destroy_at(&_u.result);
                if constexpr (std::is_void_v<Error>)
                    std::construct_at(&_u.error);
                else
                    std::construct_at(&_u.error, other.error());
            }
        }
        else
        {
            if (other)
            {
                std::destroy_at(&_u.error);
                if constexpr (std::is_void_v<Result>)
                    std::construct_at(&_u.result);
                else
                    std::construct_at(&_u.result, *other);
            }
            else
            {
                if constexpr (!std::is_void_v<Error>)
                    _u.error = other.error();
            }
        }
        _hasValue = other.has_value();
        return *this;
    }

    template<class Result2, class Error2>
    constexpr ResultError &operator=(ResultError<Result2, Error2> &&other)
    noexcept(
        traits::is_nothrow_constructible_or_void_v<Result, Result2>
     && traits::is_nothrow_constructible_or_void_v<Error, Error2 >
     && traits::is_nothrow_assignable_or_void_v<
            std::add_lvalue_reference_t<Result>,
            Result2>
     && traits::is_nothrow_assignable_or_void_v<
            std::add_lvalue_reference_t<Error>,
            Error2>)
    requires(
        traits::is_constructible_or_void_v<Result  , Result2>
     && traits::is_constructible_or_void_v<Error   , Error2 >
     && traits::is_assignable_or_void_v<
            std::add_lvalue_reference_t<Result>,
            Result2>
     && traits::is_assignable_or_void_v<
            std::add_lvalue_reference_t<Error>,
            Error2 >
     && !traits::is_convertible_from_wrapper_v<Result, ResultError<Result2, Error2>>
     && !traits::is_assignable_from_wrapper_v <Result, ResultError<Result2, Error2>>)
    {
        if (_hasValue)
        {
            if (other)
            {
                if constexpr (!std::is_void_v<Result>)
                    _u.result = *std::move(other);
            }
            else
            {
                std::destroy_at(&_u.result);
                if constexpr (std::is_void_v<Error>)
                    std::construct_at(&_u.error);
                else
                    std::construct_at(&_u.error, std::move(other).error());
            }
        }
        else
        {
            if (other)
            {
                std::destroy_at(&_u.error);
                if constexpr (std::is_void_v<Result>)
                    std::construct_at(&_u.result);
                else
                    std::construct_at(&_u.result, *std::move(other));
            }
            else
            {
                if constexpr (!std::is_void_v<Error>)
                    _u.error = std::move(other).error();
            }
        }
        return *this;
    }

    constexpr ~ResultError()
    {
        if (_hasValue)
            std::destroy_at(&_u.result);
        else
            std::destroy_at(&_u.error);
    }

    [[nodiscard]] constexpr std::add_lvalue_reference_t<Result> operator*() & noexcept
    requires (!std::is_void_v<Result>)
    {
        assert(_hasValue);
        return _u.result;
    }

    [[nodiscard]] constexpr std::add_lvalue_reference_t<Result const> operator*() const & noexcept
    requires (!std::is_void_v<Result>)
    {
        assert(_hasValue);
        return _u.result;
    }

    [[nodiscard]] constexpr std::add_rvalue_reference_t<Result> operator*() && noexcept
    requires (!std::is_void_v<Result>)
    {
        assert(_hasValue);
        return std::move(_u.result);
    }

    [[nodiscard]] constexpr std::add_rvalue_reference_t<Result const> operator*() const && noexcept
    requires (!std::is_void_v<Result>)
    {
        assert(_hasValue);
        return std::move(_u.result);
    }

    // TODO: replace with lvalue/rvalue pointer
    [[nodiscard]] constexpr Result *operator->() noexcept
    requires (!std::is_void_v<Result>)
    {
        assert(_hasValue);
        return &_u.result;
    }

    // TODO: replace with lvalue/rvalue pointer
    [[nodiscard]] constexpr Result const *operator->() const noexcept
    requires (!std::is_void_v<Result>)
    {
        assert(_hasValue);
        return &_u.result;
    }

    constexpr void operator*() const noexcept
    requires std::is_void_v<Result>
    {
        assert(_hasValue);
        return;
    }

    [[nodiscard]] constexpr std::add_lvalue_reference_t<Error> error() & noexcept
    requires (!std::is_void_v<Error>)
    {
        assert(!_hasValue);
        return _u.error;
    }

    [[nodiscard]] constexpr std::add_lvalue_reference_t<Error const> error() const & noexcept
    requires (!std::is_void_v<Error>)
    {
        assert(!_hasValue);
        return _u.error;
    }

    [[nodiscard]] constexpr std::add_rvalue_reference_t<Error> error() && noexcept
    requires (!std::is_void_v<Error>)
    {
        assert(!_hasValue);
        return std::move(_u.error);
    }

    [[nodiscard]] constexpr std::add_rvalue_reference_t<Error const> error() const && noexcept
    requires (!std::is_void_v<Error>)
    {
        assert(!_hasValue);
        return std::move(_u.error);
    }

    constexpr void error() const noexcept
    requires std::is_void_v<Error>
    {
        assert(!_hasValue);
        return;
    }

    [[nodiscard]] explicit constexpr operator bool() const noexcept { return _hasValue; }

    [[nodiscard]] constexpr bool has_value() const noexcept { return _hasValue; }

private:
    union U
    {
        [[no_unique_address]] tags::none_t                   none;
        [[no_unique_address]] traits::none_if_void_t<Result> result;
        [[no_unique_address]] traits::none_if_void_t<Error>  error;
        constexpr ~U() {}
    };

    [[no_unique_address]] U _u;
    bool                    _hasValue;
};

template<class Result, class Error>
using RE = ResultError<Result, Error>;
} // export namespace pl
