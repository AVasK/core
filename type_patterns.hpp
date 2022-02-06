#pragma once 

#include <iostream>
#include <type_traits>
#include "meta_core.hpp"
#include "typesystem.hpp" // Temporary - remove after testing!

namespace core {


///TODO: Cleanup!
struct wildcard {};
using _ = wildcard;

struct variadic_wildcard {};
using ___ = variadic_wildcard;


// Auxiliary types
template <typename... Ts>
struct matched_types {};

template <typename... Ts>
struct variadic {};

template <typename... Ts>
struct func_match {};



template <typename T, class Pattern>
struct match;

template <typename P>
using pattern = match<P,P>;


template <typename T, bool result>
struct match_result {
    static constexpr bool value = result;
    using type = T;
};


template <typename T, class Pattern>
struct match : meta::select< 
    std::is_same<T,Pattern>::value,
    match_result<T, true>,
    match_result<T, false> 
> {
    template <typename X> using pass = meta::select< 
        std::is_same<T,Pattern>::value,
        T,
        meta::error
    >;
};


template <typename T>
struct match<T, _> : match_result<T, true> {
    template <typename X> using pass = X;
};

template <typename... Ts>
struct match<variadic<Ts...>, ___> : match_result<matched_types<Ts...>, true> {
    template <typename... Xs> using pass = meta::typelist<Xs...>;
};


template <typename T, typename P>
struct match <const T, const P> : match<T,P> {
    template <typename X> using pass = typename pattern<P>::template pass<X> const;
};

template <typename T, typename P>
struct match <T*, P*> : match<T,P> {
    template <typename X> using pass = typename pattern<P>::template pass<X> *;
};

template <typename T, typename P>
struct match <T&, P&> : match<T,P> {
    template <typename X> using pass = typename pattern<P>::template pass<X> &;
};

template <typename T, typename P>
struct match <T[], P[]> : match<T,P> {
    template <typename X> using pass = typename pattern<P>::template pass<X> [];
};

template <typename T, typename P>
struct match <const T[], const P[]> : match<T,P> {
    template <typename X> using pass = const typename pattern<P>::template pass<X> [];
};

template <typename T, typename P, size_t N>
struct match <T[N], P[N]> : match<T,P> {
    template <typename X> using pass = typename pattern<P>::template pass<X> [N];
};

template <typename T, typename P, size_t N>
struct match <const T[N], const P[N]> : match<T,P> {
    template <typename X> using pass = const typename pattern<P>::template pass<X> [N];
};


/// Func match
template <typename TR, typename...Ts, typename PR, typename... Ps>
struct match <TR(Ts...), PR(Ps...)> {
    // if sizeof...(Ts) == sizeof...(Ps)
    static constexpr bool value = match<TR,PR>::value && meta::all({ match<Ts,Ps>::value... });
    using type = meta::concat< func_match<typename match<TR,PR>::type>, matched_types<typename match<Ts,Ps>::type...> >;


    template <typename X>
    struct pass_impl {
        using type = meta::error;
    };

    template <typename R, typename... Xs> 
    struct pass_impl< func_match<R, Xs...> > {
        using type = typename pattern<PR>::template pass<R> (typename pattern<Ps>::template pass<Xs>...); 
    };

    template <typename X>
    using pass = typename pass_impl<X>::type;
};


template <
    template<typename...> class C,
    typename... Ts,
    typename... Ps
>
struct match <C<Ts...>, C<Ps...>> {
    // otherwise
    template <typename T, typename P, class Enabled=void>
    struct choose {
        static constexpr bool value = false;
        using type = meta::error;

        template <typename X>
        using pass = meta::error;
    };

    // if sizeof...(Ts) == sizeof...(Ps)
    template <typename... Types, typename... Patterns>
    struct choose< 
        C<Types...>, C<Patterns...>, 
        std::enable_if_t<(
            ( sizeof...(Types) == sizeof...(Patterns) ) 
            &&
            !meta::contains<meta::typelist<Patterns...>, ___>::value
        )> 
    > {
        static constexpr bool value = meta::all({ match<Ts,Ps>::value ... });
        using type = matched_types< typename match<Ts,Ps>::type ... >;

        template <typename... Xs>
        using pass = meta::typelist< typename pattern<Ps>::template pass<Xs>... >;
    };

    // else, if Patterns list ends with ___ [variadic wildcard]
    template <typename... Types, typename... Patterns>
    struct choose<C<Types...>, C<Patterns...>, 
    std::enable_if_t< 
        (sizeof...(Types) >= sizeof...(Patterns)-1) &&
        std::is_same<meta::type_at<sizeof...(Patterns)-1, Patterns...>, ___>::value 
    >> {
        // enum{ value = meta::all({ match<Ts,Ps>::value ... }) };
        static constexpr auto N_Ps = sizeof...(Patterns);

        using list = meta::zip_with< 
            meta::take<N_Ps, meta::append<meta::take<N_Ps-1, matched_types<Types...>>, meta::drop<N_Ps-1, variadic<Types...>> >>,
            meta::typelist<Patterns...>,
            match
        >;

        using type = meta::transform< 
            meta::extract_type,
            list
        >;

        static constexpr bool value = meta::all<list>(); ///TODO: Fix this!
    };


    static constexpr bool value = choose<C<Ts...>, C<Ps...>>::value;
    using type = typename choose<C<Ts...>, C<Ps...>>::type;
    
};

} // namespace core