#pragma once

#include <cstddef> // std::size_t
#include <cassert>
#include <initializer_list>
#include <iostream>

#include "range.hpp"
#include "class/extension.hpp"
#include "class/mixins.hpp"

namespace core {

/**
 * @brief A constexpr optional class
 *        A pretty naive implementation, due to placement new & reinterpret cast being impossible inside a constexpr
 * @returns either an empty optional or a value of type T.
 * @tparam T 
 * @namespace core
 */
template <typename T>
class cx_optional {
public:
    constexpr cx_optional (const T& v) : _data{v}, is_empty{false} {}
    constexpr cx_optional () : is_empty{true} {};

    struct cx_optional_empty {};

    constexpr explicit operator bool() const { return !is_empty; }

    constexpr auto value() const {
        assert(!is_empty);
        return _data;
        // else throw cx_optional_empty {};
    }

    constexpr auto operator* () const {
        return value();
    }

    constexpr auto operator->() const { 
        return value();
    }

private:
    union { T _data; };
    bool is_empty=true;
};

template <typename T>
constexpr auto make_cx_optional(const T& value) {
    return cx_optional<T>(value);
}



/**
 * @brief constexpr array type
 *        the array itself is an immutable compile-time evaluated type,
 *        all the work is done returning new - modified - arrays.
 * 
 * @tparam T - type of elements
 * @tparam N - size of array
 */
template <typename T, std::size_t N>
class cx_array {
public:

    explicit constexpr cx_array() = default; 

    constexpr cx_array (std::initializer_list<T> list) {
        size_t i=0;
        for (auto v : list) {
            _data[i++] = v;
        }
    }


    template <size_t M>
    constexpr cx_array (cx_array<T,M> const& other) {
        for (auto i : range(N)) {
            _data[i] = other[i];
        }
    }


    constexpr cx_array (T const* other) {
        for (auto i : range(N)) {
            _data[i] = other[i];
        }
    }


    static constexpr cx_array fill(T fill_value) {
        auto arr = cx_array();
        for (auto i : core::range(N)) {
            arr[i] = fill_value;
        }
        return arr;
    }

    
    /**
     * @brief Initializing by concatenating the other two cx_arrays
     * 
     * @tparam M - size of the first array
     * @tparam L - size of the second array
     * @invariant M+L == N
     */
    template <size_t M, size_t L, typename=std::enable_if_t<(M+L == N)>>
    constexpr cx_array (cx_array<T,M> const& a, cx_array<T,L> const& b) {
        static_assert(M+L == N, 
        "the result of concatenation of two arrays should have size exaclty equal to the sum of their sizes");
        size_t i=0;
        for (auto && v : a) {
            _data[i++] = v;
        }
        for (auto && w : b) {
            _data[i++] = w;
        }
    }


    template <size_t M, size_t L>
    constexpr cx_array (cx_array<T,M> && a, cx_array<T,L> && b) {
        static_assert(M+L == N, 
        "the result of concatenation of two arrays should have size exaclty equal to the sum of their sizes");
        size_t i=0;
        for (auto && v : a) {
            _data[i++] = std::move(v);
        }
        for (auto && w : b) {
            _data[i++] = std::move(w);
        }
    }


    /**
     * @brief Slicing returns a new array == old[From : To]
     * 
     * @tparam From 
     * @tparam To 
     * @return cx_array<T, To-From> 
     */
    template <size_t From, size_t To>
    constexpr auto slice() const -> cx_array<T, To-From> {
        static_assert(0 <= From && From <= N && 0 <= To && To <= N && To > From ,"Slice out of bounds of the array");
        return {&_data[From]};

    }

    /**
     * @brief Indexing array
     * 
     * @tparam Idx 
     * @return T 
     */
    template <size_t Idx>
    constexpr auto at() const -> T {
        static_assert(0<= Idx && Idx < N, "cx_array::at<Idx>: Index `Idx` out of range");
        return _data[Idx];
    }


    constexpr auto operator[] (size_t i) -> T& {
        return _data[i];
    }

    constexpr auto operator[] (size_t i) const -> T const& {
        return _data[i];
    }

    constexpr auto size() const -> size_t { return N; }


    struct value_with_position {
        T value;
        size_t at;

        constexpr value_with_position (T val, size_t pos) : value{val}, at{pos} {}
        constexpr operator T() const { return value; }
    };

    constexpr auto max() const {
        auto res = value_with_position(_data[0], 0);
        for (auto i : range(N)) {
            if (_data[i] > res) res = {_data[i], i};
        }
        return res;
    }


    constexpr auto min() const {
        auto res = value_with_position(_data[0], 0);
        for (auto i : range(N)) {
            if (_data[i] < res) res = {_data[i], i};
        }
        return res;
    }


    constexpr auto find(T value, size_t from=0) const -> cx_optional<size_t> {
        if (from >= N) return {};

        for (auto i : range(from, N)) {
            if (_data[i] == value) {
                return i;
            }
        }
        return {};
    }


    constexpr auto count(T value, size_t from=0) const -> size_t {
        size_t count = 0;
        for (auto i : range(from, N)) {
            if (_data[i] == value) {
                count += 1;
            }
        }
        return count;
    }
    

    template <typename F>
    constexpr auto where(F const& f, size_t from=0) const -> cx_optional<size_t> {
        if (from >= N) return {};

        for (auto i : range(from, N)) {
            if ( f(_data[i]) ) {
                return i;
            }
        }
        return {};
    }

    const T* data() const { return &_data[0]; }


    // Iterators
    constexpr auto begin() const noexcept { return &_data[0]; }
    constexpr auto end()   const noexcept { return &_data[N]; }

    // Printing
    friend auto operator<< (std::ostream& os, cx_array const& arr) -> std::ostream& {
        os << "[";
        for (auto& v : arr) {
            os << v << ", ";
        }
        os << "\b\b]";
        return os;
    }

private:
    T _data[N];
};

template <typename T, size_t N, size_t M>
constexpr auto operator+ (cx_array<T,N> const& a, cx_array<T,M> const& b) -> cx_array<T, N+M> {
    return {a, b};
}


/////////////// [ cx_string ] //////////////
CORE_MIXIN( MCopyableFromBase,
    template <typename T>
    constexpr MCopyableFromBase( T && other ) : Base{ std::forward<T>(other) } {}
);

CORE_MIXIN( MToString,
    operator std::string() const { return std::string( this->data(), this->size() ); }
);

template <std::size_t N>
using cx_string = core::extend< cx_array<char,N>, MCopyableFromBase, MToString >;

template <size_t N>
auto operator<< (std::ostream& os, cx_string<N> const& cxstr) -> std::ostream& {
    for (auto& v : cxstr) {
        os << v;
    }
    return os;
}

template <size_t N, size_t M>
constexpr auto operator+ (cx_string<N> const& a, cx_string<M> const& b) -> cx_string<N+M> {
    return {a, b};
}

template <size_t N>
constexpr auto cx_str (const char (&cstr) [N]) {
    return cx_string<N-1>{&cstr[0]};
}

template <size_t N>
constexpr auto cx_str (cx_array<char, N> const& arr) {
    return cx_string<N>{arr};
}


}//namespace core
