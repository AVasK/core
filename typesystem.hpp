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
struct TypeOf;  

#if __cplusplus/100 >= 2014
template <typename T>
CORE_CPP17_INLINE_VARIABLE
constexpr auto Type = TypeOf<T>{};
#endif


template <typename... Ts>
struct TypeList;

#if __cplusplus/100 >= 2014
template <typename... Ts>
CORE_CPP17_INLINE_VARIABLE
constexpr auto Types = TypeList<Ts...>{};
#endif


template <typename T>
struct TypeOf {
    using type = T;

    operator TypeList<T> (){return {};} // makes TypeOf<T> convertible to TypeList<T>

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

#   if __cplusplus/100 <= 2014
    template <typename Case, typename... Cases>
    constexpr auto match_helper(meta::tag<true>, Case c, Cases... cases) const noexcept {
        using new_T = typename Case::template apply< T >::type;
        return Type<new_T>;
    }

    template <typename Case, typename... Cases>
    constexpr auto match_helper(meta::tag<false>, Case c, Cases... cases) const noexcept {
        return this->match(cases...);
    }
#   endif

    // Pattern matching
    template <
        typename Case,
        typename... Cases,
        typename=meta::require< Types<Case, Cases...>.all( core::is_subclass_of<core::detail::TypeCase> )>
    >
    inline
    constexpr auto match(Case c, Cases... cases) const noexcept {
    #if __cplusplus/100 >= 2017
        if constexpr ( c.template test<T>() ) {
            using new_T = typename Case::template apply< T >::type;
            return Type<new_T>;
        } else {
            return this->match(cases...);
        }
    #else
        constexpr bool cond = c.template test<T>();
        return match_helper(meta::tag<cond>(), c, cases...);
    #endif
    }
    
    constexpr auto match() const noexcept {
        return *this;
    }

    
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

private:
    template <typename Transformation>
    constexpr auto apply_transform(Transformation t) {
        return Type<decltype(t(*this))>;
    }

    // TODO: Add conversion to string
    // operator std::string () {
    //     return type_name<T>();
    // }
};


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
    using types = meta::typelist<Ts...>;
    
    template <template <typename...> class F, typename... Args>
    using recast = F<Ts..., Args...>;

    constexpr auto add_cv() const noexcept -> TypeList<typename std::add_cv<Ts>::type...> { return {}; }
    constexpr auto remove_cv() const noexcept -> TypeList<typename std::remove_cv<Ts>::type...> { return {}; }

    constexpr auto add_const() const noexcept -> TypeList<typename std::add_const<Ts>::type...> { return {}; }
    constexpr auto remove_const() const noexcept -> TypeList<typename std::remove_const<Ts>::type...> { return {}; }

    constexpr auto add_volatile() const noexcept -> TypeList<typename std::add_volatile<Ts>::type...> { return {}; }
    constexpr auto remove_volatile() const noexcept -> TypeList<typename std::remove_volatile<Ts>::type...> { return {}; }

    constexpr auto add_lvalue_reference() const noexcept -> TypeList<typename std::add_lvalue_reference<Ts>::type...> { return {}; }
    constexpr auto add_rvalue_reference() const noexcept -> TypeList<typename std::add_rvalue_reference<Ts>::type...> { return {}; }
    constexpr auto remove_reference() const noexcept -> TypeList<typename std::remove_reference<Ts>::type...> {return {};}
    
    constexpr auto add_pointer() const noexcept -> TypeList<typename std::add_pointer<Ts>::type...> { return {}; }
    constexpr auto remove_pointer() const noexcept -> TypeList<typename std::remove_pointer<Ts>::type...> { return {}; }
    
    // common shortcuts
    constexpr auto add_lvalue_ref() const noexcept -> TypeList<typename std::add_lvalue_reference<Ts>::type...> { return {}; }
    constexpr auto add_rvalue_ref() const noexcept -> TypeList<typename std::add_rvalue_reference<Ts>::type...> { return {}; }
    constexpr auto remove_ref() const noexcept -> TypeList<typename std::remove_reference<Ts>::type...> {return {};}
    constexpr auto add_ptr() const noexcept -> TypeList<typename std::add_pointer<Ts>::type...> { return {}; }
    constexpr auto remove_ptr() const noexcept -> TypeList<typename std::remove_pointer<Ts>::type...> { return {}; }

    constexpr auto decay() const noexcept -> TypeList<typename std::decay<Ts>::type...> { return {}; }
    constexpr auto raw() const noexcept -> TypeList<typename std::remove_reference<typename std::remove_cv<Ts>::type>::type...> { return {}; }


    // transformations on types
    template <template <typename...> class NewType, typename... Args>
    constexpr auto make(Args&&... args) const -> recast<NewType> {
        return NewType<Ts...> (std::forward<Args>(args)...);
    }
    constexpr auto head() const noexcept -> TypeOf< meta::head<Ts...> > { return {}; } 
    constexpr auto tail() const noexcept -> meta::apply<TypeList, meta::tail<Ts...>> { return {}; }

    // template <template <typename> class MetaFunc>
    // constexpr auto transform() const noexcept -> TypeList<MetaFunc<Ts>...> { return {}; }

    // template <template<typename...> class... MetaFuncs>
    // struct transformed;

    // template <template<typename...> class MetaFunc, template<typename...> class... MetaFuncs>
    // struct transformed<MetaFunc, MetaFuncs...> {
    //     using type = typename TypeList< MetaFunc<Ts>... >::template transformed<MetaFuncs...>::type;
    // };

    // template <>
    // struct transformed<> {using type = TypeList<Ts...>; };
   

    template <template<typename...> class MetaFunc, template<typename...> class... MetaFuncs>
    constexpr auto transform() const noexcept {
    #if __cplusplus/100 >= 2017
        if constexpr (sizeof...(MetaFuncs) == 0) {
            return Types< MetaFunc<Ts>... >;
        } else {
            return Types< MetaFunc<Ts>... >.template transform<MetaFuncs...>();
        }
    #else 
        return this->transform_helper<MetaFunc, MetaFuncs...>( meta::tag<(sizeof...(MetaFuncs) == 0)>{} );
        // return typename TypeList<Ts...>::transformed<MetaFunc, MetaFuncs...>::type{};
    #endif
    }


    template <typename Predicate>
    constexpr auto filter(Predicate) const noexcept -> meta::filter_p<TypeList<Ts...>, Predicate> { 
        static_assert(Type<Predicate>( is_subclass_of<detail::TypePredicate> ), 
        "Predicate has to be a subclass of detail::TypePredicate. Use bind_predicate to convert."); 
        return{}; 
    }
        // if constexpr (Types<Ts...>.head().satisfies( p )) {
        //     return Types<Ts...>.head() + Types<Ts...>.tail().filter(p);
        // } else {
        //     return Types<Ts...>.tail().filter(p);
        // }


