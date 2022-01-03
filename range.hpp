#pragma once

#include <type_traits>
#include "macros.hpp"

namespace core {

// A stride-1 range
template <typename T>
class Range;

template <typename T>
class StrideRange;


template <typename T>
class RangeIterator;

template <typename T>
class StrideRangeIterator;


template <typename T>
inline
constexpr auto range(T end) noexcept -> Range<T>
{
    return {end};
}


template <typename T>
inline
constexpr auto range(T start, T end) noexcept -> Range<T>
{
    return Range<T>(start, end);
}


template <typename T>
inline
constexpr auto range(T start, T end, T step) noexcept -> StrideRange<T>
{
    return StrideRange<T>(start, end, step);
}

    
/**
 * @brief A stride-1 range: [begin, end)
 * 
 * @tparam T 
 */
template <typename T>
class Range {
public:

    const T from;
    const T to;


    constexpr Range(T end) noexcept
        : from {0}
        , to {end}
        {}
    
    constexpr Range(T begin, T end) noexcept
        : from {begin}
        , to   {end}
    {}
    
    constexpr auto begin() const noexcept -> RangeIterator<T>
    {
        return RangeIterator<T>( from );
    }
    
    constexpr auto end() const noexcept -> RangeIterator<T>
    {
        return RangeIterator<T>( to );
    }
    
    constexpr auto withStride(T stride) const noexcept -> StrideRange<T>
    {
        return StrideRange<T>( *this, stride );
    }
    
    constexpr auto reverse() const noexcept -> StrideRange<T>
    {
        return StrideRange<T>( to-1, from-1, -1 );
    }
    
    // Utility functions
    constexpr auto contains(T value) const noexcept -> bool;

};


template <typename T>
class StrideRange {
public:

    const T from;
    const T stride;
    const T to;

    constexpr StrideRange(T begin, T end, T step) noexcept 
    : from{ begin }
    , to{ end }
    , stride{ step } 
    {}
    
    constexpr auto begin() const noexcept -> StrideRangeIterator<T> {
        return StrideRangeIterator<T>( from, stride );
    }
    
    constexpr auto end() const noexcept -> StrideRangeIterator<T> {
        return StrideRangeIterator<T>( to, stride );
    }
    
    constexpr auto reverse() const noexcept -> StrideRange {
        return StrideRange( to-1, from-1, -1 );
    }

    // Utility functions
    constexpr auto contains(T value) const noexcept -> bool;
};


template <typename T>
class RangeIterator {
public:
    constexpr RangeIterator(T start_index) noexcept
    : index{ start_index }
    {}
    
    CORE_CPP14_CONSTEXPR_FUNC auto operator++() -> RangeIterator& {
        index += 1;
        return *this;
    }
    
    CORE_CPP14_CONSTEXPR_FUNC auto operator++(int _) -> RangeIterator {
        auto temp = *this;
        this->operator++();
        return temp;
    }
    
    constexpr bool operator==(RangeIterator const& other) const noexcept {
        return index == other.index;
    }
    
    constexpr bool operator!=(const RangeIterator& other) const noexcept {
        return !(*this == other);
    }
    
    CORE_CPP14_CONSTEXPR_FUNC auto operator*() const -> T {
        return index;
    }
private:
    T index;
};


template <typename T>
class StrideRangeIterator {
public:
    constexpr StrideRangeIterator(T _value, T _step) noexcept
        : current{_value}
        , step{_step} {}
    
    
    // operators to support range-based for:
    // 1. operator++()
    CORE_CPP14_CONSTEXPR_FUNC auto operator++ () -> StrideRangeIterator& {
        current += step;
        return *this;
    }
    
    CORE_CPP14_CONSTEXPR_FUNC auto operator++ (int _) -> StrideRangeIterator {
        auto temp = current;
        current += step;
        return temp;
    }
    
    constexpr bool operator!= (const StrideRangeIterator& other) const noexcept {
        return (step > 0) ? (current < other.current) : (current > other.current);
    }
    
    CORE_CPP14_CONSTEXPR_FUNC auto operator*() -> T& {
        return this->current;
    }
    
private:
    T current;
    T step;
};


template <typename T>
inline
constexpr bool Range<T>::contains(T value) const noexcept {
    return from <= value && value < to;
}


template <typename T>
inline
constexpr bool StrideRange<T>::contains(T value) const noexcept {
    return (value % stride == from) && Range<T>::contains(value);
}

}//namespace core