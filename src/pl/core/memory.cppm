module;
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include <pl/macro.hpp>

export module pl.core:memory;

import :error;
import :null;
import :numeric;
import :result_error;
import :span;
import :tags;
import :utility;


export namespace pl
{
// Begins the lifetime of an object of type T at location p.
// Essentially, this function "reinterprets" the data at p into object of type T.
// T MUST be an ImplicitLifetimeType, otherwise the behavior is undefined.
template<class T>
[[nodiscard]] Ptr<T> start_lifetime_as(Ptr<void> p) noexcept
{
    // Triggers implicit object creation, as std::memmove is blessed by the standard.
    // std::launder is used to force the implicit object creation to create object of type T.
    // Alternatively, new (p) std::byte[sizeof(T)] can be used to perform implicit object creation as well,
    // However, that tampers with the object representation of memory pointed to by p.
    // This method perserves that object representation, which can be useful for
    // "reinterpreting" the data pointed to by p safely without needing to copy the data somewhere else.
    // 
    // Reference:
    // https://stackoverflow.com/questions/76445860/implementation-of-stdstart-lifetime-as
    return makeNonNull_Unchecked(std::launder(static_cast<T*>(std::memmove(p, p, sizeof(T)))));
}

// Begins the lifetime of an object of type T[count] at location p.
// Unlike the previous overload, this function does not require T to be an ImplicitLifetimeType.
// If T is an ImplicitLifetimeType, their lifetime will be began.
// Otherwise, only the lifetime of the array will be began, NOT its elements.
// Useful for doing allocating space for vector and such.
template<class T>
[[nodiscard]] Span<T> start_lifetime_as_array(Ptr<void> p, std::size_t count) noexcept
{
    // Same idea as start_lifetime_as.
    // However, this works for array of dynamic size.
    // The implicit object creation of the array elements are forced by putting the resulting
    // laundered pointer into Span.
    // The fact that Span performs pointer arithmetic on the resulting pointer to get the end pointer
    // forces the implicit creation of the array elements.
    // This means that the resulting Span MUST be dynamic Span that actually stores the end pointer,
    // NOT fixed length Span. Otherwise, the behavior could be undefined.
    // 
    // For fixed length array, the next overload should be used instead.
    //
    // Reference:
    // https://stackoverflow.com/questions/66731097/how-to-create-an-array-and-start-its-lifetime-without-starting-the-lifetime-of-a
    return {std::launder(static_cast<T*>(std::memmove(p, p, sizeof(T) * count))), count};
}

template<class T, std::size_t N>
requires (N != tags::dynamic_extent)
[[nodiscard]] 
Span<T, N> start_lifetime_as_array(Ptr<void> p) noexcept
{
    return {*std::launder(static_cast<T(*)[N]>(std::memmove(p, p, sizeof(T) * N)))};
}


template<class T, class ...Args>
constexpr Ptr<T> construct_at(Ptr<T> p, Args &&...args)
noexcept(std::is_nothrow_constructible_v<T, Args...>)
requires std::is_constructible_v<T, Args...>
{
    return makeNonNull_Unchecked(std::construct_at<T>(p, std::forward<Args>(args)...));
}

template<class T>
constexpr Ptr<T> default_construct_at(Ptr<T> p)
noexcept(std::is_nothrow_default_constructible_v<T>)
requires std::is_default_constructible_v<T>
{
    if constexpr (std::is_trivially_default_constructible_v<T>)
        return makeNonNull_Unchecked(new (p) T);
    else
        return construct_at(p);
}

template<class T>
constexpr void destroy_at(Ptr<T> p)
noexcept(std::is_nothrow_destructible_v<T>)
requires std::is_destructible_v<T>
{
    std::destroy_at<T>(p);
}

[[nodiscard]] 
constexpr bool isValidAlignment(std::size_t alignment) noexcept
{
    return (alignment != 0) && ((alignment & (alignment - 1)) == 0);
}

template<class T>
concept allocator = 
    requires (
        T a,
        Ptr<void> memory,
        NonZero<std::size_t> size,
        NonZero<std::size_t> alignment)
    {
        { a.alloc(size, alignment) } noexcept -> std::same_as<RE<Ptr<void>, SimpleError>>;
        { a.free(memory, size, alignment) } noexcept;
    }
 && std::is_nothrow_copy_constructible_v<std::remove_reference_t<T>>;

class BadAlloc : public SuberrorType<>
{
public:
    using SuberrorType::SuberrorType;

    [[nodiscard]] 
    constexpr char const *name() const noexcept override
    {
        return "BadAlloc";
    }
};

class BadMalloc : public SuberrorType<BadAlloc>
{
public:
    using SuberrorType::SuberrorType;

    [[nodiscard]] 
    constexpr char const *name() const noexcept override
    {
        return "BadMalloc";
    }
};

class Mallocator
{
public:
    [[nodiscard]] 
    static RE<Ptr<void>, SimpleError> alloc(NonZero<std::size_t> size, NonZero<std::size_t> alignment) noexcept
    {
        assert(alignment <= alignof(std::max_align_t) && isValidAlignment(alignment));
        if (void *memory = std::malloc(size); memory)
            return makeNonNull_Unchecked(memory);
        else
            return {tags::error, getSingleton<BadMalloc>()};
    }

