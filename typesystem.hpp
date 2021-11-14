#pragma once

#include <iostream>
#include <type_traits>
#include <cctype> // isspace
#include "compiler_detect.hpp"


#if defined CORE_GCC || defined CORE_CLANG
#   define PRETTY_FUNC __PRETTY_FUNCTION__
#elif defined CORE_MSVC
#   define PRETTY_FUNC __FUNCSIG__
#endif


#if __cplusplus/100 >= 2017
#include "type_predicates.hpp"
#endif


namespace core {
inline namespace typesystem {
    
#if __cplusplus/100 >= 2017
using namespace core::type_predicates;
#endif 

template <typename T>
struct TypeOf {
    using type = T;
    using raw_T = typename std::remove_reference<T>::type;


    constexpr auto remove_ref() const noexcept -> TypeOf<typename std::remove_reference<T>::type> {return {};}


    #if __cplusplus/100 >= 2017
    template <class P,
        typename=typename std::enable_if<std::is_base_of<core::TypePredicate, P>::value>::type 
    >
    constexpr bool satisfies(P) const noexcept {
        return P::template eval<type>();
    }

    template <class P, 
        typename=typename std::enable_if<std::is_base_of<core::TypePredicate, P>::value>::type 
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


#if __cplusplus/100 >= 2017
template <typename T>
inline auto Type = TypeOf<T>{};
#endif


template <typename T>
inline constexpr 
auto type(T&&) noexcept -> TypeOf<T&&> { return {}; };


template <class T1, class T2>
inline constexpr 
bool operator== (TypeOf<T1>, TypeOf<T2>) noexcept {
    return std::is_same<T1,T2>::value;
}

template <class T1, class T2>
inline constexpr 
bool operator!= (TypeOf<T1> t1, TypeOf<T2> t2) noexcept {
    return !(t1 == t2);
}


// Printing the TypeOf<T>
namespace detail {

template <size_t N>
inline
constexpr size_t slen(const char (&str) [N]) {
    return N;
}

inline
constexpr size_t pslen(const char * const ps) {
    for (size_t i=0; ;++i) {
        if (ps[i] == '\0') return i;
    }
}

template <size_t N>
inline
constexpr size_t findChar(const char (&str)[N], char c, size_t idx=0) {
    return (str[idx] == c || idx >= N) ?
    idx
    :
    findChar(str, c, idx+1);
        
}

template <size_t N>
inline
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
constexpr auto type_name() -> CSlice {
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

#if defined PRETTY_FUNC
#   undef PRETTY_FUNC
#endif

}//namespace typesystem
}//namespace core