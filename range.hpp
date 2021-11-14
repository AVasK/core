#pragma once

#include <type_traits>

namespace core {


// A stride-1 range
template <typename T>
class Range;


template <typename T>
class StrideRange;


// returned by begin() [and end() (before C++17)]
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
constexpr auto range(T start, T end, T step) noexcept -> Range<T>
{
    return Range<T>(start, end, step);
}

    
template <typename T>
class Range {
public:
    constexpr Range(T end) noexcept
        : _begin {0}
        , _end {end}
        {}
    
    constexpr Range(T begin, T end) noexcept
        : _begin {begin}
        , _end   {end}
    {}
    
    constexpr auto begin() noexcept -> RangeIterator<T>
    {
        return RangeIterator<T>( _begin );
    }
    
    constexpr auto end() noexcept -> RangeIterator<T>
    {
        return RangeIterator<T>( _end );
    }
    
    constexpr auto withStride(T stride) noexcept -> StrideRange<T>
    {
        return StrideRange<T>( *this, stride );
    }
    
    constexpr auto reverse() noexcept -> StrideRange<T>
    {
        return StrideRange<T>( Range(_end-1, _begin-1), -1 );
    }
    
    // Utility functions
    constexpr auto contains(T value) const noexcept -> bool;

protected:
    T _begin;
    T _end;
};


template <typename T>
class StrideRange : public Range<T> {
    using Range<T>::_begin;
    using Range<T>::_end;

public:
    constexpr StrideRange(Range<T> base, T stride) noexcept
    : Range<T> {std::move(base)}
    , _stride {stride}
    {}
    
    constexpr auto begin() noexcept -> StrideRangeIterator<T> {
        return StrideRangeIterator<T>( _begin, _stride );
    }
    
    constexpr auto end() noexcept -> StrideRangeIterator<T> {
        return StrideRangeIterator<T>( _end, _stride );
    }
    
    constexpr auto reverse() noexcept -> StrideRange {
        return StrideRange( Range<T>(_end-1, _begin-1), -1 );
    }

    // Utility functions
    constexpr auto contains(T value) const noexcept -> bool;
private:
    T _stride;
};


template <typename T>
class RangeIterator {
public:
    constexpr RangeIterator(T start_index) noexcept
    : index{ start_index }
    {}
    
    constexpr auto operator++() -> RangeIterator& {
        index += 1;
        return *this;
    }
    
    constexpr auto operator++(int _) -> RangeIterator {
        auto temp = *this;
        this->operator++();
        return temp;
    }
    
    constexpr bool operator==(RangeIterator const& other) {
        return index == other.index;
    }
    
    constexpr bool operator!=(const RangeIterator& other) {
        return !(*this == other);
    }
    
    constexpr auto operator*() const -> T {
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
    constexpr auto operator++ () -> StrideRangeIterator& {
        current += step;
        return *this;
    }
    
    constexpr auto operator++ (int _) -> StrideRangeIterator {
        auto temp = current;
        current += step;
        return temp;
    }
    
    constexpr bool operator!= (const StrideRangeIterator& other) {
        return (step > 0) ? (current < other.current) : (current > other.current);
    }
    
    constexpr auto operator*() -> T& {
        return this->current;
    }
    
private:
    T current;
    T step;
};


template <typename T>
inline
constexpr bool Range<T>::contains(T value) const noexcept {
    return _begin <= value && value < _end;
}


template <typename T>
inline
constexpr bool StrideRange<T>::contains(T value) const noexcept {
    return (value % _stride == _begin) && Range<T>::contains(value);
}

}//namespace core