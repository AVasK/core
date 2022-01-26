#include <iostream>
#include <memory>
#include <type_traits>

#include "typesystem.hpp"
#include "macros.hpp"
#include "maybe_empty.hpp"

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
        Type<BaseDel>(is_convertible_from<Deleter>)
    >* = nullptr>
    constexpr operator ptr<TBase, BaseDel>() noexcept {
        return ptr<TBase,BaseDel>(p);
    }

};


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

}