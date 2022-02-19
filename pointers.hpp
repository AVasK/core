#include <iostream>
#include <memory>
#include <type_traits>

#include "typesystem.hpp"
#include "pattern_matching.hpp"
#include "macros.hpp"
#include "maybe_empty.hpp" // EBCO for Deleter/Allocator
#include "hash.hpp" // hashable<>

namespace core {

namespace detail {
    // as in unique_ptr::pointer -- std::remove_reference<Deleter>::type::pointer if that type exists,
    // otherwise T* 
    template <class Del, typename T>
    constexpr auto get_pointer_type_impl(T) noexcept -> typename std::remove_reference_t<Del>::pointer;

    template <class Del, typename T>
    constexpr auto get_pointer_type_impl(...) noexcept -> T*;

    template <class Del, typename T>
    using get_pointer_type = decltype( get_pointer_type_impl<Del,T>( std::declval<T>() ) );

    template <class Del>
    using Ref = typename decltype(
        Type<Del>.match(
            pattern< _ const& >  >>  pattern< _ const& >, // if Del is a const&,
            pattern< _& >        >>  pattern< _& >,       // else, if Del is &
            pattern< _ >         >>  pattern< _ const& >  // otherwise,
        )
    )::type;

    // using Test = Ref<int>;

    template <typename T>
    using add_ref = std::add_lvalue_reference_t<T>;
}



///=============[ VIEW ]===============
template <typename T, typename Pointer=std::add_pointer_t<T>>
class view {
public:
    using value_type = T;
    using pointer = Pointer;

    constexpr view (pointer __p=nullptr) noexcept : p {__p} {}

    detail::add_ref<T> operator* () const noexcept { return *p; }
    pointer operator->() const noexcept { return p; }

    /// <!> unsafe operation, leaking raw pointer may result in memory leaks or double-free
    constexpr auto get_raw() const noexcept -> pointer { return p; }

    template <class TBase, std::enable_if_t< Type<TBase>(is_base_of<T>) >* = nullptr>
    constexpr operator view<TBase>() noexcept {
        return view<TBase>(p);
    }

protected:
    pointer p;
};



///=====================[ PTR ]=======================
template <typename T, class Deleter=std::default_delete<T>>
class ptr : public view<T, detail::get_pointer_type<Deleter, T>>
          , private MaybeEmpty<Deleter> 
{
    using _view = view<T,detail::get_pointer_type<Deleter, T>>;
    using _view::p;
public:
    using pointer = detail::get_pointer_type<Deleter, T>;
    using element_type = T;
    using deleter_type = Deleter;    

    // static_assert(Type<Deleter>( is_invocable_with< typename view<T>::ptr_type > ), "");
    static_assert(Type<Deleter>(!is_rvalue_reference),
        "the specified deleter type cannot be an rvalue reference");
    

    //============[ Constructors ]=============
    // (1)
    template <class D = Deleter, typename= std::enable_if_t<
        Type<D>(is_default_constructible && !is_pointer)
    >>
    constexpr ptr () noexcept : _view{}, MaybeEmpty<Deleter>{Deleter()} {}

    // (1)
    template <class D = Deleter, typename= std::enable_if_t<
        Type<D>(is_default_constructible && !is_pointer)
    >>
    constexpr ptr (std::nullptr_t) noexcept : _view{nullptr}, MaybeEmpty<Deleter>{Deleter()} {}


    // (2)
    template <class D = Deleter, typename= std::enable_if_t<
        Type<D>(is_default_constructible && !is_pointer)
    >>
    explicit constexpr ptr (pointer __p) noexcept : _view{__p} , MaybeEmpty<Deleter>{Deleter()} {}


    // (3)
    template <
        class D = Deleter, 
        typename= std::enable_if_t<
            Type<D>(is_copy_constructible)
        >
    >
    constexpr ptr (pointer __p, detail::Ref<D> __d) noexcept : _view{__p}, MaybeEmpty<Deleter>{__d} {}


    // (4.a)
    template <
        class D = Deleter, 
        typename= std::enable_if_t<
            Type<D>(is_move_constructible)
        >
    >
    constexpr ptr (pointer __p, std::enable_if_t<Type<D>(!is_lvalue_reference), D&&> __d) noexcept 
    : _view{__p}, MaybeEmpty<Deleter>{std::move(__d)} {}

    // (4.b)
    template <class D = Deleter, class RawD = std::remove_reference_t<D> >
    constexpr ptr (pointer, std::enable_if_t<Type<D>(is_lvalue_reference), RawD&&>) = delete;


    //=============[ COPY ]==============
    /// No copy for non-shared owning pointer
    ptr(ptr const&) = delete;
    ptr& operator= (ptr const&) = delete;


    //=============[ MOVE ]==============
    template <
        class D = Deleter,
        typename= std::enable_if_t<
            Type<D>(is_move_constructible)
    >>
    constexpr ptr(ptr&& other)
    : _view{other.release()}
    , MaybeEmpty<Deleter>{std::forward<deleter_type>(other.get_deleter())} {}


    template <
        typename U, 
        class E,
        typename=meta::require<
            Type<typename ptr<U,E>::pointer>( is_convertible_to< pointer > ) 
            &&
            Type< U >( !is_array ) 
            &&
            (
                ( Type<Deleter>(is_reference) && (Type<Deleter> == Type<E>) ) 
                ||
                ( Type<Deleter>(!is_reference) && Type<E>( is_convertible_to<Deleter> ))
            )
        >
    >
    constexpr ptr(ptr<U,E>&& u) noexcept 
        : _view{u.release()}, MaybeEmpty<Deleter>{ std::forward<E>(u.get_deleter()) } {}


    constexpr ptr& operator= (ptr&& other) noexcept {
        reset( other.release() );
        get_deleter() = std::forward<Deleter>( other.get_deleter() );
        return *this; 
    }


    template <
        typename U, 
        class E,
        typename=meta::require<
            Type<U>( !is_array ) && 
            Type<typename ptr<U,E>::pointer>(is_convertible_to<pointer>) &&
            Type<Deleter&>( is_assignable<E&&> )
        >
    >
    constexpr ptr& operator= ( ptr<U,E> && other ) noexcept {
        reset( other.release() );
        get_deleter() = std::forward<Deleter>( other.get_deleter() );
        return *this; 
    } 

    constexpr ptr& operator= (std::nullptr_t) noexcept { reset(); return *this; }


    //! TODO: Look at the codegen for reset() vs the other one
    CORE_CONSTEXPR_DESTRUCTOR
    ~ptr() noexcept { reset(); } 
        // if (p != nullptr){ get_deleter()(std::move(p)); }
    // }


    explicit operator bool() const noexcept { return p != nullptr; }


    detail::add_ref<T> operator* () const noexcept { return _view::operator*(); }


    pointer operator->() const noexcept { return _view::operator->(); }


    constexpr auto get_raw() const noexcept -> pointer { return p; }


    constexpr auto release() noexcept -> pointer { pointer tmp = p; p = nullptr; return tmp; }


    constexpr auto get_deleter() noexcept -> deleter_type& { return MaybeEmpty<Deleter>::get(); }


    constexpr auto get_deleter() const noexcept -> deleter_type const& { return MaybeEmpty<Deleter>::get(); }


    constexpr auto get() const noexcept -> _view { return *this; }


    constexpr void reset(pointer other=pointer()) noexcept {
        if (p != nullptr){ get_deleter()(std::move(p)); }
        p = other;
    }

    template <class OtherDeleter>
    constexpr auto with_deleter(OtherDeleter const& del=OtherDeleter()) && -> ptr<T, OtherDeleter> {
        auto res = ptr<T,OtherDeleter>(p, del);
        p = nullptr;
        return res;
    }

};


//===========[ hash for ptr ]============
namespace detail {
    template <class P, typename RawPointer = typename P::pointer,
        bool = hashable<RawPointer>::value
    >
    struct ptr_hash : private hashable<RawPointer> {
        size_t operator() (P const& p) const 
        noexcept(noexcept( std::declval<std::hash<RawPointer>>()(std::declval<RawPointer>()) ))
        { return std::hash<RawPointer>{}( p.get_raw() ); }
    };

