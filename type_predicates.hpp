#pragma once

#include <type_traits>
#include "macros.hpp"

namespace core {
inline namespace typesystem {
namespace detail {

struct TypePredicate {};
struct TypeCase {};

struct TypeTransformation {};

template <template <typename> class F>
struct type_transform : TypeTransformation {
    template <typename T>
    using apply = F<T>;
};


template <
    typename Predicate,  
    typename Transform,
    typename = meta::require< 
        std::is_base_of<TypePredicate, Predicate>::value 
        &&
        std::is_base_of<TypeTransformation, Transform>::value
    >  
>
struct TypeMorph : TypeCase {
    template <typename T>
    constexpr bool test() {
        return Predicate::template eval<T>();
    }

    template <typename T>
    using apply = typename Transform::template apply<T>;
};


template <
    typename Predicate,  
    typename T,
    typename = meta::require< 
        std::is_base_of<TypePredicate, Predicate>::value 
    >  
>
struct TypeChange : TypeCase {
    template <typename X>
    constexpr bool test() {
        return Predicate::template eval<X>();
    }

    template <typename>
    using apply = meta::identity<T>;
};


template <
    typename Transform, 
    typename Predicate,
    typename = meta::require< 
                std::is_base_of<TypeTransformation, Transform>::value
                &&
                std::is_base_of<TypePredicate, Predicate>::value
    >
>
constexpr auto operator>> (Predicate condition, Transform morph) noexcept {
    return TypeMorph<Predicate, Transform>{};
}


// Predicate binding
template <template<typename...> class Predicate, typename... Ts>
struct bind_predicate : TypePredicate {
    template <typename T>
    static constexpr 
    bool eval() {
        return Predicate<T,Ts...>::value;
    }
};

template <template<typename...> class Predicate, typename... Ts>
struct rbind_predicate : TypePredicate {
    template <typename T>
    static constexpr 
    bool eval() {
        return Predicate<Ts...,T>::value;
    }
};


// Logical operations on predicates
template <class P>
struct Neg : TypePredicate {
    template <class T>
    static constexpr 
    bool eval() {
        return !P::template eval<T>();
    }
};


template <class P1, class P2>
struct And : TypePredicate {
    template <class T>
    static constexpr 
    bool eval() {
        return P1::template eval<T>() 
            && 
            P2::template eval<T>();
    }
};


template <class P1, class P2>
struct Or : TypePredicate {
    template <class T>
    static constexpr 
    bool eval() {
        return P1::template eval<T>() 
            ||
            P2::template eval<T>();
    }
};

}//namespace detail


template <class P1, class P2,
    typename = std::enable_if_t<std::is_base_of<detail::TypePredicate, P1>::value>,
    typename = std::enable_if_t<std::is_base_of<detail::TypePredicate, P2>::value>
>
inline
constexpr auto operator& (P1, P2) noexcept -> detail::And<P1,P2> {
    return detail::And<P1,P2>();
} 


template <class P1, class P2,
    typename = std::enable_if_t<std::is_base_of<detail::TypePredicate, P1>::value>,
    typename = std::enable_if_t<std::is_base_of<detail::TypePredicate, P2>::value>
>
inline
constexpr auto operator| (P1, P2) noexcept -> detail::Or<P1,P2> {
    return detail::Or<P1,P2>();
} 


template <class Pred,
    typename = std::enable_if_t<std::is_base_of<detail::TypePredicate, Pred>::value>
>
inline
constexpr auto operator! (Pred p) -> detail::Neg<Pred> {
    return detail::Neg<Pred>{};
} 


template <template <typename> class F>
constexpr static auto transform = detail::type_transform<F>();

constexpr static auto otherwise = detail::bind_predicate<meta::always_true>();


using detail::bind_predicate;
using detail::rbind_predicate;

// PREDICATES:
CORE_CPP17_INLINE_VARIABLE constexpr auto is_void = bind_predicate<std::is_void>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_null_pointer = bind_predicate<std::is_null_pointer>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_integral = bind_predicate<std::is_integral>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_floating_point = bind_predicate<std::is_floating_point>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_array = bind_predicate<std::is_array>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_enum = bind_predicate<std::is_enum>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_union = bind_predicate<std::is_union>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_class = bind_predicate<std::is_class>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_function = bind_predicate<std::is_function>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_pointer = bind_predicate<std::is_pointer>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_lvalue_reference = bind_predicate<std::is_lvalue_reference>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_rvalue_reference = bind_predicate<std::is_rvalue_reference>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_member_object_pointer = bind_predicate<std::is_member_object_pointer>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_member_function_pointer = bind_predicate<std::is_member_function_pointer>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_fundamental = bind_predicate<std::is_fundamental>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_arithmetic = bind_predicate<std::is_arithmetic>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_scalar = bind_predicate<std::is_scalar>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_object = bind_predicate<std::is_object>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_compound = bind_predicate<std::is_compound>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_reference = bind_predicate<std::is_reference>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_member_pointer = bind_predicate<std::is_member_pointer>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_const = bind_predicate<std::is_const>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_volatile = bind_predicate<std::is_volatile>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_trivial = bind_predicate<std::is_trivial>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_trivially_copyable = bind_predicate<std::is_trivially_copyable>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_standard_layout = bind_predicate<std::is_standard_layout>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_pod = bind_predicate<std::is_pod>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_literal_type = bind_predicate<std::is_literal_type>();

#if __cplusplus/100 >= 2017
CORE_CPP17_INLINE_VARIABLE constexpr auto has_unique_object_represenations = bind_predicate<std::has_unique_object_representations>();
#endif

CORE_CPP17_INLINE_VARIABLE constexpr auto is_empty = bind_predicate<std::is_empty>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_polymorphic = bind_predicate<std::is_polymorphic>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_abstract = bind_predicate<std::is_abstract>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_final = bind_predicate<std::is_final>();

#if __cplusplus/100 >= 2017
CORE_CPP17_INLINE_VARIABLE constexpr auto is_aggregate = bind_predicate<std::is_aggregate>();
#endif

CORE_CPP17_INLINE_VARIABLE constexpr auto is_signed = bind_predicate<std::is_signed>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_unsigned = bind_predicate<std::is_unsigned>();

template <class Other>
CORE_CPP17_INLINE_VARIABLE constexpr auto is = bind_predicate<std::is_same, Other>();

template <typename Sub>
CORE_CPP17_INLINE_VARIABLE constexpr auto is_base_of = bind_predicate<std::is_base_of, Sub>();

template <typename Base>
CORE_CPP17_INLINE_VARIABLE constexpr auto is_subclass_of = rbind_predicate<std::is_base_of, Base>();

template <class... Args>
CORE_CPP17_INLINE_VARIABLE constexpr auto is_constructible_from = bind_predicate<std::is_constructible, Args...>();

template <class... Args>
CORE_CPP17_INLINE_VARIABLE constexpr auto is_trivially_constructible_from = bind_predicate<std::is_trivially_constructible, Args...>();

template <typename... Args>
CORE_CPP17_INLINE_VARIABLE constexpr auto is_nothrow_constructible_from = bind_predicate<std::is_nothrow_constructible, Args...>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_default_constructible = bind_predicate<std::is_default_constructible>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_trivially_default_constructible = bind_predicate<std::is_trivially_default_constructible>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_nothrow_default_constructible = bind_predicate<std::is_nothrow_default_constructible>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_copy_constructible = bind_predicate<std::is_copy_constructible>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_trivially_copy_constructible = bind_predicate<std::is_trivially_copy_constructible>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_nothrow_copy_constructible = bind_predicate<std::is_nothrow_copy_constructible>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_move_constructible = bind_predicate<std::is_move_constructible>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_trivially_move_constructible = bind_predicate<std::is_trivially_move_constructible>();

CORE_CPP17_INLINE_VARIABLE constexpr auto is_nothrow_move_constructible = bind_predicate<std::is_nothrow_move_constructible>();

} //namespace typesystem
} //namespace core