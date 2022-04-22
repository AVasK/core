#pragma once

#include <utility>
#include "typesystem/typelist.hpp"
#include "typesystem/type_manipulation.hpp"
#include "meta.hpp"
#include "traits/iter_traits.hpp"
#include "iteration.hpp"

namespace core {

namespace detail {
        
    template <typename F>
    struct fmap {
        F f;
    };

    // =====[ Tuple ]=====
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
    constexpr decltype(auto) operator| (std::tuple<Ts...> & t, fmap<F> && op) {
        constexpr bool all_void = core::Types<decltype( std::move(op).f(std::declval<Ts&>()) )...>.all( core::is_void );
        using R = meta::select<all_void, void, std::tuple<decltype(std::move(op).f(std::declval<Ts&>()))...>>;
        return tuple_map<R>::impl(std::make_index_sequence<sizeof...(Ts)>{}, std::move(op).f, t);
    }

    template <typename F, typename... Ts>
    constexpr decltype(auto) operator| (std::tuple<Ts...> const& t, fmap<F> && op) {
        constexpr bool all_void = core::Types<decltype( std::move(op).f(std::declval<Ts const&>()) )...>.all( core::is_void );
        using R = meta::select<all_void, void, std::tuple<decltype(std::move(op).f(std::declval<Ts const&>()))...>>;
        return tuple_map<R>::impl(std::make_index_sequence<sizeof...(Ts)>{}, std::move(op).f, t);
    }

    template <typename F, typename... Ts>
    constexpr decltype(auto) operator| (std::tuple<Ts...> && t, fmap<F> && op) {
        constexpr bool all_void = core::Types<decltype( std::move(op).f(std::declval<Ts&&>()) )...>.all( core::is_void );
        using R = meta::select<all_void, void, std::tuple<decltype(std::move(op).f(std::declval<Ts&&>()))...>>;
        return tuple_map<R>::impl(std::make_index_sequence<sizeof...(Ts)>{}, std::move(op).f, std::move(t));
    }



    // =====[ Iterable ]======

    template <typename R>
    struct iterable_map {
        template <typename F, typename Iterable>
        static constexpr R impl(Iterable&& iter, F && f) {
            R ret ( iter.size() );
            for (auto && [res_elem, iter_elem] : core::zip(ret, iter)) {
                res_elem = std::move(f)( iter_elem );
            }
            return ret;
        }
    };

    template <>
    struct iterable_map<void> {
        template <typename F, typename Iterable>
        static constexpr void impl(Iterable&& iter, F && f) {
            for (auto & elem : iter) std::move(f)( elem );
        }
    };


    template <typename F, typename Iterable,
        typename=meta::require< core::is_iterable<Iterable>{} >
    >
    constexpr decltype(auto) operator| (Iterable&& iter, fmap<F> && map) {
        using T = decltype( *std::begin(iter) );
        using NewT = decltype(std::move(map).f(std::declval< T >()));

        using R = typename meta::select<(core::Type<NewT> != core::Type<void>), 
            meta::defer< meta::unpack_base, core::remove_cvref_t<Iterable>, NewT >,
            meta::identity< void >
        >::type;

        return iterable_map<R>::impl(iter, std::move(map).f);
    }

}//namespace detail

/**
 * @brief apply: creates an apply operation object.
 * `apply` operation creates new object if F returns value, otherwise transforms passed object in place
 * 
 * @tparam F 
 * @param f 
 * @return detail::fmap<F> 
 */
template <typename F>
constexpr auto apply(F && f) -> detail::fmap<F> { return {std::forward<F>(f)}; }

}// namespace core