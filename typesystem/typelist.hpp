#pragma once 

#include "../meta.hpp"
#include "../constexpr_types.hpp" // cx_optional, cx_array
#include "type.hpp"

namespace core {
inline namespace typesystem {

/**
 * @brief Extract type pack Ts... from SomeType<Ts...>
 * 
 * @tparam List List<Ts...>
 * @return TypeList<Ts...>
 */
template <class List>
constexpr auto extract_types() -> meta::rename<List, TypeList> { return {}; }


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

    // sizes of types
    constexpr auto sizes() const noexcept -> cx_array<size_t, sizeof...(Ts)> {
        return {sizeof(Ts)...};
    }

    // alignments of types
    constexpr auto alignments() const noexcept -> cx_array<size_t, sizeof...(Ts)> {
        return {alignof(Ts)...};
    }

    // indexing types
    template <size_t Index>
    constexpr auto at() const noexcept -> TypeOf<meta::type_at<Index, Ts...>> { return {}; }

    // TypeList size = sizeof...(Ts)
    constexpr auto size() const noexcept -> size_t { return sizeof...(Ts); }

    // transformations on types
    template <template <typename...> class NewType, typename... Args>
    constexpr auto make(Args&&... args) const -> recast<NewType> {
        return NewType<Ts...> (std::forward<Args>(args)...);
    }
    constexpr auto head() const noexcept -> TypeOf< meta::head<Ts...> > { return {}; } 
    constexpr auto tail() const noexcept -> meta::apply<TypeList, meta::tail<Ts...>> { return {}; }


    // template <template <typename> class MetaFunc>
    // constexpr auto transform() const noexcept -> TypeList<MetaFunc<Ts>...> { return {}; }   

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
    #endif
    }


    /**
     * @brief filter TypeList by Predicate
     * 
     * @tparam Predicate subclass of detail::TypePredicate [can be created by bind_predicate<>()]
     * @return meta::filter_p<TypeList<Ts...>, Predicate> 
     */
    template <typename Predicate>
    constexpr auto filter(Predicate) const noexcept -> meta::filter_p<TypeList<Ts...>, Predicate> { 
        static_assert(Type<Predicate>( is_subclass_of<detail::TypePredicate> ), 
        "Predicate has to be a subclass of detail::TypePredicate. Use bind_predicate to convert."); 
        return{}; 
    }

    
    /**
     * @brief Test if TypeList contains a single type X
     * 
     * @warning If TypeList is not a set, will result in hard error!
     * @tparam X contained type
     * @return true if X is in TypeList
     */
    template <typename X>
    constexpr bool contains_unique() const noexcept {
        return meta::set_contains<meta::typelist<Ts...>, X>::value;
    }


    /**
     * @brief Test if TypeList contains type X
     * 
     * @note Not optimized for large numbers of template parameters
     */
    template <typename X>
    constexpr bool contains() const noexcept {
        return meta::contains<meta::typelist<Ts...>, X>::value;
    }

#if __cplusplus/100 >= 2014
    // Pattern matching
    template <
        typename... Cases,
        typename=meta::require< Types<Cases...>.all( core::is_subclass_of<detail::TypeCase> )>
    >
    inline
    constexpr auto match(Cases... cases) const noexcept 
    {
    // #if __cplusplus/100 >= 2017
    //     return (Type<Ts>.match(cases...) + ...); // A C++17 way of saying this
    // #else
        // A funny thing is that c++17 fold expression proved to be MUCH SLOWER when compiling with 1000 args
        // and requires setting -fbracket-depth 
        return Types<typename decltype(Type<Ts>.match(cases...))::type...>;
    // #endif
    }
#endif

    ///TODO: Add for_each


#if __cplusplus/100 >= 2014
    template <class P, 
        typename=typename std::enable_if_t<std::is_base_of<detail::TypePredicate, P>::value> 
    >
    constexpr bool all (P) const noexcept {
        return meta::all({ P::template eval<Ts>()... });
    }

    
    template <class P, 
        typename=typename std::enable_if_t<std::is_base_of<detail::TypePredicate, P>::value> 
    >
    constexpr bool any (P) const noexcept {
        return meta::any({ P::template eval<Ts>()... });
    }


    template <class P, 
        typename=typename std::enable_if_t<std::is_base_of<detail::TypePredicate, P>::value> 
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



// TypeList equality testing
template <typename... T1, typename... T2>
constexpr 
bool operator== (TypeList<T1...>, TypeList<T2...>) noexcept {
    return std::is_same<meta::typelist<T1...>, meta::typelist<T2...>>::value;
}

template <typename... T1, typename... T2>
constexpr 
bool operator!= (TypeList<T1...>, TypeList<T2...>) noexcept {
    return !std::is_same<meta::typelist<T1...>, meta::typelist<T2...>>::value;
}

// TypeOf concat
template <typename T, typename U>
constexpr auto operator+ (TypeOf<T>, TypeOf<U>) -> TypeList<T, U> {
    return {};
}

// TypeList concat
template <typename... Ts, typename... Us>
constexpr auto operator+ (TypeList<Ts...>, TypeList<Us...>) -> TypeList<Ts..., Us...> {
    return {};
}

// // TypeList + TypeOf 
template <typename... Ts, typename U>
constexpr auto operator+ (TypeList<Ts...>, TypeOf<U>) -> TypeList<Ts..., U> {
    return {};
}
// TypeOf + TypeList 
template <typename T, typename... Us>
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
    os << "\b\b";
    return os;
}
#endif

}//inline namespace typesystem
}//namespace core