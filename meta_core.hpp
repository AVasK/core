#pragma once

#include <type_traits>
#include <initializer_list>

#define metafunc template<class...> class

namespace meta {


template <bool Cond, typename T=void>
struct enable_if {};

template <typename T>
struct enable_if<true,T> {
    using type = T;
};

template <bool Cond>
using require = typename std::enable_if<Cond>::type;


template <typename T>
struct identity {
    using type = T;
};


struct nothing {};


template <typename T>
using extract_type = typename T::type;


template <typename T>
struct always_true : std::true_type {};


// ===== typelist =====
template <typename... Ts>
struct typelist;// {}; // We don't even need to actually define it :)

namespace detail {

    template <typename... Ts>
    struct head_impl;

    template <typename T, typename... Ts>
    struct head_impl<T,Ts...> {
        using type = T;
    };

    template <>
    struct head_impl<> {
        using type = nothing;
    };


    template <typename... Ts>
    struct tail_impl;

    template <typename T, typename... Ts>
    struct tail_impl<T,Ts...> {
        using type = typelist<Ts...>;
    };

    template <>
    struct tail_impl<> {
        using type = typelist<>;
    };
}

template <typename... Ts>
using head = typename detail::head_impl<Ts...>::type;

template <typename... Ts>
using tail = typename detail::tail_impl<Ts...>::type;


// ===== select =====
namespace detail {
    template <bool Condition, class ThenType, class ElseType>
    struct select_impl {
        using type = ElseType;
    };

    template <class ThenType, class ElseType>
    struct select_impl<true, ThenType, ElseType> {
        using type = ThenType;
    };
}

template <bool Cond, typename T, typename E>
using select = typename detail::select_impl<Cond, T, E>::type;

template <class Cond, typename T, typename E>
using select_if = typename detail::select_impl<bool(Cond::value), T,E>::type;


// ===== defer =====
namespace detail {
    template <metafunc F, typename... Args>
    struct defer_impl {
        using eval = F<Args...>;
        using type = F<Args...>;
    };
}

template <metafunc F, typename... Args>
using defer = detail::defer_impl<F, Args...>;


// ===== type_at =====
namespace detail {
    template <size_t Index, typename... Ts>
    struct type_at_impl {
        ///TODO: Implement a non-naive solution for compilers other than Clang
    };
}

#if defined CORE_CLANG && __has_builtin(__type_pack_element)
template <size_t Index, typename... Ts>
using type_at = __type_pack_element<Index, Ts...>;
#else 
template <size_t Index, typename... Ts>
using type_at = typename detail::type_at_impl<Index, Ts...>::type;
#endif

// ===== quote =====
template <metafunc F>
struct quote {
    template <typename... Args>
    using fn = F<Args...>;
};

// ===== invoke =====
template <class Q, typename... Args>
using invoke = typename Q::template fn<Args...>;


// ===== bind =====
template <metafunc F, typename... Ts>
struct bind_first {
    template <typename... Us>
    using with = F<Ts..., Us...>;

    template <typename... Us>
    using fn = F<Ts..., Us...>; // to support interoperability with other quoted metafunctions
};

template <metafunc F, typename T>
using bind = bind_first<F,T>;


// ===== chain =====
// template <metafunc... Fs>
// struct chain;

// template <metafunc F1, metafunc F2, metafunc... Fs>
// struct chain<F1, F2, Fs...> {
//     template <typename... Args>
//     using apply = typename chain<F2, Fs...>::template apply< F1<Args...> >;
// };

// template <metafunc F>
// struct chain<F> {
//     template <typename... Args>
//     using apply = typename meta::defer<F, Args...>::type;
// };

// chain<F1, F2, F3> -> F1<F2<F3>>


template <size_t N>
struct number {
    enum{ value = N };
};


template <size_t N>
struct tag : std::integral_constant<size_t, N> {};


// prepend
namespace detail {
    template <class List, typename... T>
    struct prepend_impl;

    template <
        template<typename...> class List, typename... Ts,
        typename... T
    >
    struct prepend_impl<List<Ts...>, T...> {
        using type = List<T...,Ts...>;
    };
}

template <class List, typename... T>
using prepend = typename detail::prepend_impl<List,T...>::type;


// append
namespace detail {
    template <class List, typename... T>
    struct append_impl;

    template <
        template<typename...> class List, typename... Ts,
        typename... T
    >
    struct append_impl<List<Ts...>, T...> {
        using type = List<Ts..., T...>;
    };
}

template <class List, typename... T>
using append = typename detail::append_impl<List,T...>::type;


// map
namespace detail {
    template <metafunc F, class List>
    struct apply_impl;

    template <metafunc F, metafunc List, typename... Ts>
    struct apply_impl<F, List<Ts...>> {
        using type = F<Ts...>;
    };
}

template <metafunc F, class List>
using apply = typename detail::apply_impl<F, List>::type;


// filter_p (filter with predicate)
namespace detail {
    template <class List, class Predicate>
    struct filter_p_impl;

