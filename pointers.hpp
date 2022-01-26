#include <iostream>
#include <memory>
#include <type_traits>

#include "typesystem.hpp"
#include "macros.hpp"

namespace core {


// view
template <typename T>
class view {
public:
    using ptr_type = typename std::add_pointer<T>::type;
    using value_type = T;

    explicit constexpr view (T * pointer=nullptr) noexcept : p {pointer} {}

    T& operator* () { return *p; }
    T* operator->() { return p; }

    const T& operator* () const { return *p; }
    const T* operator->() const { return p; }

    constexpr auto get() -> ptr_type { return p; }

    template <class TBase, std::enable_if_t< Type<TBase>(is_base_of<T>) >* = nullptr>
    constexpr operator view<TBase>() noexcept {
        return view<TBase>(p);
    }

protected:
    ptr_type p;
};



// #if (__has_cpp_attribute(no_unique_address) || __cplusplus/100 >= 2020) // Clang
#if CORE_HAS_ATTR(NO_UNIQUE_ADDRESS)
#   warning [[no_unique_address]] feature is used
template <class Base>
class MaybeEmpty {
public:
    // [[no_unique_address]] Base elem;
    NO_UNIQUE_ADDRESS Base elem;
    constexpr MaybeEmpty(Base const& b) : elem{b} {}

    constexpr Base& get() { return elem; }
};
#else
#   warning inheritance-based MaybeEmpty is used
template <class Base, bool Use_inheritance>
class EBO_specialized : public Base {
public:
    constexpr EBO_specialized(Base const& b) : Base{b} {}
    constexpr Base& get() { return *static_cast<Base*>(this); }
};

template <class Base>
class EBO_specialized<Base, false> {
public:
    Base elem;
    constexpr EBO_specialized(Base const& b) : elem{b} {}

    constexpr Base& get() { return elem; }
};

template <class Base>
class MaybeEmpty : public EBO_specialized<Base, Type<Base>( is_empty && !is_final )> {
using EBOBase = EBO_specialized<Base, Type<Base>( is_empty && !is_final )>;
public:
    constexpr MaybeEmpty(Base const& d=Base()) : EBOBase{d} {}
    using EBOBase::get;
};
#endif


namespace detail {
    // as in unique_ptr::pointer -- std::remove_reference<Deleter>::type::pointer if that type exists,
    // otherwise T* 
    template <class Del, typename T>
    constexpr auto get_pointer_type_impl(T) noexcept -> typename std::remove_reference_t<Del>::pointer;

    template <class Del, typename T>
    constexpr auto get_pointer_type_impl(...) noexcept -> T*;

    template <class Del, typename T>
    using get_pointer_type = decltype( get_pointer_type_impl<Del,T>( std::declval<T>() ) );
}

// By inheriting from view<T> we can pass templated ptr<T> into functions expecting view<T>  
template <typename T, class Deleter=std::default_delete<T>>
class ptr : public view<T>, private MaybeEmpty<Deleter> {
    // static_assert(Type<Deleter>( is_invocable_with< typename view<T>::ptr_type > ), "");
    using view<T>::p;
public:
    using pointer = detail::get_pointer_type<Deleter, T>;

    explicit constexpr ptr (T * pointer=nullptr, Deleter const& d=Deleter()) noexcept : view<T>{pointer}, MaybeEmpty<Deleter>{d} {}

    ptr(ptr const&) = delete;
    constexpr ptr(ptr&& other) : view<T>{other.p}, MaybeEmpty<Deleter>{other} { other.p = nullptr; }

    constexpr ptr& operator= (ptr const&) = delete;
    constexpr ptr& operator= (ptr&& other) { p = other.p; other.p = nullptr; return *this; }

    CORE_CONSTEXPR_DESTRUCTOR
    ~ptr() noexcept { if (p) MaybeEmpty<Deleter>::get()(p); }
    
    explicit operator bool() { return p != nullptr; }

    // operator view<T>() { return p; }

    T& operator* () { return view<T>::operator*(); }
    T* operator->() { return view<T>::operator->(); }

    const T& operator* () const { return view<T>::operator*(); }
    const T* operator->() const { return view<T>::operator->(); }

    T * unsafe_raw_ptr() { return p; }

    template <class OtherDeleter>
    constexpr auto with_deleter(OtherDeleter const& del=OtherDeleter()) && -> ptr<T, OtherDeleter> {
        auto res = ptr<T,OtherDeleter>(p, del);
        p = nullptr;
        return res;
    }

    template <class TBase, class BaseDel, std::enable_if_t<
        Type<TBase>(is_base_of<T>) &&
        Type<BaseDel>( core::is_convertible_from<Deleter> )
    >* = nullptr>
    constexpr operator ptr<TBase, BaseDel>() noexcept {
        return ptr<TBase,BaseDel>(p);
    }

};


template <typename T, class Alloc>
struct DeleterFor : MaybeEmpty<Alloc> {
    DeleterFor (Alloc const& a) : MaybeEmpty<Alloc>{ a } {}
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

}