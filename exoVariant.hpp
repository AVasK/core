//PENDING REWRITE
#pragma once

#include <iostream>
#include <cstddef>
#include <array>
#include "meta_old.hpp"

using byte = int8_t;


template <class... _Fs>
inline
static constexpr auto __make_farray(_Fs&&... __fs) {
    using result = std::array<std::common_type_t<_Fs...>, sizeof...(_Fs)>;
    return result{{std::forward<_Fs>(__fs)...}};
}

template <typename... Ts>
static auto deleters_for() 
-> const meta::c_func_ptr<void, void*> * {
    static constexpr meta::c_func_ptr<void, void*> fs[] = {
        [](void * p) {reinterpret_cast<Ts*>(p)->~Ts();}...
    };
    return fs;
}

template <typename... Ts>
static constexpr auto variant_deleters() 
-> std::array<std::function<void(void*)>, sizeof...(Ts)> {
    /*constexpr*/ std::array<std::function<void(void*)>, sizeof...(Ts)> fs = {
        [](void * p) {reinterpret_cast<Ts*>(p)->~Ts();}...
    };
    return fs;
}

// constexpr auto switch_() {
//     std::initializer_list<bool> ({(i == Is ? (ret = F<Is> ()),true : true)...});
// }

template <typename T, size_t I>
struct _Del {
    // constexpr _Del(size_t tag, void * mem) {
    //     if (tag == I ) {
    //         reinterpret_cast<T*>(mem)->~T();
    //     }
    // }

    static constexpr bool del(size_t tag, void * mem) {
        if (tag == I ) {
            reinterpret_cast<T*>(mem)->~T();
        }
        return true;
    }
};

template <typename... Ts>
struct Del : _Del<Ts, meta::index_of<Ts,Ts...>()>... {
    // constexpr Del(size_t tag, void * mem) 
    // : _Del<Ts, meta::index_of<Ts,Ts...>()>{ tag, mem }...
    // { }

    static constexpr void del(size_t tag, void * mem) {
        auto _ = {_Del<Ts, meta::index_of<Ts,Ts...>()>::del( tag, mem )...};
    }
};

// Cons: not constexpr, uses function, e.t.c
template <typename... Ts>
static void deleter(size_t idx, void * mem) {
    static const std::function<void(void*)> del[] = {
        [](void * mem){reinterpret_cast<Ts*>(mem)->~Ts();}...
    };
    del[idx](mem);
}

namespace exo {

template <typename... Ts>
class variant;



// Error types:
struct bad_variant_access {};

struct unchecked_t {} unchecked;

namespace detail {
        std::false_type _is_variant(void const*);

        template <typename ...Ts>
        std::true_type _is_variant(variant<Ts...> const*);

        template <typename T>
        struct is_variant
          : decltype(detail::_is_variant(static_cast<T*>(nullptr)))
        {};

        template <typename ...Ts>
        struct is_variant<variant<Ts...>>
          : std::true_type
        {};

        template <typename ...Ts>
        struct is_variant<variant<Ts...> const>
          : std::true_type
        {};
}

template <bool Condition>
struct conditional;

template <>
struct conditional<false> {
    template <typename F, typename... Args>
    static constexpr 
    void call(F&&, Args&&...) noexcept {META_UNREACHABLE;}
};

template <> 
struct conditional<true> {
    template <typename F, typename... Args>
    static constexpr 
    decltype(auto) call(F&& f, Args&&... args) {
        return std::invoke( std::forward<F>(f), std::forward<Args>(args)... );
    }
};


template <
    bool Condition,
    typename F,
    typename... Args
>
constexpr decltype(auto) invoke_if(F&& f, Args&&... args) {
    return conditional<Condition>::template call(std::forward<F>(f), std::forward<Args>(args)...);
    // if constexpr (Condition) {
    //     return std::invoke( std::forward<F>(f), std::forward<Args>(args)... );
    // } else {
    //     META_UNREACHABLE;
    // }
}

// VISITATION (multi-visitation)
// template <typename F, typename... Vs>
// constexpr decltype(auto) visit(F && f, Vs&&... vs) {}

// VISIT (single)
template <typename F, typename V, typename=std::enable_if_t<detail::is_variant<std::remove_reference_t<V>>::value> >
constexpr decltype(auto) visit(F && f, V && v) {
    constexpr size_t N = std::remove_reference_t<V>::num_types();
    switch( v.type_index() ) {
        // case 0: dispatch<( 0 < N )>::template case_<0, void>( std::forward<F>(f), v );
        case 0: return invoke_if<( 0 < N )>( std::forward<F>(f), v.template as<0>(unchecked) );
        case 1: return invoke_if<( 1 < N )>( std::forward<F>(f), v.template as<1>(unchecked) );
        case 2: return invoke_if<( 2 < N )>( std::forward<F>(f), v.template as<2>(unchecked) );
        case 3: return invoke_if<( 3 < N )>( std::forward<F>(f), v.template as<3>(unchecked) );
        case 4: return invoke_if<( 4 < N )>( std::forward<F>(f), v.template as<4>(unchecked) );
        case 5: return invoke_if<( 5 < N )>( std::forward<F>(f), v.template as<5>(unchecked) );
        case 6: return invoke_if<( 6 < N )>( std::forward<F>(f), v.template as<6>(unchecked) );
        case 7: return invoke_if<( 7 < N )>( std::forward<F>(f), v.template as<7>(unchecked) );
    }
}


// Variant:
template <typename... Ts>
class variant {
    static_assert( meta::all({ !std::is_same<Ts, void>::value... }),
    "variant cannot hold a void type");

public:

