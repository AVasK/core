#pragma once
#include <type_traits>
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

struct nothing;


template <typename... Ts>
struct typelist;// {}; // We don't even need to actually define it :)

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


template <typename... Ts>
using head = typename head_impl<Ts...>::type;

template <typename... Ts>
using tail = typename tail_impl<Ts...>::type;


// select
template <bool Condition, class ThenType, class ElseType>
struct select_impl {
    using type = ElseType;
};

template <class ThenType, class ElseType>
struct select_impl<true, ThenType, ElseType> {
    using type = ThenType;
};

template <bool Cond, typename T, typename E>
using select = typename select_impl<Cond, T, E>::type;

template <class Cond, typename T, typename E>
using select_if = typename select_impl<bool(Cond::value), T,E>::type;


// defer
template <metafunc F, typename... Args>
struct defer_impl {
    using eval = F<Args...>;
};

template <metafunc F, typename... Args>
using defer = defer_impl<F, Args...>;


// prepend
template <class List, typename T>
struct prepend_impl;

template <
    template<typename...> class List, typename... Ts,
    typename T
>
struct prepend_impl<List<Ts...>, T> {
    using type = List<T,Ts...>;
};

template <class List, typename T>
using prepend = typename prepend_impl<List,T>::type;

// append
template <class List, typename T>
struct append_impl;

template <
    template<typename...> class List, typename... Ts,
    typename T
>
struct append_impl<List<Ts...>, T> {
    using type = List<Ts..., T>;
};

template <class List, typename T>
using append = typename append_impl<List,T>::type;


// map
template <metafunc F, class List>
struct apply_impl;

template <metafunc F, metafunc List, typename... Ts>
struct apply_impl<F, List<Ts...>> {
    using type = F<Ts...>;
};

template <metafunc F, class List>
using apply = typename apply_impl<F, List>::type;


//transform
template <metafunc F, class List>
struct map_impl;

template <metafunc F, metafunc List, typename... Ts>
struct map_impl< F, List<Ts...> >{
    using type = List< F<Ts>... >;
};

template <metafunc F, class List>
using map = typename map_impl<F,List>::type;

// maybe move impl to separate namespaces?
// mb even versioned for different cpp_stds?


template <class List1, class List2>
struct zip_impl;

template <metafunc List1, typename... T1s, metafunc List2, typename... T2s>
struct zip_impl <List1<T1s...>, List2<T2s...>>{
    using type = typelist< typelist<T1s, T2s>... >;
};

template <class List1, class List2>
using zip = typename zip_impl<List1, List2>::type;


template <class List1, class List2, metafunc F>
struct zip_with_impl;

template <metafunc List1, typename... T1s, metafunc List2, typename... T2s, metafunc F>
struct zip_with_impl <List1<T1s...>, List2<T2s...>, F>{
    using type = typelist< F<T1s, T2s>... >;
};

template <class List1, class List2, metafunc F>
using zip_with = typename zip_with_impl<List1, List2, F>::type;


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

template <class... Lists>
using concat = typename concat_impl<Lists...>::type;


template <class List, class Value>
struct set_contains_impl;

template <class List, class Value>
using set_contains = typename set_contains_impl<List, Value>::type;

template <
    template <class...> class List,
    typename... Ts,
    class Value
>
struct set_contains_impl< List<Ts...>, Value > {
    struct L : identity<Ts>... {};
    using type = std::is_base_of<identity<Value>, L>;
};

constexpr size_t sum() noexcept { return 0; }

template <typename T, typename... Ts>
constexpr size_t sum(T v, Ts... vs) noexcept {
    return v + sum(vs...);
}


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


#undef metafunc
} //namespace meta