    // Specialization is disabled if the hash is not defined for RawPointer
    template <class P, typename RawPointer>
    struct ptr_hash<P, RawPointer, false>  : private hashable<RawPointer> {};
}//namespace detail


template <typename T, class Alloc>
struct DeleterFor : MaybeEmpty<Alloc> {
    DeleterFor (Alloc & a) : MaybeEmpty<Alloc>{ a } {}
    void operator() (T * p) { 
        MaybeEmpty<Alloc>::get().deallocate(p, 1); 
        p->~T();
    }
};


template <typename T, class Alloc=std::allocator<T>> 
struct alloc_with : private MaybeEmpty<Alloc> {
    constexpr explicit alloc_with (Alloc const& a=Alloc()) : MaybeEmpty<Alloc>{ a } {}

    template <typename... Ts>
    constexpr auto operator() (Ts&&... args) {
        auto * p = MaybeEmpty<Alloc>::get().allocate(1);
        new(p) T(std::forward<Ts>(args)...);

        using Del = DeleterFor<T,Alloc>;
        Del del {MaybeEmpty<Alloc>::get()};
        return ptr<T,Del>( p, del );
    }
};


template <typename T> 
struct alloc_with<T,std::allocator<T>> : private MaybeEmpty<std::allocator<T>> {
    using Alloc = std::allocator<T>;
    constexpr explicit alloc_with (Alloc const& a=Alloc()) : MaybeEmpty<Alloc>{ a } {}

    template <typename... Ts>
    constexpr auto operator() (Ts&&... args) {
        return ptr<T>( new T(std::forward<Ts>(args)...) ); 
    }
};


template <typename T, class Alloc=std::allocator<T>> 
static auto alloc = alloc_with<T,Alloc>();

}// namespace core;


namespace std {
    template <typename T, class Del>
    struct hash<core::ptr<T,Del>> : core::detail::ptr_hash<core::ptr<T,Del>> {};

    template <typename T, class Pointer>
    struct hash<core::view<T,Pointer>> : core::detail::ptr_hash<core::view<T,Pointer>> {};
} // namespace std