    static constexpr size_t num_types() noexcept { return sizeof...(Ts); };

    // TODO: Probably Rewrite 
    // constexpr 
    // variant(variant const& other)
    // : type_tag{other.type_tag}
    // {
    //     for (size_t i=0; i<std::max({sizeof(Ts)...}); ++i){
    //         memory[i] = other.memory[i];
    //     }
    // }

    // [C++14] need to exclude variant<...> for V!
    template <typename V, typename = std::enable_if_t<!detail::is_variant<std::remove_reference_t<V>>::value>>
    constexpr /*explicit*/
    variant (V&& value) 
    : type_tag {meta::index_of<meta::best_match_t<V, Ts...>, Ts...>()}
    {
        using X = std::decay_t<V>;
        new(&memory) X( std::forward<X>(value) );
        std::cout << "Type: " << type(value).remove_ref() << " -> ";
        std::cout << "Tag: " << type_tag << "\n";
    }

    template <typename V>
    constexpr auto operator= (V&& value) {
        // check if V matches Ts...
        auto new_type_tag = meta::index_of< 
                                meta::best_match_t<  
                                    std::remove_reference_t<V>, 
                                    Ts...
                                >,
                                Ts...
                            >();
        if (new_type_tag == meta::constants::not_found) {
            std::cout << "cannot match " << type(value) << "\n";
        }
        std::cout << "new_tag: " << new_type_tag << "\n";
        if ( new_type_tag < sizeof...(Ts) ) {
            // delete old value
            Del<Ts...>::del(type_tag, &memory);
            // this->match(
            //     [](Ts* mem){delete mem;}...
            // );
            // set new value
            new(&memory) V( std::forward<V>(value) );
            // update type_tag
            type_tag = new_type_tag;
            std::cout << "tag = " << new_type_tag << "\n";
        } else {
            // throw exception otherwise
        }
    }

    template <typename V>
    constexpr bool is() const noexcept {
        return (type_tag == meta::index_of<V, Ts...>());
    }

    template <typename V>
    constexpr auto as() -> V& {
        if ( this->is<V>() ) {
            return *reinterpret_cast<V*>(&memory);
        } else {
            throw bad_variant_access {};
        }
    }

    template <
        size_t I,
        typename V = meta::type_at<I, Ts...>
    >
    constexpr auto as() -> V& {
        static_assert(I < sizeof...(Ts), "variant::as<I>: index I out of bounds!");
        if ( this->is<V>() ) {
            return *reinterpret_cast<V*>(&memory);
        } else {
            throw bad_variant_access {};
        }
    }

    template <
        size_t I,
        typename V = meta::maybe_type_at<I, Ts...>
    >
    constexpr auto as(unchecked_t) -> V& {
        return *reinterpret_cast<V*>(&memory);
    }


    template <typename V>
    constexpr auto as(unchecked_t) -> V& {
        return *reinterpret_cast<V*>(&memory);
    }


    size_t type_index() const {
        return this->type_tag;
    }


    template <typename... Fs>
    constexpr 
    decltype(auto) match (Fs&&... fs) {
        exo::visit(meta::overload(std::forward<Fs>(fs)...), *this);
    }

    ~variant() {
        std::cerr << "Deleting variant " << type(*this) << "["<<type_tag<<"]\n";
        // deleter<Ts...>(type_tag, &memory);
        Del<Ts...>::del(this->type_tag, &memory);
        // match(
        //     [](Ts * mem){delete mem;}...
        // );
    }
private:
    alignas(Ts...) byte memory[ std::max({sizeof(Ts)...}) ];
    size_t type_tag;
    //static const std::array<std::function<void(void*)>, sizeof...(Ts)> deleter = make_deleters_for_types<Ts...>();
};

} //exo