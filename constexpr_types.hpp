#pragma once

#include <cstddef> // std::size_t
#include <initializer_list>

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

    constexpr operator bool() const { return !is_empty; }
    constexpr auto operator* () const { return data; }
    constexpr auto operator->() const { return &data; }
private:
    union { T data; };
    bool is_empty;
};

template <typename T>
constexpr auto cx_make_optional(const T& value) {
    return cx_optional<T>(value);
}


template <typename T, std::size_t N>
class cx_array {
public:
    constexpr cx_array (std::initializer_list<T> list) {
        size_t i=0;
        for (auto v : list) {
            data[i++] = std::move(v);
        }
    }


    // Iterators
    constexpr auto begin() const noexcept { return &data[0]; }
    constexpr auto end()   const noexcept { return &data[N]; }


    friend auto operator<< (std::ostream& os, cx_array array) -> std::ostream& {
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


}//namespace core
