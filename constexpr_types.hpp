#pragma once

#include <cstddef> // std::size_t
#include <initializer_list>

#include "range.hpp"

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
    constexpr cx_optional (const T& v) : data{v}, is_empty{false} {}
    constexpr cx_optional () : is_empty{true} {};

    struct cx_optional_empty {};

    constexpr operator bool() const { return !is_empty; }

    constexpr auto value() const {
        if (!is_empty) return data;
        else throw cx_optional_empty {};
    }

    constexpr auto operator* () const {
        return value();
    }

    constexpr auto operator->() const { 
        return value();
    }

private:
    union { T data; };
    bool is_empty;
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
    constexpr cx_array (std::initializer_list<T> list) {
        size_t i=0;
        for (auto v : list) {
            data[i++] = v;
        }
    }


    template <size_t M>
    constexpr cx_array (cx_array<T,M> const& other) {
        for (auto i : range(N)) {
            data[i] = other[i];
        }
    }


    constexpr cx_array (T const* other) {
        for (auto i : range(N)) {
            data[i] = other[i];
        }
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
            data[i++] = v;
        }
        for (auto && w : b) {
            data[i++] = w;
        }
    }


    template <size_t M, size_t L>
    constexpr cx_array (cx_array<T,M> && a, cx_array<T,L> && b) {
        static_assert(M+L == N, 
        "the result of concatenation of two arrays should have size exaclty equal to the sum of their sizes");
        size_t i=0;
        for (auto && v : a) {
            data[i++] = std::move(v);
        }
        for (auto && w : b) {
            data[i++] = std::move(w);
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
        return {&data[From]};

    }

    /**
     * @brief Indexing array
     * 
     * @tparam Idx 
     * @return T 
     */
    template <size_t Idx>
    constexpr auto at() const -> T {
        static_assert(0<= Idx && Idx < N, "cx_array::operator[]: Index `Idx` out of range");
        return data[Idx];
    }

    constexpr auto operator[] (size_t i) const -> T {
        return data[i];
    }

    constexpr auto size() const -> size_t { return N; }


    struct value_with_position {
        T value;
        size_t at;

        constexpr value_with_position (T val, size_t pos) : value{val}, at{pos} {}
        constexpr operator T() const { return value; }
    };

    constexpr auto max() const {
        auto res = value_with_position(data[0], 0);
        for (auto i : range(N)) {
            if (data[i] > res) res = {data[i], i};
        }
        return res;
    }


    constexpr auto min() const {
        auto res = value_with_position(data[0], 0);
        for (auto i : range(N)) {
            if (data[i] < res) res = {data[i], i};
        }
        return res;
    }


    constexpr auto find(T value, size_t from=0) const -> cx_optional<size_t> {
        if (from >= N) return {};

        for (auto i : range(from, N)) {
            if (data[i] == value) {
                return i;
            }
        }
        return {};
    }


    constexpr auto count(T value, size_t from=0) const -> size_t {
        size_t count = 0;
        for (auto i : range(from, N)) {
            if (data[i] == value) {
                count += 1;
            }
        }
        return count;
    }
    

    template <typename F>
    constexpr auto where(F const& f, size_t from=0) const -> cx_optional<size_t> {
        if (from >= N) return {};

        for (auto i : range(from, N)) {
            if ( f(data[i]) ) {
                return i;
            }
        }
        return {};
    }

    // Iterators
    constexpr auto begin() const noexcept { return &data[0]; }
    constexpr auto end()   const noexcept { return &data[N]; }

    // Printing
    friend auto operator<< (std::ostream& os, cx_array const& array) -> std::ostream& {
        os << "[";
        for (auto& v : array) {
            os << v << ", ";
        }
        os << "\b\b]";
        return os;
    }

private:
    T data[N];
};

template <typename T, size_t N, size_t M>
constexpr auto operator+ (cx_array<T,N> const& a, cx_array<T,M> const& b) -> cx_array<T, N+M> {
    return {a, b};
}


///TODO: REWRITE
template <typename T, std::size_t N>
class cx_string : public cx_array<T,N> {
public:

    constexpr cx_string (T const* other) : cx_array<T,N>{other} {};

    constexpr cx_string (cx_array<T,N>&& temp) : cx_array<T,N>{temp} {};
    // Printing
    friend auto operator<< (std::ostream& os, cx_string const& s) -> std::ostream& {
        for (auto& v : s) {
            os << v;
        }
        return os;
    }

};


}//namespace core