#if __cplusplus/100 >= 2014
    // Pattern matching
    template <
        typename... Cases,
        typename=meta::require< Types<Cases...>.all( core::is_subclass_of<detail::TypeCase> )>
    >
    inline
    constexpr auto match(Cases... cases) const noexcept 
    {
    #if __cplusplus/100 >= 2017
        return (Type<Ts>.match(cases...) + ...); // A C++17 way of saying this
    #else
        return Types<typename decltype(Type<Ts>.match(cases...))::type...>;
    #endif
    }
#endif

    //TODO: Add for_each


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

private:
     template <template<typename...> class MetaFunc, template<typename...> class... MetaFuncs>
    constexpr auto transform_helper(meta::tag<true>) const noexcept {
            return Types< MetaFunc<Ts>... >;
    }

    template <template<typename...> class MetaFunc, template<typename...> class... MetaFuncs>
    constexpr auto transform_helper(meta::tag<false>) const noexcept {
            return Types< MetaFunc<Ts>... >.template transform<MetaFuncs...>();
    }
};


// TypeOf concat
template <typename T, typename U>
inline
constexpr auto operator+ (TypeOf<T>, TypeOf<U>) -> TypeList<T, U> {
    return {};
}

// TypeList concat
template <typename... Ts, typename... Us>
inline
constexpr auto operator+ (TypeList<Ts...>, TypeList<Us...>) -> TypeList<Ts..., Us...> {
    return {};
}

// // TypeList + TypeOf 
template <typename... Ts, typename U>
inline
constexpr auto operator+ (TypeList<Ts...>, TypeOf<U>) -> TypeList<Ts..., U> {
    return {};
}
// TypeOf + TypeList 
template <typename T, typename... Us>
inline
constexpr auto operator+ (TypeOf<T>, TypeList<Us...>) -> TypeList<T, Us...> {
    return {};
}


// TypeCase construction
template <
    typename T, 
    typename Predicate,
    typename = meta::require< 
                std::is_base_of<detail::TypePredicate, Predicate>::value
    >
>
constexpr auto operator>> (Predicate p, TypeOf<T> t) noexcept {
    return detail::TypeChange<Predicate, T>();
}


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


#if defined PRETTY_FUNC
#   undef PRETTY_FUNC
#endif

}//namespace typesystem
}//namespace core