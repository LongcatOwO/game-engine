module;
#include <concepts>
#include <initializer_list>
#include <source_location>
#include <type_traits>
#include <utility>

export module pl.core:error;

export import :tags;
export import :singleton;

import :traits;

import :result_error;

export namespace pl
{
template<class ParentType>
class SuberrorType;

template<class T>
struct is_suberror_type : std::false_type {};

template<class T>
requires 
    requires (T *tp)
    { 
        []<class ParentType>(SuberrorType<ParentType> const *){}(tp);
    }
struct is_suberror_type<T> : std::true_type {};

template<class T>
constexpr bool is_suberror_type_v = is_suberror_type<T>::value;


class ErrorType : public Singleton
{
public:
    [[nodiscard]]         constexpr ErrorType const *parent()   const noexcept { return _parent; }
    [[nodiscard]] virtual constexpr char      const *name()     const noexcept = 0;

    template<class ParentType> requires is_suberror_type_v<ParentType>
    [[nodiscard]] constexpr bool isSubtypeOf() const noexcept;

private:
    template<class>
    friend class SuberrorType;

    [[nodiscard]] constexpr ErrorType(auto singleton_tag, ErrorType const *parent) noexcept : Singleton(singleton_tag), _parent(parent) {}

    ErrorType const *_parent;
};

template<class ParentType = void>
class SuberrorType;

template<class ParentType>
requires is_suberror_type_v<ParentType> || std::is_void_v<ParentType>
class SuberrorType<ParentType> : public ErrorType
{
public:
    [[nodiscard]] constexpr SuberrorType(auto singleton_tag) noexcept
    :
        ErrorType(
            singleton_tag,
            [] -> ErrorType const *
            {
                if constexpr (std::is_void_v<ParentType>)
                    return nullptr;
                else
                    return &getSingleton<ParentType>();
            }())
    {}
};

template<class ParentType> requires is_suberror_type_v<ParentType>
[[nodiscard]] constexpr bool ErrorType::isSubtypeOf() const noexcept
{
    ErrorType const *parent = &getSingleton<ParentType>();
    ErrorType const *current = this;
    for (; current && current != parent; current = current->parent());
    return current != nullptr;
}

template<class T>
concept error_info = requires (T const info) {
    { info.errorType() } noexcept -> std::same_as<ErrorType const &>;
};

template <error_info ErrorInfo>
class [[nodiscard]] Error
{
public:
    [[nodiscard]] 
    explicit(!traits::is_implicitly_default_constructible_v<ErrorInfo>)
    Error()
    noexcept(std::is_nothrow_default_constructible_v<ErrorInfo>)
    requires std::is_default_constructible_v<ErrorInfo>
    : _info() {}

    template<class T = ErrorInfo>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<T, ErrorInfo>)
    constexpr Error(T &&info) noexcept
    requires(
        std::is_constructible_v<ErrorInfo, T>
     && !std::is_same_v<std::remove_cvref_t<T>, tags::in_place_t>
     && !std::is_same_v<std::remove_cvref_t<T>, Error>)
    : _info(std::forward<T>(info)) {}

    template<class ...Args>
    [[nodiscard]] 
    explicit(!traits::is_implicitly_constructible_v<ErrorInfo, Args...>)
    constexpr Error(tags::in_place_t, Args &&...args)
    noexcept(std::is_nothrow_constructible_v<ErrorInfo, Args...>)
    requires std::is_constructible_v<ErrorInfo, Args...>
    : _info(std::forward<Args>(args)...) {}

    template<class T, class ...Args>
    [[nodiscard]] 
    explicit(!traits::is_implicitly_constructible_v<ErrorInfo, std::initializer_list<T>, Args...>)
    constexpr Error(tags::in_place_t, std::initializer_list<T> ilist, Args &&...args)
    noexcept(std::is_nothrow_constructible_v<ErrorInfo, std::initializer_list<T>, Args...>)
    requires std::is_constructible_v<ErrorInfo, std::initializer_list<T>, Args...>
    : _info(std::move(ilist), std::forward<Args>(args)...) {}

    [[nodiscard]] 
    explicit(!traits::is_implicitly_copy_constructible_v<ErrorInfo>)
    Error(Error const &)            = default;

    [[nodiscard]] 
    explicit(!traits::is_implicitly_move_constructible_v<ErrorInfo>)
    Error(Error &&)                 = default;