    template<metafunc List, typename... Ts, class Predicate>
    struct filter_p_impl<List<Ts...>, Predicate> {
        using type = select< 
            Predicate::template eval< head<Ts...> >(), 
            prepend< typename filter_p_impl<apply<List, tail<Ts...>>, Predicate>::type, head<Ts...> >,
            typename filter_p_impl<apply<List, tail<Ts...>>, Predicate>::type
        >;
    };

    template<metafunc List, class Predicate>
    struct filter_p_impl<List<>, Predicate> {
        using type = List<>;
    };
}

template<class List, class Predicate>
using filter_p = typename detail::filter_p_impl<List, Predicate>::type;


//transform
namespace detail {
    template <metafunc F, class List>
    struct transform_impl;

    template <metafunc F, metafunc List, typename... Ts>
    struct transform_impl< F, List<Ts...> >{
        using type = List< F<Ts>... >;
    };
}

template <metafunc F, class List>
using transform = typename detail::transform_impl<F,List>::type;


// zip
namespace detail {
    template <class List1, class List2>
    struct zip_impl;

    template <metafunc List1, typename... T1s, metafunc List2, typename... T2s>
    struct zip_impl <List1<T1s...>, List2<T2s...>>{
        using type = typelist< typelist<T1s, T2s>... >;
    };
}

template <class List1, class List2>
using zip = typename detail::zip_impl<List1, List2>::type;


// zip_with
namespace detail {
    template <class List1, class List2, metafunc F>
    struct zip_with_impl;

    template <metafunc List1, typename... T1s, metafunc List2, typename... T2s, metafunc F>
    struct zip_with_impl <List1<T1s...>, List2<T2s...>, F>{
        using type = typelist< F<T1s, T2s>... >;
    };
}

template <class List1, class List2, metafunc F>
using zip_with = typename detail::zip_with_impl<List1, List2, F>::type;


// concat
namespace detail {
    template <class... Lists>
    struct concat_impl;

    template <
        metafunc L1, typename... T1s,
        metafunc L2, typename... T2s,
        class... Lists
    >
    struct concat_impl< L1<T1s...>, L2<T2s...>, Lists... >{
        using type = typename concat_impl< L1<T1s..., T2s...>, Lists... >::type;
    };

    template <metafunc List, typename... Ts>
    struct concat_impl< List<Ts...> >{
        using type = List<Ts...>;
    };

    template <>
    struct concat_impl<> {
        using type = typelist<>;
    };
}

template <class... Lists>
using concat = typename detail::concat_impl<Lists...>::type;


//repeat
namespace detail {
    template <size_t N, typename... Ts>
    struct repeat_impl {
        using l = typename repeat_impl<N/2, Ts...>::type;
        using r = typename repeat_impl<(N-N/2), Ts...>::type;
        using type = concat<l, r>;
    };

    template <typename... Ts>
    struct repeat_impl<0,Ts...> {
        using type = typelist<>;
    };

    template <typename... Ts>
    struct repeat_impl<1,Ts...> {
        using type = typelist<Ts...>;
    };
}

template <size_t N, typename... Ts>
using repeat = typename detail::repeat_impl<N,Ts...>::type;



// set_contains
namespace detail {
    template <class List, class Value>
    struct set_contains_impl;

    template <
        template <class...> class List,
        typename... Ts,
        class Value
    >
    struct set_contains_impl< List<Ts...>, Value > {
        struct L : identity<Ts>... {};
        using type = std::is_base_of<identity<Value>, L>;
    };
}

template <class List, class Value>
using set_contains = typename detail::set_contains_impl<List, Value>::type;


// sum
constexpr size_t sum() noexcept { return 0; }

template <typename T, typename... Ts>
constexpr size_t sum(T v, Ts... vs) noexcept {
    return v + sum(vs...);
}


#if __cplusplus/100 >= 2014
// Any & All boolean funcs beta [bound to change]
// ALL
// template <bool... Vs>
// constexpr bool all() noexcept {
//     constexpr bool flag[] = {Vs...};
//     for (size_t i = 0; i < sizeof...(Vs); ++i) {
//         if (!flag[i]) { return false; }
//     }
//     return true;
// }

constexpr bool all(std::initializer_list<bool> vs) noexcept {
    for (auto&& flag : vs) {
        if (!flag) { return false; }
    }
    return true;
}


// ANY
template <bool... Vs>
constexpr bool any() noexcept {
    constexpr bool flag[] = {Vs...};
    for (size_t i = 0; i < sizeof...(Vs); ++i) {
        if (flag[i]) { return true; }
    }
    return false;
}

constexpr bool any(std::initializer_list<bool> vs) noexcept {
    for (auto&& flag : vs) {
        if (flag) { return true; }
    }
    return false;
}
#endif

#undef metafunc
} //namespace meta
