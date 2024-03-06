module;
#include <cstddef>
#include <limits>

export module pl.core:tags;

export namespace pl::tags
{
struct in_place_t   {} constexpr in_place;
struct error_t      {} constexpr error;
struct none_t       {} constexpr none;
struct nullopt_t    {} constexpr nullopt;
struct impl_t       {} constexpr impl;
struct value_init_t {} constexpr value_init;

constexpr std::size_t dynamic_extent = (std::size_t) -1;
} // export namespace pl::tags
