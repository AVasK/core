//
//  vswitch.hpp
//  VariadicSwitch
//
//  Created by Alex Vaskov on 16.04.2022.
//
#pragma once

template <class T, T Case, typename F>
struct Case_ {
    F f;
};

template <class T, T Case, typename F>
auto case_(F && f) {
    return Case_<T, Case, F>{ std::forward<F>(f) };
}


template <typename R, typename T>
R variadic_switch_impl (T value) { __builtin_unreachable(); }


template <typename R, typename T, T Case, T... Cases, typename F, typename... Fs>
auto variadic_switch_impl (T value, F&& f, Fs&&... fs) {
    if ( value == Case ) { return std::forward<F>(f)(); }
    else return variadic_switch_impl<R, T, Cases...>(value, std::forward<Fs>(fs)... );
//    int ret;
//    std::initializer_list<int> ({(value == Cases ? (ret = fs()),0 : 0)...});
//    return ret;
}


template <
    typename R,
    typename T,

    T Case1,
    T Case2,
    T Case3,
    T Case4,
    T... Cases,
    
    typename F1,
    typename F2,
    typename F3,
    typename F4,
    typename... Fs
>
auto variadic_switch_impl (T value, F1&& f1, F2&& f2, F3&& f3, F4&& f4, Fs&&... fs) {
    switch( value )
    {
        case Case1: return std::forward<F1>(f1)(); break;
        case Case2: return std::forward<F2>(f2)(); break;
        case Case3: return std::forward<F3>(f3)(); break;
        case Case4: return std::forward<F4>(f4)(); break;
    }
    return variadic_switch_impl<R, T, Cases...>(value, std::forward<Fs>(fs)... );
}

template <typename T, T Case, T... Cases, typename F, typename... Fs>
auto switch_(T value, Case_<T, Case, F> case1, Case_<T, Cases, Fs>... cases) {
    using R = decltype( case1.f() );
    return variadic_switch_impl<R, T, Case, Cases...>(value, case1.f, cases.f ...);
}




