#pragma once 

#include <iostream>
#include <type_traits>
#include "meta.hpp"
#include "typesystem/type_predicates.hpp"

namespace core {

namespace pattern_matching {
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
}

namespace detail {
    // Placeholders
    using pattern_matching::_;
    using pattern_matching::___;

    // Auxiliary types
    using pattern_matching::matched_types;
    using pattern_matching::variadic;
    using pattern_matching::func_match;

    template <typename T>
    using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

    template <typename T, class Pattern>
    struct match;

    template <typename P>
    using ptrn = match<P,P>;

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

    template <typename... Ts>
    struct match<variadic<Ts...>, ___&> {
        static constexpr bool value = meta::all({ match<Ts, _&>::value... });
        using type = meta::transform< meta::extract_type, matched_types< match<Ts, _&>... >>;
        template <typename... Xs> using pass = meta::typelist<Xs...>;
    };

    template <typename... Ts>
    struct match<variadic<Ts...>, ___ const> {
        static constexpr bool value = meta::all({ match<Ts, _ const>::value... });
        using type = meta::transform< meta::extract_type, matched_types< match<Ts, _ const>... >>;
        template <typename... Xs> using pass = meta::typelist<Xs...>;
    };

    template <typename... Ts>
    struct match<variadic<Ts...>, ___ const&> {
        static constexpr bool value = meta::all({ match<Ts, _ const&>::value... });
        using type = meta::transform< meta::extract_type, matched_types< match<Ts, _ const&>... >>;
        template <typename... Xs> using pass = meta::typelist<Xs...>;
    };

    template <typename T, typename P>
    struct match <const T, const P> : match<T,P> {
        template <typename X> using pass = typename ptrn<P>::template pass<X> const;
    };

    template <typename T, typename P>
    struct match <T*, P*> : match<T,P> {
        template <typename X> using pass = typename ptrn<P>::template pass<X> *;
    };

    template <typename T, typename P>
    struct match <T&, P&> : match<T,P> {
        template <typename X> using pass = typename ptrn<P>::template pass<X> &;
    };

    template <typename T, typename P>
    struct match <T[], P[]> : match<T,P> {
        template <typename X> using pass = typename ptrn<P>::template pass<X> [];
    };

    template <typename T, typename P>
    struct match <const T[], const P[]> : match<T,P> {
        template <typename X> using pass = const typename ptrn<P>::template pass<X> [];
    };

    template <typename T, typename P, size_t N>
    struct match <T[N], P[N]> : match<T,P> {
        template <typename X> using pass = typename ptrn<P>::template pass<X> [N];
    };

    template <typename T, typename P, size_t N>
    struct match <const T[N], const P[N]> : match<T,P> {
        template <typename X> using pass = const typename ptrn<P>::template pass<X> [N];
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
            using type = typename ptrn<PR>::template pass<R> (typename ptrn<Ps>::template pass<Xs>...); 
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
                !meta::contains<meta::transform<remove_cvref_t, meta::typelist<Patterns...>>, ___>::value
            )> 
        > {
            static constexpr bool value = meta::all({ match<Ts,Ps>::value ... });
            using type = matched_types< typename match<Ts,Ps>::type ... >;

            template <typename... Xs>
            using pass = meta::typelist< typename ptrn<Ps>::template pass<Xs>... >;
        };

        // else, if Patterns list ends with ___ [variadic wildcard]
        template <typename... Types, typename... Patterns>
        struct choose<C<Types...>, C<Patterns...>, 
        std::enable_if_t< 
            (sizeof...(Types) >= sizeof...(Patterns)-1) &&
            std::is_same<remove_cvref_t<meta::type_at<sizeof...(Patterns)-1, Patterns...>>, ___>::value 
        >> {
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

            static constexpr bool value = meta::all<list>();
        };

        static constexpr bool value = choose<C<Ts...>, C<Ps...>>::value;
        using type = typename choose<C<Ts...>, C<Ps...>>::type;
    };

}// namespace detail

namespace pattern_matching {

    template <typename T>
    struct convert_impl {
        using type = TypeList<T>;
    };

    template <template <typename...> class L, typename... Ts>
    struct convert_impl<L<Ts...>> {
        using type = TypeList<Ts...>;
    };

    template <typename T>
    using convert = typename convert_impl<T>::type;

        
    template <typename T, class Pattern>
    using match = detail::match<T,Pattern>;

    template <class P>
    struct Pattern : detail::TypePredicate {
        using token = P;
        using this_pattern = detail::match<P,P>;

        template <typename T>
        constexpr bool matches() const noexcept { return match<T,P>::value; }

        template <typename X>
        constexpr auto unpack() const noexcept { return convert< typename match<X,P>::type >{}; }

        template <typename T>
        constexpr bool eval() const noexcept { return matches<T>(); }

        template <typename T>
        using fn = typename match<T,P>::type;

        template <typename T>
        using substitute = typename detail::match<P,P>::template pass<T>;
    };

    template <class P>
    constexpr auto pattern = Pattern<P>{};

    template <class From, class To>
    struct ConversionPattern : detail::TypeCase {
        template <typename T>
        constexpr static bool test() {
            return From{}.template matches<T>();
                //    To{}.template matches< meta::invoke<From,T> >();
        }

        template <typename T>
        using fn = typename To::template substitute< meta::invoke<From,T> >;
    };


    template <typename T>
    struct is_pattern : std::integral_constant<bool, false> {};

    template <typename P>
    struct is_pattern< Pattern<P> > : std::integral_constant<bool, true> {};


    template <class From, class To, 
        typename Morph = ConversionPattern<From, To>,
        typename=std::enable_if_t<is_pattern<From>::value && is_pattern<To>::value>
    >
    constexpr auto operator>> (From, To) {
        return Morph{};
    }
}

using pattern_matching::pattern;
namespace ptrn = pattern_matching;

} // namespace core