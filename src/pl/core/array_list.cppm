module;
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include <pl/macro.hpp>

export module pl.core:array_list;

import :error;
import :iterator;
import :memory;
import :result_error;
import :span;

export namespace pl
{
template<class T, allocator A>
class ArrayList;
} // export namespace pl

namespace pl
{
template<class T>
class ArrayListIterator
{
public:
    using iterator_concept = std::contiguous_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using element_type = T;
    using reference = T &;
    using pointer = T *;

    ArrayListIterator           ()                          = default;
    ArrayListIterator           (ArrayListIterator const &) = default;
    ArrayListIterator &operator=(ArrayListIterator const &) = default;

    template<class A>
    constexpr ArrayListIterator(
        T *p
#ifndef NDEBUG
        ,
        ArrayList<T, A> &arr
#endif
    ) noexcept
    :
        _p(p)
#ifndef NDEBUG
        ,
        _begin(arr.data())
#endif
    {}

    constexpr ArrayListIterator &operator++() noexcept
    {
        ++_p;
        return *this;
    }

    constexpr ArrayListIterator operator++(int) noexcept
    {
        auto copy = *this;
        ++_p;
        return copy;
    }

    constexpr ArrayListIterator &operator--() noexcept
    {
        --_p;
        return *this;
    }

    constexpr ArrayListIterator operator--(int) noexcept
    {
        auto copy = *this;
        --_p;
        return copy;
    }

    constexpr ArrayListIterator &operator+=(difference_type d) noexcept
    {
        _p += d;
        return *this;
    }

    constexpr ArrayListIterator &operator-=(difference_type d) noexcept
    {
        _p -= d;
        return *this;
    }

    friend constexpr ArrayListIterator operator+(ArrayListIterator i, difference_type d) noexcept
    {
        i._p += d;
        return i;
    }

    friend constexpr ArrayListIterator operator+(difference_type d, ArrayListIterator i) noexcept
    {
        i._p += d;
        return i;
    }

    friend constexpr ArrayListIterator operator-(ArrayListIterator i, difference_type d) noexcept
    {
        i._p -= d;
        return i;
    }

    friend constexpr difference_type operator-(ArrayListIterator a, ArrayListIterator b) noexcept
    {
        PL_ASSERT(a._begin == b._begin);
        return a._p - b._p;
    }

    constexpr reference operator*() const noexcept
    {
        return *_p;
    }

    constexpr pointer operator->() const noexcept
    {
        return _p;
    }

    constexpr reference operator[](difference_type d) const noexcept
    {
        return _p[d];
    }

    constexpr bool operator==(ArrayListIterator const &other) const noexcept
    {
        PL_ASSERT(_begin == other._begin);
        return _p == other._p;
    }

    constexpr auto operator<=>(ArrayListIterator const &other) const noexcept
    {
        PL_ASSERT(_begin == other._begin);
        return _p <=> other._p;
    }
private:
    T *_p;
#ifndef NDEBUG
    T *_begin;
    // cannot add end iterator, because the container's size could keep increasing
#endif
};
} // namespace pl

export namespace pl
{
template<class T, allocator A = Mallocator>
class ArrayList
{
public:
    using value_type             = T;
    using allocator_type         = A;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = value_type       &;
    using const_reference        = value_type const &;
    using pointer                = value_type       *;
    using const_pointer          = value_type const *;
    using iterator               = ArrayListIterator<T>;
    using const_iterator         = const_iterator<iterator>;
    using reverse_iterator       = std::reverse_iterator<      iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    

    [[nodiscard]] ArrayList() = default;

    [[nodiscard]] explicit constexpr ArrayList(allocator_type const &a) noexcept
    : _allocator(a) {}

    [[nodiscard]] constexpr ArrayList(ArrayList &&other) noexcept
    : 
        _arr(std::exchange(other._arr, {})),
        _end(std::exchange(other._end, {})),
        _allocator(other._allocator)
    {}

    constexpr ArrayList &operator=(ArrayList &&other)
    noexcept(std::is_nothrow_copy_assignable_v<A>)
    {
        _arr = std::exchange(other._arr, {});
        _allocator = other._allocator;
        return *this;
    }

