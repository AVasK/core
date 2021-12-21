#pragma once

#include <iostream>
#include <type_traits>
#include <cctype> // isspace
#include "compiler_detect.hpp"
#include "macros.hpp"
#include "meta_core.hpp"

#if defined CORE_GCC || defined CORE_CLANG
#   define PRETTY_FUNC __PRETTY_FUNCTION__
#elif defined CORE_MSVC
#   define PRETTY_FUNC __FUNCSIG__
#endif


#if __cplusplus/100 >= 2014
#include "type_predicates.hpp"
#endif


namespace core {
inline namespace typesystem {
    

template <typename T>
struct TypeOf {
    using type = T;

    constexpr auto remove_cv() const noexcept -> TypeOf<typename std::remove_cv<type>::type> { return {}; }
    constexpr auto add_cv() const noexcept -> TypeOf<typename std::add_cv<type>::type> { return {}; }

    constexpr auto remove_const() const noexcept -> TypeOf<typename std::remove_const<type>::type> { return {}; }
    constexpr auto add_const() const noexcept -> TypeOf<typename std::add_const<type>::type> { return {}; }

    constexpr auto remove_volatile() const noexcept -> TypeOf<typename std::remove_volatile<type>::type> { return {}; }
    constexpr auto add_volatile() const noexcept -> TypeOf<typename std::add_volatile<type>::type> { return {}; }

    constexpr auto remove_reference() const noexcept -> TypeOf<typename std::remove_reference<type>::type> {return {};}
    constexpr auto add_lvalue_reference() const noexcept -> TypeOf<typename std::add_lvalue_reference<type>::type> { return {}; }
    constexpr auto add_rvalue_reference() const noexcept -> TypeOf<typename std::add_rvalue_reference<type>::type> { return {}; }
    // common shortcuts
    constexpr auto remove_ref() const noexcept -> TypeOf<typename std::remove_reference<type>::type> {return {};}
    constexpr auto add_lvalue_ref() const noexcept -> TypeOf<typename std::add_lvalue_reference<type>::type> { return {}; }
    constexpr auto add_rvalue_ref() const noexcept -> TypeOf<typename std::add_rvalue_reference<type>::type> { return {}; }

    constexpr auto remove_pointer() const noexcept -> TypeOf<typename std::remove_pointer<type>::type> { return {}; }
    constexpr auto add_pointer() const noexcept -> TypeOf<typename std::add_pointer<type>::type> { return {}; }
    // common shortcuts
    constexpr auto remove_ptr() const noexcept -> TypeOf<typename std::remove_pointer<type>::type> { return {}; }
    constexpr auto add_ptr() const noexcept -> TypeOf<typename std::add_pointer<type>::type> { return {}; }

    constexpr auto decay() const noexcept -> TypeOf<typename std::decay<type>::type> { return {}; }

    constexpr auto raw() const noexcept -> TypeOf<typename std::remove_reference<typename std::remove_cv<type>::type>::type> { return {}; }


    #if __cplusplus/100 >= 2014
    template <class P,
        typename=typename std::enable_if<std::is_base_of<core::detail::TypePredicate, P>::value>::type 
    >
    constexpr bool satisfies(P) const noexcept {
        return P::template eval<type>();
    }

    template <class P, 
        typename=typename std::enable_if<std::is_base_of<core::detail::TypePredicate, P>::value>::type 
    >
    constexpr bool operator() (P) const noexcept {
        return P::template eval<type>();
    }
    #endif

    // TODO: Add conversion to string
    // operator std::string () {
    //     return type_name<T>();
    // }
};


#if __cplusplus/100 >= 2014
template <typename T>
CORE_CPP17_INLINE_VARIABLE
constexpr auto Type = TypeOf<T>{};
#endif


template <typename T>
constexpr 
auto type(T&&) noexcept -> TypeOf<T&&> { return {}; };


template <class T1, class T2>
constexpr 
bool operator== (TypeOf<T1>, TypeOf<T2>) noexcept {
    return std::is_same<T1,T2>::value;
}

template <class T1, class T2>
constexpr 
bool operator!= (TypeOf<T1> t1, TypeOf<T2> t2) noexcept {
    return !(t1 == t2);
}



// Types
template <typename... Ts>
struct TypeList {

    constexpr auto remove_cv() const noexcept -> TypeList<typename std::remove_cv<Ts>::type...> { return {}; }
    constexpr auto add_cv() const noexcept -> TypeList<typename std::add_cv<Ts>::type...> { return {}; }

    constexpr auto remove_const() const noexcept -> TypeList<typename std::remove_const<Ts>::type...> { return {}; }
    constexpr auto add_const() const noexcept -> TypeList<typename std::add_const<Ts>::type...> { return {}; }

    constexpr auto remove_volatile() const noexcept -> TypeList<typename std::remove_volatile<Ts>::type...> { return {}; }
    constexpr auto add_volatile() const noexcept -> TypeList<typename std::add_volatile<Ts>::type...> { return {}; }

    constexpr auto remove_reference() const noexcept -> TypeList<typename std::remove_reference<Ts>::type...> {return {};}
    constexpr auto add_lvalue_reference() const noexcept -> TypeList<typename std::add_lvalue_reference<Ts>::type...> { return {}; }
    constexpr auto add_rvalue_reference() const noexcept -> TypeList<typename std::add_rvalue_reference<Ts>::type...> { return {}; }
    // common shortcuts
    constexpr auto remove_ref() const noexcept -> TypeList<typename std::remove_reference<Ts>::type...> {return {};}
    constexpr auto add_lvalue_ref() const noexcept -> TypeList<typename std::add_lvalue_reference<Ts>::type...> { return {}; }
    constexpr auto add_rvalue_ref() const noexcept -> TypeList<typename std::add_rvalue_reference<Ts>::type...> { return {}; }

    constexpr auto remove_pointer() const noexcept -> TypeList<typename std::remove_pointer<Ts>::type...> { return {}; }
    constexpr auto add_pointer() const noexcept -> TypeList<typename std::add_pointer<Ts>::type...> { return {}; }
    // common shortcuts
    constexpr auto remove_ptr() const noexcept -> TypeList<typename std::remove_pointer<Ts>::type...> { return {}; }
    constexpr auto add_ptr() const noexcept -> TypeList<typename std::add_pointer<Ts>::type...> { return {}; }

    constexpr auto decay() const noexcept -> TypeList<typename std::decay<Ts>::type...> { return {}; }

    constexpr auto raw() const noexcept -> TypeList<typename std::remove_reference<typename std::remove_cv<Ts>::type>::type...> { return {}; }


    // transformations on types
    constexpr auto head() const noexcept -> TypeOf< meta::head<Ts...> > { return {}; } 

    constexpr auto tail() const noexcept -> meta::apply<TypeList, meta::tail<Ts...>> { return {}; }

    template <template <typename> class MetaFunc>
    constexpr auto transform() const noexcept -> TypeList<MetaFunc<Ts>...> { return {}; }

    // template <template <typename> class MetaFunc>
    // constexpr auto filter() const noexcept -> TypeList<meta::filter<MetaFunc, meta::typelist<Ts...>> { return {}; }

    #if __cplusplus/100 >= 2014
    
    template <class P, 
        typename=typename std::enable_if_t<std::is_base_of<core::detail::TypePredicate, P>::value> 
    >
    constexpr bool all (P) const noexcept {
        return meta::all({ P::template eval<Ts>()... });
    }

    
    template <class P, 
        typename=typename std::enable_if_t<std::is_base_of<core::detail::TypePredicate, P>::value> 
    >
    constexpr bool any (P) const noexcept {
        return meta::any({ P::template eval<Ts>()... });
    }


    template <class P, 
        typename=typename std::enable_if_t<std::is_base_of<core::detail::TypePredicate, P>::value> 
    >
    constexpr bool none (P) const noexcept {
        return !meta::any({ P::template eval<Ts>()... });
    }

    #endif
};

#if __cplusplus/100 >= 2014
template <typename... Ts>
CORE_CPP17_INLINE_VARIABLE
constexpr auto Types = TypeList<Ts...>{};
#endif



// Printing the TypeOf<T>
namespace detail {

template <size_t N>
inline
constexpr size_t slen(const char (&str) [N]) {
    return N;
}

#if __cplusplus/100 >= 2014
inline
namespace impl_cpp14{
    constexpr size_t pslen(const char * const ps) {
        for (size_t i=0; ;++i) {
            if (ps[i] == '\0') return i;
        }
    }
}
#elif __cplusplus/100 >= 2011
inline
#endif
namespace impl_cpp11{
    constexpr size_t pslen(const char* const ps, size_t idx=0) {
        return (ps[idx] == '\0') ?
        idx : pslen(ps, idx+1);
    }
}

template <size_t N>
constexpr size_t findChar(const char (&str)[N], char c, size_t idx=0) {
    return (str[idx] == c || idx >= N) ?
    idx
    :
    findChar(str, c, idx+1);
        
}

template <size_t N>
constexpr size_t skipSpace(const char (&str)[N], size_t idx=0) {
    return (!std::isspace( (int)str[idx] )) ?
        idx
        :
        skipSpace(str, idx+1);
}


struct CSlice {
    
    constexpr CSlice (const char * const cstr)
    : from {0}
    , to  {pslen(cstr)}
    , str {cstr}
    {}
    
    constexpr char operator[] (size_t i) const {
        return str[i];
    }
    
    friend
    std::ostream& operator<< (std::ostream& os, CSlice slice) {
        for (size_t i=slice.from; i<slice.to; ++i) {
            os << slice[i];
        }
        return os;
    }
    
    size_t from;
    size_t to;
    const char * const str;
};


template <typename T>
inline 
CORE_CPP14_CONSTEXPR_FUNC
auto type_name() -> CSlice {
    auto&& prettyName = PRETTY_FUNC;
    // return prettyName;
    auto name = CSlice( PRETTY_FUNC );
    auto sq_brace = findChar(prettyName, '[');
    auto T_pos = findChar(prettyName, 'T', sq_brace);
    auto eq_sign_pos = findChar(prettyName, '=', T_pos);
    auto type_pos = skipSpace(prettyName, eq_sign_pos+1);
    name.from = type_pos;// + slen("T = ");
    name.to   = slen(PRETTY_FUNC) - 2;
    return name;
}

}//namespace detail


template <typename T>
inline
auto operator<< (std::ostream& os, TypeOf<T> type) -> std::ostream& {
    os << detail::type_name<T>();
    return os;
}



template <typename T, typename... Ts>
inline
auto operator<< (std::ostream& os, TypeList<T, Ts...> types) -> std::ostream& {
#if __cplusplus/100 >= 2017
    if constexpr (sizeof...(Ts) > 0) {
        os << detail::type_name<T>() << ", " << Types<Ts...>;
    } else {
        os << detail::type_name<T>();
    }
#else
    os << detail::type_name<T>() << ", " << Types<Ts...>;
#endif
    return os;
}

#if __cplusplus/100 < 2017
inline
auto operator<< (std::ostream& os, TypeList<> types) -> std::ostream& {
    return os;
}
#endif


template <typename... Ts, typename... Us>
inline
constexpr auto operator+ (TypeList<Ts...>, TypeList<Us...>) -> TypeList<Ts..., Us...> {
    return {};
}


#if defined PRETTY_FUNC
#   undef PRETTY_FUNC
#endif

}//namespace typesystem
}//namespace core