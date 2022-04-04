#pragma once
//! TODO: Rewrite for variadic case -- more than 2 iterables can be zipped
// Pair iteration for range-based for loops:

#include <utility>
#include <iterator>


namespace core {

template <class Iter1, class Iter2>
class zip_iterator;


template <class T1, class T2>
class zip_adaptor {
public:
    template <class X, class Y>
    zip_adaptor(X&& x, Y&& y) : _i1{std::forward<X>(x)}, _i2{std::forward<Y>(y)} {}

    auto begin() { return zip_iterator< decltype(std::begin(_i1)), decltype(std::begin(_i2))>( std::begin(_i1), std::begin(_i2) ); }
    auto end() { return zip_iterator< decltype(std::begin(_i1)), decltype(std::begin(_i2))>( std::end(_i1), std::end(_i2) ); }
private:
    T1 _i1;
    T2 _i2;
};


template <class Iter1, class Iter2>
class zip_iterator {
public:
    zip_iterator(Iter1 i1, Iter2 i2) : _i1{std::move(i1)}, _i2{std::move(i2)} {}

    auto operator++ () {
        return zip_iterator { ++_i1, ++_i2 };
    }

    auto operator++ (int _) {
        return zip_iterator { _i1++, _i2++ };
    }

    bool operator== (zip_iterator const& other) {
        return (_i1 == other._i1) || (_i2 == other._i2);
    }

     bool operator!= (zip_iterator const& other) {
        return (_i1 != other._i1) && (_i2 != other._i2);
    }

    auto operator*() {
        return std::make_pair(*_i1, *_i2);
    }

private:
    Iter1 _i1;
    Iter2 _i2;
};


template <typename T, typename U>
auto zip(T && t, U && u) {
    return zip_adaptor<T,U>(std::forward<T>(t), std::forward<U>(u));
}

template <typename Iterable, typename Counter>
auto enumerate(Iterable && iterable, Counter start=0) {
    return zip_adaptor<Iterable, core::Range<Counter>>(std::forward<Iterable>(iterable), core::range<Counter>(start, iterable.size()+start));
}

}// namespace core