    constexpr ~ArrayList()
    {
        std::ranges::destroy(_arr.data(), _end);
        free(_allocator, _arr);
    }

    template<std::input_iterator I, std::sentinel_for<I> S>
    [[nodiscard]] constexpr RE<void, SimpleError> assign(I first, S last)
    {
        return assign_elements(first, last);
    }

    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept
    {
        return _allocator;
    }

    [[nodiscard]] constexpr reference operator[](size_type idx) noexcept
    {
        PL_ASSERT(idx < size());
        return _arr.data()[idx];
    }

    [[nodiscard]] constexpr const_reference operator[](size_type idx) const noexcept
    {
        PL_ASSERT(idx < size());
        return _arr.data()[idx];
    }

    [[nodiscard]] constexpr reference front() noexcept
    {
        PL_ASSERT(!empty());
        return *_arr.data();
    }

    [[nodiscard]] constexpr const_reference front() const noexcept
    {
        PL_ASSERT(!empty());
        return *_arr.data();
    }

    [[nodiscard]] constexpr reference back() noexcept
    {
        PL_ASSERT(!empty());
        return _end[-1];
    }

    [[nodiscard]] constexpr const_reference back() const noexcept
    {
        PL_ASSERT(!empty());
        return _end[-1];
    }

    [[nodiscard]] constexpr T *data() noexcept
    {
        return _arr.data();
    }

    [[nodiscard]] constexpr T const *data() const noexcept
    {
        return _arr.data();
    }

