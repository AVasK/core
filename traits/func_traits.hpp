//! Function traits: result_of, callable_with

#pragma once

#include <type_traits>
#include "../meta.hpp"
#include "../macros.hpp"
#include "../function.hpp"


namespace core {

// ===== [ Result Of ] =====
template <typename F, typename... Args>
using result_of = decltype(invoke(std::declval<F>(), std::declval<Args>()...));
//decltype( std::declval<F>()( std::declval<Args>()... ) );



// ===== [ Callable With ] =====
namespace detail {
    template <class Inv, class=meta::check_valid>
    struct callable_with_impl : std::false_type {};

    template <typename F, typename... Args>
    struct callable_with_impl<meta::typelist<F,Args...>, meta::is_valid<result_of<F,Args...>>> 
    : std::true_type {};
}

template <typename F, typename... Args>
using callable_with = detail::callable_with_impl< meta::typelist<F, Args...> >;


// ===== [ Callable with Result ] ======
namespace detail {
    template <typename Ret, class Inv>
    struct callable_with_result_impl : std::false_type {};

    template <typename R, typename F, typename... Args>
    struct callable_with_result_impl<R, meta::typelist<F,Args...>> 
    : std::is_convertible< result_of<F,Args...>, R> {};
}

template <typename R, typename F, typename... Args>
using callable_with_result = detail::callable_with_result_impl< R, meta::typelist<F, Args...> >;


// ===== [ is_noexcept ] =====
template <typename F, typename... Args>
struct is_noexcept {
    struct _ {
        _ () noexcept(
            noexcept(std::declval<F>()(std::declval<Args>()...))
        ) {}
    };

    enum { value = std::is_nothrow_constructible<_>{} };
    operator bool() { return value; }
};


template <typename F, typename... Args>
CORE_CPP17_INLINE_VARIABLE constexpr auto is_noexcept_v = is_noexcept<F,Args...>::value;


}//namespace core