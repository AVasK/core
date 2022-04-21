#pragma once

#include <utility>
#include "typesystem/typelist.hpp"
#include "meta.hpp"

namespace core {

namespace detail {
        
    template <typename F>
    struct elementwise_apply {
        F f;
    };

    template <typename R>
    struct tuple_map {
        template <typename F, typename T, size_t... Is>
        static constexpr decltype(auto) impl(std::index_sequence<Is...>, F && f, T&& tuple) {
            return R{ std::forward<F>(f)( std::get<Is>(tuple) )... };
        }
    };

    template<>
    struct tuple_map<void> {
        template <typename F, typename T, size_t... Is>
        static constexpr void impl(std::index_sequence<Is...>, F && f, T&& tuple) {
            [[maybe_unused]] std::initializer_list<int> _ {((void)std::forward<F>(f)( std::get<Is>(tuple) ), 0)... };
        }
    };

    template <typename F, typename... Ts>
    constexpr decltype(auto) operator|(std::tuple<Ts...> const& t, elementwise_apply<F> && op) {
        constexpr bool all_void = core::Types<decltype( std::move(op).f(std::declval<Ts>()) )...>.all( core::is_void );
        using R = meta::select<all_void, void, std::tuple<decltype(std::move(op).f(std::declval<Ts>()))...>>;
        return tuple_map<R>::impl(std::make_index_sequence<sizeof...(Ts)>{}, std::move(op).f, t);
    }

}//namespace detail

template <typename F>
constexpr auto foreach(F && f) -> detail::elementwise_apply<F> { return {std::forward<F>(f)}; }

// template <typename F, typename T, size_t... Is>
// constexpr decltype(auto) tuple_map(std::index_sequence<Is...>, F && f, T&& tuple) {
//     if constexpr (core::Types<decltype( std::forward<F>(f)(std::get<Is>(tuple)) )...>.all( core::is_void )) {
//         [[maybe_unused]] std::initializer_list<int> _ {(std::forward<F>(f)( std::get<Is>(tuple) ), 0)... };
//     } else 
//         return std::make_tuple( std::forward<F>(f)( std::get<Is>(tuple) )... );
// }

}// namespace core