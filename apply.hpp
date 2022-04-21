#pragma once

#include <utility>
#include "typesystem/typelist.hpp"
#include "meta.hpp"
#include "traits/iter_traits.hpp"

namespace core {

namespace detail {
        
    template <typename F>
    struct fmap {
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

    // Tuple
    template <typename F, typename... Ts>
    constexpr decltype(auto) operator| (std::tuple<Ts...> const& t, fmap<F> && op) {
        constexpr bool all_void = core::Types<decltype( std::move(op).f(std::declval<Ts>()) )...>.all( core::is_void );
        using R = meta::select<all_void, void, std::tuple<decltype(std::move(op).f(std::declval<Ts>()))...>>;
        return tuple_map<R>::impl(std::make_index_sequence<sizeof...(Ts)>{}, std::move(op).f, t);
    }

    // Iterable
    template <typename F, typename Iterable,
        typename=meta::require< core::is_iterable<Iterable>{} >
    >
    constexpr decltype(auto) operator| (Iterable&& iter, fmap<F> && map) {
        using T = decltype( *std::begin(iter) );
        using NewT = decltype(std::move(map).f(std::declval< T >()));
        using NewIterable = meta::unpack_base<std::remove_reference_t<Iterable>, NewT>;

        NewIterable ret = std::forward<Iterable>( iter );

        for (auto& v : ret) {
            v = std::move(map).f( v );
        }

        return ret;
    } 

}//namespace detail

template <typename F>
constexpr auto apply(F && f) -> detail::fmap<F> { return {std::forward<F>(f)}; }

// template <typename F, typename T, size_t... Is>
// constexpr decltype(auto) tuple_map(std::index_sequence<Is...>, F && f, T&& tuple) {
//     if constexpr (core::Types<decltype( std::forward<F>(f)(std::get<Is>(tuple)) )...>.all( core::is_void )) {
//         [[maybe_unused]] std::initializer_list<int> _ {(std::forward<F>(f)( std::get<Is>(tuple) ), 0)... };
//     } else 
//         return std::make_tuple( std::forward<F>(f)( std::get<Is>(tuple) )... );
// }

}// namespace core