    template<class ErrorInfo2>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<ErrorInfo2 const &, ErrorInfo>)
    constexpr Error(Error<ErrorInfo2> const &other)
    noexcept(std::is_nothrow_constructible_v<ErrorInfo, ErrorInfo2 const &>)
    requires(
        std::is_constructible_v<ErrorInfo, ErrorInfo2 const &>
     && !traits::is_convertible_from_wrapper_v<ErrorInfo, Error<ErrorInfo2>>)
    : _info(other.info()) {}

    template<class ErrorInfo2>
    [[nodiscard]] 
    explicit(!std::is_convertible_v<ErrorInfo2, ErrorInfo>)
    constexpr Error(Error<ErrorInfo2> &&other)
    noexcept(std::is_nothrow_constructible_v<ErrorInfo, ErrorInfo2>)
    requires(
        std::is_constructible_v<ErrorInfo, ErrorInfo2>
     && !traits::is_convertible_from_wrapper_v<ErrorInfo, Error<ErrorInfo2>>)
    : _info(std::move(other).info()) {}


    template<class T = ErrorInfo>
    constexpr Error &operator=(T &&result)
    noexcept(std::is_nothrow_assignable_v<ErrorInfo &, T>)
    requires(
        std::is_assignable_v<ErrorInfo &, T>
     && !std::is_same_v<Error, std::remove_cvref_t<T>>)
    {
        _info = std::forward<T>(result);
        return *this;
    }

    Error &operator=(Error const &) = default;
    Error &operator=(Error &&)      = default;

    template<class ErrorInfo2>
    constexpr Error &operator=(Error<ErrorInfo2> const &other)
    noexcept(std::is_nothrow_assignable_v<ErrorInfo &, ErrorInfo2 const &>)
    requires(
        std::is_assignable_v<ErrorInfo &, ErrorInfo2 const &>
     && !traits::is_convertible_from_wrapper_v<ErrorInfo, Error<ErrorInfo2>>
     && !traits::is_assignable_from_wrapper_v<ErrorInfo, Error<ErrorInfo2>>)
    {
        _info = other.info();
        return *this;
    }

    template<class ErrorInfo2>
    constexpr Error &operator=(Error<ErrorInfo2> &&other)
    noexcept(std::is_nothrow_assignable_v<ErrorInfo &, ErrorInfo2>)
    requires(
        std::is_assignable_v<ErrorInfo &, ErrorInfo2>
     && !traits::is_convertible_from_wrapper_v<ErrorInfo, Error<ErrorInfo2>>
     && !traits::is_assignable_from_wrapper_v<ErrorInfo, Error<ErrorInfo2>>)
    {
        _info = std::move(other).info();
        return *this;
    }

    [[nodiscard]] constexpr ErrorInfo        &info()        & noexcept { return           _info;  }
    [[nodiscard]] constexpr ErrorInfo const  &info() const  & noexcept { return           _info;  }
    [[nodiscard]] constexpr ErrorInfo       &&info()       && noexcept { return std::move(_info); }
    [[nodiscard]] constexpr ErrorInfo const &&info() const && noexcept { return std::move(_info); }

    [[nodiscard]] constexpr ErrorType const &errorType() const noexcept { return _info.errorType(); }

private:
    ErrorInfo _info;
};

template<class T>
Error(T) -> Error<T>;

class SimpleErrorInfo
{
public:
    [[nodiscard]] constexpr SimpleErrorInfo(ErrorType const &type) noexcept : _type(&type) {}
    [[nodiscard]] constexpr ErrorType const &errorType() const noexcept { return *_type; }

private:
    ErrorType const *_type;
};

using SimpleError = Error<SimpleErrorInfo>;

// TODO: StacktracedErrorInfo

// class ErrorInfo {
// public:
//     constexpr ErrorInfo(
//         ErrorType const &errorType,
//         std::source_location &&srcLoc = std::source_location::current())
//     noexcept
//     :
//         _errorType(&errorType), _srcLoc(std::move(srcLoc))
//     {}
// 
//     ErrorInfo(ErrorInfo &&) = default;
//     ErrorInfo &operator=(ErrorInfo &&) = default;
// 
// private:
//     ErrorType const *_errorType;
//     std::source_location _srcLoc;
// };
} // export namespace pl

namespace pl
{
static_assert(error_info<SimpleErrorInfo>, "SimpleErrorInfo must satisfy error_info constraint");
}