    static void free(Ptr<void> memory, NonZero<std::size_t> size, NonZero<std::size_t> alignment) noexcept
    {
        (void) size;
        assert(alignment <= alignof(std::max_align_t) && isValidAlignment(alignment));
        std::free(memory);
    }
};

} // export namespace pl

namespace pl
{
static_assert(allocator<Mallocator>, "Mallocator must be an allocator");
} // namespace pl

export namespace pl
{
template<class T, allocator A>
requires (sizeof(T) != 0)
[[nodiscard]] 
constexpr RE<Ptr<T>, SimpleError> alloc(A &&a) noexcept
{
    if consteval
    {
        // On consteval, std::allocator::allocate would never return null.
        // Does it???
        return makeNonNull_Unchecked(std::allocator<T>().allocate(1));
    }
    else
    {
        PL_TRY_ASSIGN(Ptr<void> memory, a.alloc(
                makeNonZero_Unchecked(sizeof(T)),
                makeNonZero_Unchecked(alignof(T))));

        return start_lifetime_as<T>(memory);
    }
}

template<class T, allocator A>
requires (sizeof(T) != 0)
[[nodiscard]] 
constexpr RE<Span<T>, SimpleError> alloc(A &&a, std::size_t count) noexcept
{
    if (count == 0) return {};

    if consteval
    {
        return {tags::in_place, std::allocator<T>().allocate(count), count};
    }
    else
    {
        PL_TRY_ASSIGN(Ptr<void> memory, a.alloc(
                makeNonZero_Unchecked(sizeof(T) * count),
                makeNonZero_Unchecked(alignof(T))));

        return start_lifetime_as_array<T>(memory, count);
    }
}

template<class T, std::size_t Count, allocator A>
requires (sizeof(T) != 0)
[[nodiscard]] 
constexpr RE<Span<T, Count>, SimpleError> alloc(A &&a) noexcept
{
    if constexpr (Count == 0)
        return {};

    if consteval
    {
        return Span<T, Count>(std::allocator<T>().allocate(Count), Count);
    }
    else
    {
        PL_TRY_ASSIGN(Ptr<void> memory, a.alloc(
                makeNonZero_Unchecked(sizeof(T) * Count),
                makeNonZero_Unchecked(alignof(T))));

        return start_lifetime_as_array<T, Count>(memory);
    }
}

template<class T, allocator A>
requires (sizeof(T) != 0)
constexpr void free(A &&a, Ptr<T> obj) noexcept
{
    if consteval
    {
        std::allocator<T>().deallocate(obj, 1);
    }
    else
    {
        a.free(
            obj,
            makeNonZero_Unchecked(sizeof(T)),
            makeNonZero_Unchecked(alignof(T)));
    }
}

template<class T, std::size_t N, allocator A>
requires (sizeof(T) != 0)
constexpr void free(A &&a, Span<T, N> arr) noexcept
{
    if (arr.empty()) return;
    if consteval
    {
        std::allocator<T>().deallocate(arr.data(), arr.size());
    }
    else
    {
        a.free(
            // Non-empty Span will NEVER have nullptr as data
            makeNonNull_Unchecked(arr.data()),
            makeNonZero_Unchecked(sizeof(T) * arr.size()),
            makeNonZero_Unchecked(alignof(T)));
    }
}

template<class T>
requires (sizeof(T) != 0)
class NewArr
{
public:
    [[nodiscard]] 
    constexpr NewArr(std::size_t count) noexcept : _count(count) {}

    template<allocator A>
    [[nodiscard]] 
    constexpr RE<Span<T>, SimpleError> operator()(A &&a) const noexcept
    {
        RE<Span<T>, SimpleError> result = alloc<T>(a, _count);
        if (result) for (T &obj : *result) default_construct_at(addr(obj));
        return result;
    }

    template<allocator A, class Constructor>
    [[nodiscard]]
    constexpr RE<Span<T>, SimpleError> operator()(A &&a, Constructor &&c) const noexcept
    {
        RE<Span<T>, SimpleError> result = alloc<T>(a, _count);
        if (result) for (T &obj : *result) c(addr(obj));
        return result;
    }
private:
    std::size_t _count;
};

template<class T>
requires (sizeof(T) != 0)
class New
{
public:
    template<allocator A, class ...Args>
    [[nodiscard]] 
    static constexpr RE<Ptr<T>, SimpleError> operator()(A &&a, Args &&...args) noexcept
    {
        RE<Ptr<T>, SimpleError> result = alloc<T>(a);
        if (result)
        {
            if constexpr (sizeof...(Args) == 0)
                default_construct_at(*result);
            else
                construct_at(*result, std::forward<Args>(args)...);
        }
        return result;
    }

    [[nodiscard]] 
    static constexpr NewArr<T> operator[](std::size_t count) noexcept
    {
        return count;
    }
};

template<class T> requires (sizeof(T) != 0)
constexpr New<T> new_;

template<class T, allocator A>
requires (sizeof(T) != 0)
constexpr void delete_(A &&a, Ptr<T> obj) noexcept
{
    destroy_at(obj);
    free(a, obj);
}

template<class T, std::size_t N, allocator A>
requires (sizeof(T) != 0)
constexpr void delete_(A &&a, Span<T, N> arr) noexcept
{
    std::ranges::destroy(arr);
    free(a, arr);
}
} // export namespace pl
