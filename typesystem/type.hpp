#pragma once 

#include <iostream>
#include <type_traits>
#include <cctype> // isspace
#include "../meta.hpp"
#include "../macros.hpp"
#include "../constexpr_types.hpp" // cx_string
#include "../lambda.hpp"
#include "type_predicates.hpp"


namespace core {
inline namespace typesystem {


// Printing the TypeOf<T>
namespace detail {

template <typename T>
inline 
constexpr 
auto type_name() {
    using namespace core::lambda;

    constexpr auto str = core::cx_str(PRETTY_FUNC);
    constexpr auto sq_brace = *str.find('[');
    constexpr auto T_pos = *str.find('T', sq_brace);
    constexpr auto eq_sign_pos = *str.find('=', T_pos);
    constexpr auto type_pos = *str.where($a != ' ', eq_sign_pos+1);
    constexpr auto name = cx_str( str.template slice<type_pos, str.size()-1>() );
    return name;
}

}//namespace detail


template <typename T>
struct TypeOf;  

template <typename... Ts>
struct TypeList;


#if CORE_HAS_VARIABLE_TEMPLATES
template <typename T>
CORE_CPP17_INLINE_VARIABLE
constexpr auto Type = TypeOf<T>{};

template <typename... Ts>
CORE_CPP17_INLINE_VARIABLE
constexpr auto Types = TypeList<Ts...>{};
#endif

template <typename T>
struct TypeOf {
    using type = T;

    // conversion to string
    operator std::string() const {
        return detail::type_name<T>();
    }

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
    // Pattern matching
    template <
        typename Case,
        typename... Cases,
        typename=meta::require< 
            std::is_base_of<detail::TypeCase, Case>::value &&
            meta::all({ std::is_base_of<detail::TypeCase, Cases>::value... }) 
        >
        // typename=meta::require< Types<Case, Cases...>.all( core::is_subclass_of<detail::TypeCase> )>
    >
    inline
    constexpr auto match(Case c, Cases... cases) const noexcept {
    #if __cplusplus/100 >= 2017
        if constexpr ( c.template test<T>() ) {
            using new_T = meta::invoke< Case, T >;
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
        typename=typename std::enable_if<std::is_base_of<detail::TypePredicate, P>::value>::type 
    >
    constexpr bool satisfies(P) const noexcept {
        return P::template eval<type>();
    }

    template <class P, 
        typename=typename std::enable_if<std::is_base_of<detail::TypePredicate, P>::value>::type 
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


#   if __cplusplus/100 <= 2014
    template <typename Case, typename... Cases>
    constexpr auto match_helper(meta::tag<true>, Case c, Cases... cases) const noexcept {
        using new_T = meta::invoke< Case, T >;
        return Type<new_T>;
    }

    template <typename Case, typename... Cases>
    constexpr auto match_helper(meta::tag<false>, Case c, Cases... cases) const noexcept {
        return this->match(cases...);
    }
#   endif

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



template <typename T>
inline
auto operator<< (std::ostream& os, TypeOf<T> type) -> std::ostream& {
    os << detail::type_name<T>();
    return os;
}

}//inline namespace typesystem
}//namespace core