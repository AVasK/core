#include <iostream>
#include <memory>
#include <type_traits>

#include "core/typesystem.hpp"
#include "core/macros.hpp"

namespace core {

// view
template <typename T>
class view {
public:
    using ptr_type = typename std::add_pointer<T>::type;
    using value_type = T;

    explicit constexpr view (T * pointer=nullptr) noexcept : p {pointer} {}
    // view (ptr<T> & pointer) : p {pointer.unsafe_raw_ptr()} {}

    T& operator* () { return *p; }
    T* operator->() { return p; }

    const T& operator* () const { return *p; }
    const T* operator->() const { return p; }

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



// By inheriting from view<T> we can pass templated ptr<T> into functions expecting view<T>  
template <typename T, class Deleter=std::default_delete<T>>
class ptr : public view<T>, private MaybeEmpty<Deleter> {
    // static_assert(Type<Deleter>( is_invocable_with< typename view<T>::ptr_type > ), "");
    using view<T>::p;
public:
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

};



// alloc
// template <typename T, typename... Ts>
// ptr<T> alloc (Ts&&... values) {
//     return ptr<T>( new T (std::forward<Ts>(values)...) );
// }


template <typename T, class Alloc=std::allocator<T>> 
struct alloc_with : private MaybeEmpty<Alloc> {
    constexpr explicit alloc_with (Alloc const& a=Alloc()) : MaybeEmpty<Alloc>{ a } {}

    struct Deleter : MaybeEmpty<Alloc> {
        Deleter(Alloc const& a) : MaybeEmpty<Alloc>{ a } {}
        void operator() (T * p) { 
            MaybeEmpty<Alloc>::get().deallocate(p, 1); 
            p->~T();
        }
    };

    template <typename... Ts>
    constexpr auto operator() (Ts&&... args) {
        auto * p = MaybeEmpty<Alloc>::get().allocate(1);
        new(p) T(std::forward<Ts>(args)...);
        if constexpr (Type<Alloc> != Type<std::allocator<T>>) {
            Deleter del {MaybeEmpty<Alloc>::get()};
            return ptr<T,Deleter>( p, del );
        }
        else {
            return ptr<T>( p ); 
        }
    }
};


template <typename T, class Alloc=std::allocator<T>> 
static auto alloc = alloc_with<T,Alloc>();


}