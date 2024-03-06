module;
#include <type_traits>
#include <utility>

export module pl.core:traits;

export import :tags;

export namespace pl::traits
{
template<class T, class ...Args>
constexpr bool is_implicitly_constructible_v = 
    requires { [](Args &&...args) -> T { return {std::forward<Args>(args)...}; }; }
 && !std::is_void_v<T>;

template<class T>
constexpr bool is_implicitly_default_constructible_v = is_implicitly_constructible_v<T>;

template<class T>
constexpr bool is_implicitly_copy_constructible_v = is_implicitly_constructible_v<T, T const&>;

template<class T>
constexpr bool is_implicitly_move_constructible_v = is_implicitly_constructible_v<T, T>;

template<class T, class ...Args>
constexpr bool is_constexpr_constructible_v = 
    requires { [](Args &&...args) { constexpr T t{std::forward<Args>(args)...}; }; };

template<class T, class Wrapper>
constexpr bool is_convertible_from_wrapper_v =  
    std::is_convertible_v<Wrapper &, T>
 || std::is_convertible_v<Wrapper, T>
 || std::is_convertible_v<Wrapper const &, T>
 || std::is_convertible_v<Wrapper const, T>
 || std::is_constructible_v<T, Wrapper &>
 || std::is_constructible_v<T, Wrapper>
 || std::is_constructible_v<T, Wrapper const &>
 || std::is_constructible_v<T, Wrapper const>;

template<class T, class Wrapper>
constexpr bool is_assignable_from_wrapper_v =
    std::is_assignable_v<T &, Wrapper &>
 || std::is_assignable_v<T &, Wrapper>
 || std::is_assignable_v<T &, Wrapper const &>
 || std::is_assignable_v<T &, Wrapper const>;

template<class T, class U>
using unless_none_t = std::conditional_t<
        std::is_same_v<tags::none_t, std::remove_cvref_t<T>>,
        U,
        T>;

template<class T>
using none_if_void_t = std::conditional_t<std::is_void_v<T>, tags::none_t, T>;

template<class T>
constexpr bool is_unqualified_v = std::is_same_v<std::remove_cvref_t<T>, T>;

template<class T, class ...Args>
constexpr bool match_any_v = (std::is_same_v<T, Args> || ...);

template<class T, class U>
constexpr bool is_qualification_convertible_v = std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;


template<class T, class U>
constexpr bool is_constructible_or_void_v =
    (std::is_void_v<T> && std::is_void_v<U>)
 || std::is_constructible_v<T, U>;

template<class T>
constexpr bool is_default_constructible_or_void_v =
    std::is_void_v<T> || std::is_default_constructible_v<T>;

template<class T>
constexpr bool is_copy_constructible_or_void_v =
    std::is_void_v<T> || std::is_copy_constructible_v<T>;

template<class T>
constexpr bool is_move_constructible_or_void_v =
    std::is_void_v<T> || std::is_move_constructible_v<T>;

template<class T, class U>
constexpr bool is_assignable_or_void_v =
    (std::is_void_v<T> && std::is_void_v<U>)
 || std::is_assignable_v<T, U>;

template<class T>
constexpr bool is_copy_assignable_or_void_v =
    std::is_void_v<T> || std::is_copy_assignable_v<T>;

template<class T>
constexpr bool is_move_assignable_or_void_v =
    std::is_void_v<T> || std::is_move_assignable_v<T>;


template<class T, class U>
constexpr bool is_nothrow_constructible_or_void_v =
    (std::is_void_v<T> && std::is_void_v<U>)
 || std::is_nothrow_constructible_v<T, U>;

template<class T>
constexpr bool is_nothrow_default_constructible_or_void_v =
    std::is_void_v<T> || std::is_nothrow_default_constructible_v<T>;

template<class T>
constexpr bool is_nothrow_copy_constructible_or_void_v =
    std::is_void_v<T> || std::is_nothrow_copy_constructible_v<T>;

template<class T>
constexpr bool is_nothrow_move_constructible_or_void_v =
    std::is_void_v<T> || std::is_nothrow_move_constructible_v<T>;

template<class T, class U>
constexpr bool is_nothrow_assignable_or_void_v =
    (std::is_void_v<T> && std::is_void_v<U>)
 || std::is_nothrow_assignable_v<T, U>;

template<class T>
constexpr bool is_nothrow_copy_assignable_or_void_v =
    std::is_void_v<T> || std::is_nothrow_copy_assignable_v<T>;

template<class T>
constexpr bool is_nothrow_move_assignable_or_void_v =
    std::is_void_v<T> || std::is_nothrow_move_assignable_v<T>;

template<class T, template<class ...> class Tmpl>
struct is_specialization_of : std::false_type {};

template<template<class ...> class Tmpl, class ...Args>
struct is_specialization_of<Tmpl<Args...>, Tmpl> : std::true_type {};

template<class T, template<class ...> class Tmpl>
constexpr bool is_specialization_of_v = is_specialization_of<T, Tmpl>::value;

template<class T, template<class ...> class Tmpl>
concept specialization_of = is_specialization_of_v<T, Tmpl>;
} // export namespace pl::traits
