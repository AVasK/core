#include <type_traits>

template <bool Cond, typename Type>
using Enable_if = typename std::enable_if<Cond,Type>::type;


template <typename T>
class Range;

template <typename T>
class StrideRange;


// Range Iterators
// returned by begin() and end()
template <typename T>
class RangeIterator;

template <typename T>
class StrideRangeIterator;


template <typename T>
auto range(T end) -> Range<T>
{
    return Range<T>(end);
}

template <typename T>
auto range(T start, T end) -> Range<T>
{
    return Range<T>(start, end);
}

template <typename T>
auto range(T start, T end, T step) -> Range<T>
{
    return Range<T>(start, end, step);
}

    
/// Half-open range: [begin, end), no stride
template <typename T>
class Range {
public:
    Range(T end)
        : _begin {0}
        , _end {end}
        {}
    
    Range(T begin, T end)
        : _begin {begin}
        , _end   {end}
    {}
    
    auto begin() -> RangeIterator<T>
    {
        return RangeIterator<T>( _begin );
    }
    
    auto end() -> RangeIterator<T>
    {
        return RangeIterator<T>( _end );
    }
    
    auto withStride(T stride) -> StrideRange<T>
    {
        return StrideRange<T>( *this, stride );
    }
    
    auto reverse() -> StrideRange<T>
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
    StrideRange(Range<T> base, T stride)
    : Range<T> {std::move(base)}
    , _stride {stride}
    {}
    
    auto begin() -> StrideRangeIterator<T>
    {
        return StrideRangeIterator<T>( _begin, _stride );
    }
    
    auto end() -> StrideRangeIterator<T>
    {
        return StrideRangeIterator<T>( _end, _stride );
    }
    
    auto reverse() -> StrideRange
    {
        return StrideRange( Range<T>(_end-1, _begin-1), -1 );
    }
    
    
    // Utility functions
    constexpr auto contains(T value) const noexcept -> bool;
    
private:
    T _stride;
};

// No stride
template <typename T>
class RangeIterator {
public:
    
    RangeIterator(T start_index)
    : index{ start_index }
    {}
    
    auto operator++() -> RangeIterator&
    {
        index += 1;
        return *this;
    }
    
    auto operator++(int) -> RangeIterator
    {
        auto temp = *this;
        this->operator++();
        return temp;
    }
    
    auto operator==(RangeIterator const& other) -> bool
    {
        return index == other.index;
    }
    
    auto operator!=(const RangeIterator& other) -> bool
    {
        return !(*this == other);
    }
    
    auto operator*() const -> T
    {
        return index;
    }
    
private:
    T index;
};

// Range with stride
template <typename T>
class StrideRangeIterator {
public:
    StrideRangeIterator(T _value, T _step)
        : current{_value}
        , step{_step} {}
    
    
    // operators to support range-based for:
    // 1. operator++()
    inline auto operator++() -> StrideRangeIterator&
    {
        current += step;
        return *this;
    }
    
    // 1+. postfix is given for the sake of completeness
    // (but as usual is slightly less effective)
    inline auto operator++(int) -> StrideRangeIterator
    {
        auto temp = current;
        current += step;
        return temp;
    }
    
    // 2. operator!=(other)
    inline auto operator!=(const StrideRangeIterator& other) -> bool
    {
        return (step > 0) ? (current < other.current) : (current > other.current);
    }
    
    // 3. operator*()
    inline auto operator*() -> T&
    {
        return this->current;
    }
    
private:
    T current;
    T step;
};


// Impl.
template <typename T>
constexpr inline auto Range<T>::contains(T value) const noexcept -> bool
{
    return _begin <= value && value < _end;
}

template <typename T>
constexpr inline auto StrideRange<T>::contains(T value) const noexcept -> bool
{
    return (value % _stride == _begin) && Range<T>::contains(value);
}
