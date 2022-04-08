#pragma once

#include "typesystem.hpp"
#include "ints.hpp"

namespace core {

struct empty {};

template <typename... Ts>
union variadic_union;

template<> union variadic_union<> {};

template <typename T, typename... Ts>
union variadic_union<T,Ts...> {
    constexpr variadic_union() : _{} {}

    empty _;
    T value;
    variadic_union<Ts...> rest;
};



template <typename X, typename T, class=meta::check_valid>
struct overload_for {
    // T operator() (T) const;
};

template <typename X, typename T>
struct overload_for<X,T, meta::is_valid< decltype(T{std::declval<X>()}) >> {
    T operator() (T) const;
};

struct dummy_overload {
    struct no_match {};
    no_match operator() (...);
};

template <typename X, typename... Ts>
struct matches : dummy_overload, overload_for<X, Ts>... {
    // using match<Ts>::operator()...;
};

template <typename X, typename... Ts>
using best_match = decltype( matches<X, Ts...>{}(std::declval<X>()) );



template <typename... Ts>
struct either {
public:
    template <typename X, class Match = best_match<X, Ts...>>
    either (X && value) : index{ meta::find<Match, Ts...>() } {
        // static_assert(meta::set_contains<meta::typelist<Ts...>, Match>(), "not found");
        // static_assert(bool( core::Types<Ts...>.template find<Match>() ), "no match!");
        std::cerr << "index = " << index << "\n";
        std::cerr << Type< Match >;
        // std::cerr << "matched: " << core::Types<Ts...>.template at<*core::Types<Ts...>.template find<X>()>();
    }
private:
    variadic_union<Ts...> variants;
    size_t index = 0;
};

}//namespace core