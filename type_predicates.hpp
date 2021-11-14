//TODO: ADD C++ std-version checks: #if CPP_V >= 2017, e.t.c
#pragma once
#include <type_traits>

namespace core {

struct TypePredicate {};

template <template<typename...> class Predicate, typename... Ts>
struct bind_predicate : TypePredicate {
    template <typename T>
    static constexpr 
    bool eval() {
        return Predicate<T,Ts...>::value;
    }
};

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


template <class P1, class P2,
    typename = std::enable_if_t<std::is_base_of<TypePredicate, P1>::value>,
    typename = std::enable_if_t<std::is_base_of<TypePredicate, P2>::value>
>
inline
constexpr auto operator& (P1, P2) noexcept -> And<P1,P2> {
    return And<P1,P2>();
} 


template <class P1, class P2,
    typename = std::enable_if_t<std::is_base_of<TypePredicate, P1>::value>,
    typename = std::enable_if_t<std::is_base_of<TypePredicate, P2>::value>
>
inline
constexpr auto operator| (P1, P2) noexcept -> Or<P1,P2> {
    return Or<P1,P2>();
} 


template <class Pred>
inline
constexpr auto operator! (Pred p) -> Neg<Pred> {
    return {p};
} 


// PREDICATES:
inline namespace type_predicates {

inline constexpr auto is_void = bind_predicate<std::is_void>();

inline constexpr auto is_null_pointer = bind_predicate<std::is_null_pointer>();

inline constexpr auto is_integral = bind_predicate<std::is_integral>();

inline constexpr auto is_floating_point = bind_predicate<std::is_floating_point>();

inline constexpr auto is_array = bind_predicate<std::is_array>();

inline constexpr auto is_enum = bind_predicate<std::is_enum>();

inline constexpr auto is_union = bind_predicate<std::is_union>();

inline constexpr auto is_class = bind_predicate<std::is_class>();

inline constexpr auto is_function = bind_predicate<std::is_function>();

inline constexpr auto is_pointer = bind_predicate<std::is_pointer>();

inline constexpr auto is_lvalue_reference = bind_predicate<std::is_lvalue_reference>();

inline constexpr auto is_rvalue_reference = bind_predicate<std::is_rvalue_reference>();

inline constexpr auto is_member_object_pointer = bind_predicate<std::is_member_object_pointer>();

inline constexpr auto is_member_function_pointer = bind_predicate<std::is_member_function_pointer>();

inline constexpr auto is_fundamental = bind_predicate<std::is_fundamental>();

inline constexpr auto is_arithmetic = bind_predicate<std::is_arithmetic>();

inline constexpr auto is_scalar = bind_predicate<std::is_scalar>();

inline constexpr auto is_object = bind_predicate<std::is_object>();

inline constexpr auto is_compound = bind_predicate<std::is_compound>();

inline constexpr auto is_reference = bind_predicate<std::is_reference>();

inline constexpr auto is_member_pointer = bind_predicate<std::is_member_pointer>();

inline constexpr auto is_const = bind_predicate<std::is_const>();

inline constexpr auto is_volatile = bind_predicate<std::is_volatile>();

inline constexpr auto is_trivial = bind_predicate<std::is_trivial>();

inline constexpr auto is_trivially_copyable = bind_predicate<std::is_trivially_copyable>();

inline constexpr auto is_standard_layout = bind_predicate<std::is_standard_layout>();

inline constexpr auto is_pod = bind_predicate<std::is_pod>();

inline constexpr auto is_literal_type = bind_predicate<std::is_literal_type>();

inline constexpr auto has_unique_object_represenations = bind_predicate<std::has_unique_object_representations>();

inline constexpr auto is_empty = bind_predicate<std::is_empty>();

inline constexpr auto is_polymorphic = bind_predicate<std::is_polymorphic>();

inline constexpr auto is_abstract = bind_predicate<std::is_abstract>();

inline constexpr auto is_final = bind_predicate<std::is_final>();

inline constexpr auto is_aggregate = bind_predicate<std::is_aggregate>();

inline constexpr auto is_signed = bind_predicate<std::is_signed>();

inline constexpr auto is_unsigned = bind_predicate<std::is_unsigned>();

template <class Other>
inline constexpr auto is = bind_predicate<std::is_same, Other>();

template <typename Sub>
inline constexpr auto is_base_of = bind_predicate<std::is_base_of, Sub>();

template <class... Args>
inline constexpr auto is_constructible_from = bind_predicate<std::is_constructible, Args...>();

template <class... Args>
inline constexpr auto is_trivially_constructible_from = bind_predicate<std::is_trivially_constructible, Args...>();

template <typename... Args>
inline constexpr auto is_nothrow_constructible_from = bind_predicate<std::is_nothrow_constructible, Args...>();

inline constexpr auto is_default_constructible = bind_predicate<std::is_default_constructible>();

inline constexpr auto is_trivially_default_constructible = bind_predicate<std::is_trivially_default_constructible>();

inline constexpr auto is_nothrow_default_constructible = bind_predicate<std::is_nothrow_default_constructible>();

inline constexpr auto is_copy_constructible = bind_predicate<std::is_copy_constructible>();

inline constexpr auto is_trivially_copy_constructible = bind_predicate<std::is_trivially_copy_constructible>();

inline constexpr auto is_nothrow_copy_constructible = bind_predicate<std::is_nothrow_copy_constructible>();

inline constexpr auto is_move_constructible = bind_predicate<std::is_move_constructible>();

inline constexpr auto is_trivially_move_constructible = bind_predicate<std::is_trivially_move_constructible>();

inline constexpr auto is_nothrow_move_constructible = bind_predicate<std::is_nothrow_move_constructible>();


} //namespace type_predicates
} //namespace core