#pragma once

// Examples:
// // 
// //      +--lvalue 
// //      |    |   
// auto&& ret = v | apply(printer);
// //             |
// //         fmap( lvalue, in-place )   ->   lvalue
// //                                           |
// std::cout << "Ret: " << core::Type<decltype(ret)> << "\n\n";


// //         lvalue
// //           |     
// auto && v2 = v | apply($a * 2) | apply(printer);
// //       |          |
// //       |   lvalue |  create new   ->   rvalue
// //      rvalue                             |
// std::cout << "V2: " << core::Type<decltype(v2)> << "\n\n";


// auto w = v;

// // apply to temporary
// //        +------------rvalue
// //        |              |
// auto&& new_v = std::move(w) | apply([](auto && v){ v = v * 3; }) | apply(printer);
// //                          |
// //               fmap(rvalue, in-place)  ->  rvalue
// //                                             |
// std::cout << "new_v: " << core::Type<decltype(new_v)> << "\n\n";


// w = v;

// // apply to temporary + create new (should move from temporary when possible)
// //         +------------rvalue
// //         |              |
// auto&& new_v2 = std::move(v) | apply($a * 4) | apply(printer);
// //                           |                   |
// //                fmap(rvalue, create new)  ->  rvalue
// //                                               |
// std::cout << "new_v2: " << core::Type<decltype(new_v2)> << "\n\n";


#include <utility>
#include <cassert>
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
        static constexpr decltype(auto) impl(std::index_sequence<Is...>, F && f, T&& tuple) {
            [[maybe_unused]] std::initializer_list<int> _ {((void)std::forward<F>(f)( std::get<Is>(tuple) ), 0)... };
            return std::forward<T>(tuple);
            // return static_cast<T>(tuple);
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

    template <class NewType>
    struct create { 
        template <class From>
        static constexpr decltype(auto) from(From && original) {
            return NewType( original.size() );
        }

        static constexpr auto from(NewType && original) {
            return NewType( std::move(original) );
        }
    };

    template <typename T, size_t N>
    struct create< std::array<T,N> >{
        template <class From>
        static constexpr auto from(From const& original) { return std::array<T,N> {}; }
    };


    // Apply
    template <class NewType>
    struct iterable_apply { 
        template <class From, class F>
        static constexpr decltype(auto) to(From && original, F && f) {
            auto ret = create<NewType>::from( std::forward<From>(original) );

            if constexpr (core::is_indexable_with_v<NewType, size_t>) 
            {   // Indexable
                for (size_t i = 0; i < original.size(); ++i) {
                    ret[i] = std::forward<F>(f)( std::forward<From>(original)[i] );
                }
            }
            else  
            {   // Iterable general case
                for (auto&& [r, o] : core::zip(ret, std::forward<From>(original))) {
                    r = std::forward<F>(f)( o );
                }
            }
            return ret;
        }

        template <class F>
        static constexpr decltype(auto) to(NewType && original, F && f) {
            auto ret = create<NewType>::from( std::move(original) );
            for (auto && elem : ret) {
                elem = std::forward<F>(f)( elem );
            }
            return ret;
        }
    };

    template <>
    struct iterable_apply<void> {
        template <class From, class F>
        static constexpr decltype(auto) to(From&& original, F && f) {
            for (auto && elem : original) std::forward<F>(f)( elem );
            return std::forward<From>(original);
        }
    };


    template <typename F, typename Iterable,
        class=meta::require< core::is_iterable<Iterable>{} >
    >
    constexpr decltype(auto) operator| (Iterable&& iter, fmap<F> && map) {
        using NewT = decltype(std::move(map).f( *std::begin(iter) ));
        using R = typename meta::select<(core::Type<NewT> != core::Type<void>), 
            meta::defer< meta::stl_rewire_t, core::remove_cvref_t<Iterable>, NewT >,
            meta::identity< void >
        >::type;
        return iterable_apply<R>::to(std::forward<Iterable>(iter), std::move(map).f);
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