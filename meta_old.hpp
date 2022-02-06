#pragma once

#include <cstddef>
#include <type_traits>

// TODO: replace with actual macro from cc_macros.hpp
#define X_CLANG 1

#if X_GCC || X_CLANG
#define META_UNREACHABLE __builtin_unreachable()
#elif defined( X_MSVC )
#define META_UNREACHABLE __assume(0)
#endif

#define MIN_CPP_VERSION(expr) \
static_assert(__cplusplus/100 >= expr, "C++ version " #expr " is required")

namespace meta {

namespace constants {
    static constexpr size_t not_found = -1;
    static constexpr size_t ambiguous = not_found - 1;
};

inline namespace types {
    struct Nothing {};
};


template <typename Ret, typename... Ts>
using c_func_ptr = Ret (*)(Ts...);


template <typename T>
struct overload_for {
    constexpr T operator()(T) noexcept;// {};
};

template <typename... Ts>
struct overloads_for : overload_for<Ts>... {
    // void operator()();
    // using _overload<Ts>::operator()...;
};

template <typename T, typename... Ts>
struct best_match {
    // using type = typename std::result_of< overloads_for<Ts...>(T) >::type;
    using type = decltype(
        std::declval< overloads_for<Ts...> >()( std::declval<T>() )
    ); 
    // using type = std::result_of_t< _overloads<Ts...>(T)>;
};
// Need also best_match_with_predicate:
// e.g.: _overload::operator()(T) -> predicate<T>
// like in _check_for_narrowing 

template <typename V, typename... Ts>
using best_match_t = typename best_match<V, Ts...>::type;


// ALL
template <bool... Vs>
constexpr bool all() noexcept {
    constexpr bool flag[] = {Vs...};
    for (size_t i = 0; i < sizeof...(Vs); ++i) {
        if (!flag[i]) { return false; }
    }
    return true;
}

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


// SELECT
template <bool Condition, typename Then, typename Otherwise>
struct select;

template <typename Then, typename Otherwise>
struct select<true, Then, Otherwise> {
    using type = Then;
};

template <typename Then, typename Otherwise>
struct select<false, Then, Otherwise> {
    using type = Otherwise;
};

template <bool Condition, typename Then, typename Otherwise>
using select_t = typename select<Condition, Then, Otherwise>::type;

template <size_t I>
struct num_t {
    enum {value = I};
};

template <typename T>
using Identity = T;

template <template<typename...> class F, typename... Args>
struct defer {
    using evaluate = F<Args...>;
};

namespace detail {
    template <template<size_t,typename...> class F, size_t I, typename... Args>
    struct deferred_get_type {
        using evaluate = typename F<I,Args...>::type;
    };
}

// METAFUNC
template <typename F, typename... Args>
using apply = typename F::template call<Args...>;

template <
    class F1, 
    class F2
>
struct compose {
    template <typename... Ts>
    using call = apply< F1, apply<F2,Ts...> >;
};

template <class F, typename ...Args>
struct lbind {
    template <typename... Ts>
    using call = apply<F, Args..., Ts...>;
};

template <class F, typename ...Args>
struct rbind {
    template <typename... Ts>
    using call = apply<F, Ts..., Args...>;
};



// AT_INDEX
namespace detail {
    template<size_t I, typename T, typename... Ts>
    struct get_type_at {
        // static_assert(I < sizeof...(Ts)+1, "Index out of bounds");
        using type = typename get_type_at<I-1, Ts...>::type;
    };

    template <typename T, typename... Ts>
    struct get_type_at<0, T, Ts...> {
        using type = T;
    };

    template<size_t I, typename... Ts>
    struct try_get_type_at {
        using type = typename select_t<
            (I >= sizeof...(Ts)),
            defer<Identity, Nothing>,
            detail::deferred_get_type<get_type_at, I, Ts...>
        >::evaluate;
    };
}

template<size_t I, typename... Ts>
using type_at = typename detail::get_type_at<I,Ts...>::type;

template<size_t I, typename... Ts>
using maybe_type_at = typename detail::try_get_type_at<I,Ts...>::type;


// INDEX_OF
template <typename V, typename... Ts>
constexpr size_t index_of() noexcept {
    static_assert( sizeof...(Ts) > 0, "Empty type list for index_of" );
    constexpr bool matches[] = { std::is_same<V,Ts>::value... };
    for (size_t i=0; i<sizeof...(Ts); ++i) {
        if (matches[i]) return i;
    }
    return constants::not_found;
}

template <typename X, typename... Ts>
constexpr size_t unambiguous_index_of() noexcept {
    constexpr bool matches[] = { std::is_same<X,Ts>::value... };
    size_t match = constants::not_found;
    for (size_t i=0; i<sizeof...(Ts); ++i) {
        if (matches[i]) {
            if (match != constants::not_found) {
                return constants::ambiguous;
            }
            match = i;
        }
    }
    return match;
}
// TODO: Sfinae unambiguous match

// Need a way to get type at index

// sequence
template <typename T=size_t, T... Is>
struct numeric_sequence {};

template <typename T, size_t N, size_t... Is>
struct make_sequence : make_sequence<T, N-1, N-1, Is...> {};

template <typename T, size_t... Is>
struct make_sequence<T,0,Is...> : numeric_sequence<T, Is...> {};

template <size_t N, typename T=size_t>
using sequence_of = make_sequence<T,N>;

namespace detail {
    template <class T, typename F, int... Is>
    auto for_each_helper(T&& t, F&& f, numeric_sequence<size_t,Is...>) {
        auto res = { (std::get<Is>(std::forward<T>(t)), 0)... };
        return res;
    }
}

template<typename... Ts, typename F>
auto for_each(std::tuple<Ts...> const& t, F f)
{
    return detail::for_each_helper(t, f, sequence_of<sizeof...(Ts)>());
}

// INVOKE_ALL, first_match
template <typename... Fs>
struct Functions {
    std::tuple<Fs...> group;

    constexpr Functions(Fs&&... fs) 
    : group {std::forward<Fs>(fs)...} 
    {}

    template <size_t I>
    constexpr auto get() const {//-> type_at<I,Fs...> {
        return std::get<I>(group);
    }

    template <typename... Args>
    static constexpr size_t find_invocable_with() noexcept {
        constexpr bool match[] = { std::is_invocable_v<Fs, Args...>... };
        for (size_t i=0; i<sizeof...(Fs); ++i) {
            if (match[i]) {
                return i;
            }
        }
        return constants::not_found;
    }

    // invoke first matching function
    template <typename... Args>
    auto operator() (Args&& ... args) const {
        constexpr size_t Idx = Functions::find_invocable_with<Args...>();
        return this->get<Idx>()( std::forward<Args>(args)... );
    }

    // template <typename... Args>
    // constexpr decltype(auto) operator() (Args&&... args) const {

    // }

    template <typename... Args>
    constexpr auto invoke_all (Args&&... args) 
    {
        return for_each(group, [&](auto& f){ return f(std::forward<Args>(args)...); } );
    }
};
template <typename... Fs>
auto functions(Fs&&... fs) -> Functions<Fs...> {
    return {std::forward<Fs>(fs)...};
}

// OVERLOAD
#if __cplusplus/100 >= 2017
// C++17:
template <typename... Fs>
struct Overload : Fs... {
    MIN_CPP_VERSION(2017);
    using Fs::operator()...;
};

#else 
// C++11
template <typename F, typename... Fs>
struct Overload : F, Overload<Fs...> {
    MIN_CPP_VERSION(2011);
    using F::operator();
    using Overload<Fs...>::operator();

    F f;
    Overload (F _f, Fs... _fs) : F(_f), f{_f}, Overload<Fs...>(_fs...) {}
};

template <typename F> struct Overload<F> : F {
    using F::operator();

    F f;
    Overload(F _f) :  F(_f), f{_f} {}
};
#endif

template <typename... Fs>
auto overload(Fs&&... fs) -> Overload<Fs...> {
    return {std::forward<Fs>(fs)...};
}


// RUNTIME MATCH 
namespace detail {
    template <typename F, size_t I>
    struct tagged_function {
        F _f;
        
        tagged_function(F f) noexcept : _f {f} 
        {}

        template <typename... Args>
        auto call(size_t tag, Args&&... args) const
        -> decltype( _f(std::forward<Args>(args)...) )
        {
            if ( tag == I ) {
                std::cout << " tag: " << tag << " == " << I << "\n";
                return _f(std::forward<Args>(args)...);
            //} else {
                //META_UNREACHABLE;
            }
        } 
    };
}

template <typename... Fs>
struct tag_dispatch 
: detail::tagged_function<Fs, index_of<Fs,Fs...>()>...
{
    tag_dispatch (Fs... fs) 
    : detail::tagged_function<Fs, index_of<Fs,Fs...>()> (fs)...
    {}

    template <
        typename... Args,
        typename R = decltype( std::declval<type_at<0,Fs...>>()( std::declval<Args>()... ) )
    >
    constexpr decltype(auto) operator() (size_t tag, Args&&... args) const
    //noexcept( noexcept(detail::tagged_function<Fs,index_of<Fs,Fs...>()>::template call<Args>(args...))...)
    // -> R
    {
        R val[] = {detail::tagged_function<Fs,index_of<Fs,Fs...>()>::template call<Args...>(tag, std::forward<Args>(args)...)...};
        
        std::cout << "[ ";
        for (int i=0; i < sizeof...(Fs); ++i) {
            std::cout << val[i] << " ";
        }
        std::cout << "]\n";

        return val[tag];
    }
};

template <typename... Fs>
auto dispatch( Fs... fs ) -> tag_dispatch<Fs...> {
    return {fs...};    
}


} // meta namespace