    [[nodiscard]] constexpr iterator begin() noexcept
    {
        return
        {
            _arr.data(),
#ifndef NDEBUG
            *this,
#endif
        };
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept
    {
        return const_iterator(const_cast<ArrayList *>(this)->begin());
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept
    {
        return begin();
    }

    [[nodiscard]] constexpr iterator end() noexcept
    {
        return
        {
            _end,
#ifndef NDEBUG
            *this,
#endif
        };
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept
    {
        return const_iterator(const_cast<ArrayList *>(this)->end());
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept
    {
        return end();
    }

    [[nodiscard]] constexpr reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }

    [[nodiscard]] constexpr reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }

    [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return size() == 0;
    }

    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return size_type(_end - _arr.data());
    }

    // Reserve capacity enough for extra elements.
    [[nodiscard]] constexpr RE<void, SimpleError> reserve(size_type num_beyond_size)
    {
        return reserve_capacity(size() + num_beyond_size);
    }

    [[nodiscard]] constexpr RE<void, SimpleError> reserve_exact(size_type num_beyond_size)
    {
        return reserve_capacity_exact(size() + num_beyond_size);
    }

    [[nodiscard]] constexpr RE<void, SimpleError> reserve_capacity(size_type required_capacity)
    {
        if (required_capacity <= capacity()) return {};
        return reallocate(get_new_geometric_capacity(required_capacity - capacity()));
    }

    [[nodiscard]] constexpr RE<void, SimpleError> reserve_capacity_exact(size_type required_capacity)
    {
        if (required_capacity <= capacity()) return {};
        return reallocate(required_capacity);
    }

    [[nodiscard]] constexpr size_type capacity() const noexcept
    {
        return _arr.size();
    }

    [[nodiscard]] constexpr RE<void, SimpleError> shrink_to_fit()
    {
        return reallocate(size());
    }

    constexpr void clear() noexcept
    {
        // Must leave capacity at the same value
        // So only destroy elements, but don't deallocate.
        std::ranges::destroy(_arr.data(), _end);
        _end = _arr.data();
    }

    template<std::input_iterator I, std::sentinel_for<I> S>
    [[nodiscard]]
    constexpr RE<iterator, SimpleError> append(I first, S last)
    {
        return append_elements(first, last);
    }

    template<std::input_iterator I, std::sentinel_for<I> S>
    [[nodiscard]]
    constexpr RE<iterator, SimpleError> insert(const_iterator pos, I first, S last)
    {
        if (end() == pos.base()) 
            return append_elements(first, last);
        else
            return insert_elements(std::move(pos).base(), first, last);
    }

    [[nodiscard]]
    constexpr RE<void, SimpleError> push_back(T const &value)
    noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires std::is_copy_constructible_v<T>
    {
        return emplace_back(value);
    }

    [[nodiscard]]
    constexpr RE<void, SimpleError> push_back(T &&value)
    noexcept(std::is_nothrow_move_constructible_v<T>)
    requires std::is_move_constructible_v<T>
    {
        return emplace_back(std::move(value));
    }

    template<class ...Args>
    [[nodiscard]]
    constexpr RE<void, SimpleError> emplace_back(Args &&...args)
    noexcept(std::is_nothrow_constructible_v<T, Args...>)
    requires std::is_constructible_v<T, Args...>
    {
        PL_TRY_DISCARD(reserve(1));
        std::construct_at(_end++, std::forward<Args>(args)...);
        return {};
    }

    constexpr void pop_back() noexcept
    {
        PL_ASSERT(!empty());
        std::destroy_at(--_end);
    }

    constexpr void swap(ArrayList &other)
    {
        using std::swap;
        swap(_arr      , other._arr      );
        swap(_end      , other._end      );
        swap(_allocator, other._allocator);
    }

private:
    [[nodiscard]] constexpr RE<void, SimpleError> reallocate(size_type const new_capacity)
    {
        namespace rng = std::ranges;

        const size_type new_size = std::min(size(), new_capacity);
        RE<Span<T>, SimpleError> alloc_result = alloc<T>(_allocator, new_capacity);
        if (!alloc_result) return {tags::error, std::move(alloc_result).error()};
        Span<T> &new_arr = *alloc_result;
        if consteval
        {
            auto ifirst =         begin(); auto ilast  =         end();
            auto ofirst = new_arr.begin(); auto olast  = new_arr.end();
            for (; ifirst != ilast && ofirst != olast; ++ifirst, ++ofirst)
            {
                rng::construct_at(std::to_address(ofirst), rng::iter_move(ifirst));
            }
        }
        else
        {
            rng::uninitialized_move(*this, new_arr);
        }
        rng::destroy(*this);
        free(_allocator, _arr);
        _arr = new_arr;
        _end = _arr.data() + new_size;
        return {};
    }

    // Returns the equivalent of std::distance(first, last) as the first element,
    // and the equivalent of std::next(first, std::min(std::distance(first, last), max_distance))
    // as the second element.
    template<std::forward_iterator I, std::sentinel_for<I> S>
    [[nodiscard]]
    static constexpr std::pair<difference_type, I> distance_and_advance(
        I first, S last, difference_type max_distance)
    {
        difference_type distance = 0;
        for (; first != last && distance != max_distance; ++distance, ++first);
        auto mid = first;
        for (; first != last; ++first, ++distance);
        return {distance, mid};
    }

    // Returns the equivalent of std::distance(first, last) as the first element,
    // and the equivalent of std::next(first, std::min(std::distance(first, last), max_distance))
    // as the second element.
    template<std::random_access_iterator I, std::sized_sentinel_for<I> S>
    [[nodiscard]]
    static constexpr std::pair<difference_type, I> distance_and_advance(
        I first, S last, difference_type max_distance)
    {
        auto distance = std::distance(first, last);
        std::advance(first, std::min(distance, std::iter_difference_t<I>(max_distance)));
        return {difference_type(distance), first};
    }

    template<std::input_iterator I, std::sentinel_for<I> S>
    [[nodiscard]]
    constexpr RE<void, SimpleError> assign_elements(I first, S last)
    {
        iterator ofirst = begin();
        iterator olast = end();
        for (; ofirst != olast && first != last; ++ofirst, ++first)
        {
            *ofirst = *first;
        }
        std::ranges::destroy(ofirst, olast);
        _end = std::to_address(ofirst);

        PL_TRY_DISCARD(append(first, last));

        return {};
    }

    template<std::forward_iterator I, std::sentinel_for<I> S>
    [[nodiscard]]
    constexpr RE<void, SimpleError> assign_elements(I first, S last)
    {
        namespace rng = std::ranges;

        auto [new_size, mid] = distance_and_advance(first, last, static_cast<difference_type>(size()));
        // auto new_size = size_type(std::distance(first, last));
        PL_TRY_DISCARD(reserve_capacity(static_cast<size_type>(new_size)));

        iterator it_begin = begin();
        iterator it_end = end();
        rng::destroy(rng::copy(first, mid, it_begin).out, it_end);
        if consteval
        {
            for (; mid != last; ++it_end, ++mid)
                rng::construct_at(std::to_address(it_end), *mid);
        }
        else
        {
            it_end = rng::uninitialized_copy(mid, last, it_end, it_begin + new_size).out;
        }

        _end = std::to_address(it_begin + new_size);
        return {};
    }

    template<std::input_iterator I, std::sentinel_for<I> S>
    [[nodiscard]]
    constexpr RE<iterator, SimpleError> append_elements(I first, S last)
    {
        for (; first != last; ++first)
        {
            PL_TRY_DISCARD(emplace_back(*first));
        }

        return {};
    }

    template<std::forward_iterator I, std::sentinel_for<I> S>
    [[nodiscard]]
    constexpr RE<iterator, SimpleError> append_elements(I first, S last)
    {
        namespace rng = std::ranges;
        auto num_extra = std::distance(first, last);
        PL_TRY_DISCARD(reserve(size_type(num_extra)));
        
        auto ofirst = end();
        auto olast = ofirst + num_extra;
        if consteval
        {
            for (; ofirst != olast; ++ofirst, ++first)
                rng::construct_at(std::to_address(ofirst), *first);
        }
        else
        {
            rng::uninitialized_copy(first, last, ofirst, olast);
        }

        _end = std::to_address(olast);
        return ofirst;
    }

    template<std::input_iterator I, std::sentinel_for<I> S>
    [[nodiscard]]
    constexpr RE<iterator, SimpleError> insert_elements(iterator pos, I first, S last)
    {
        ArrayList tmp(_allocator);
        PL_TRY_DISCARD(tmp.append(first, last));
        return insert_elements(pos, tmp.begin(), tmp.end());
    }

    // Behaviour is undefined if first > last
    template<std::forward_iterator I, std::sentinel_for<I> S>
    [[nodiscard]]
    constexpr RE<iterator, SimpleError> insert_elements(iterator pos, I first, S last)
    {
        PL_ASSERT(begin() <= pos && pos <= end());
        if constexpr (std::sized_sentinel_for<S, I>)
        {
            PL_ASSERT(last - first >= 0);
        }

        namespace rng = std::ranges;

        difference_type dist_from_end = end() - pos;

        // mid should match with end() or less
        auto [num_extra, mid] = distance_and_advance(first, last, dist_from_end);
        PL_TRY_DISCARD(reserve(size_type(num_extra)));

        iterator it_end = end();

        // since reallocation could occur which will cause iterator invalidation
        // need to reassign pos
        pos = it_end - dist_from_end;

        iterator ilast = it_end;
        iterator ifirst = std::max(
            pos,
            ilast - std::min(num_extra, static_cast<difference_type>(size())));
        
        if consteval
        {
            iterator olast = ilast + num_extra;
            while (ilast != ifirst)
                rng::construct_at(std::to_address(--olast), rng::iter_move(--ilast));
        }
        else
        {
            std::uninitialized_move(ifirst, ilast, ifirst + num_extra);
            ilast = ifirst;
        }

        ifirst = pos;
        rng::move_backward(ifirst, ilast, ilast + num_extra);
        rng::copy(first, mid, pos);
        
        iterator new_end = pos + num_extra;

        if consteval
        {
            for (; mid != last && it_end != new_end; ++mid, ++it_end)
                rng::construct_at(std::to_address(it_end), *mid);
            
        }
        else
        {
            rng::uninitialized_copy(mid, last, it_end, pos + num_extra);
        }
        _end += num_extra;
        return pos;
    }

    [[nodiscard]] constexpr size_type get_new_geometric_capacity(size_type extra_capacity) noexcept
    {
        size_type current_capacity = capacity();
        return current_capacity + std::max(current_capacity / 2, extra_capacity);
    }

    Span<T> _arr = {};
    T *_end = {};
    [[no_unique_address]] A _allocator = {};
};
} // export namespace pl
