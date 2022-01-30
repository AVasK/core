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

    template <class Del>
    struct DeleterType {
        static_assert(Type<Del>(!is_reference), "<!> Instantiation Error");
        using lval_ref = const Del&;
        using good_rval_ref = Del&&;
        // enum{ rval_overload = true };
    };

    template <class Del>
    struct DeleterType<Del&> {
        using lval_ref = Del&;
        using bad_rval_ref = Del&&;
        // enum{ rval_overload = false };
    };

    template <class Del>
    struct DeleterType<const Del&> {
        using lval_ref = const Del&;
        using bad_rval_ref = const Del&&;
        // enum{ rval_overload = false };
    };

}

// By inheriting from view<T> we can pass templated ptr<T> into functions expecting view<T>  
template <typename T, class Deleter=std::default_delete<T>>
class ptr : public view<T>, private MaybeEmpty<Deleter> {
    // static_assert(Type<Deleter>( is_invocable_with< typename view<T>::ptr_type > ), "");
    static_assert(Type<Deleter>(!is_rvalue_reference),
                "the specified deleter type cannot be an rvalue reference");
    using view<T>::p;
public:
    using pointer = detail::get_pointer_type<Deleter, T>;
    using element_type = T;
    using deleter_type = Deleter;


    // (1)
    template <class D = meta::identity<Deleter>, typename= std::enable_if_t<
        Type<typename D::type>(is_default_constructible && !is_pointer)
    >>
    constexpr ptr () noexcept : view<T>{}, MaybeEmpty<Deleter>{Deleter()} {}

    // (1)
    template <class D = meta::identity<Deleter>, typename= std::enable_if_t<
        Type<typename D::type>(is_default_constructible && !is_pointer)
    >>
    constexpr ptr (std::nullptr_t) noexcept : view<T>{nullptr}, MaybeEmpty<Deleter>{Deleter()} {}


    // (2)
    template <class Dummy = meta::identity<Deleter>, typename= std::enable_if_t<
        Type<typename Dummy::type>(is_default_constructible && !is_pointer)
    >>
    explicit constexpr ptr (pointer p_) noexcept 
    : view<T>{p_}
    , MaybeEmpty<Deleter>{Deleter()} {}


    template <
        class DRef = typename detail::DeleterType<deleter_type>::lval_ref ,
        class D = meta::identity<Deleter>, 
        typename= std::enable_if_t<
            Type<typename D::type>(is_constructible_from<DRef>)
        >
    >
    constexpr ptr (pointer p_, DRef d_) noexcept : view<T>{p_}, MaybeEmpty<Deleter>{d_} {}


    template <
        class DType = detail::DeleterType<deleter_type> ,
        class D = meta::identity<Deleter>, 
        typename= std::enable_if_t<
            // detail::DeleterType<deleter_type>::rval_overload &&
            Type<typename D::type>(is_constructible_from<typename DType::rval_ref>)
        >
    >
    constexpr ptr (pointer p_, typename DType::good_rval_ref d_) noexcept : view<T>{p_}, MaybeEmpty<Deleter>{d_} {}


    template <
        class DType = detail::DeleterType<deleter_type>
    >
    constexpr ptr (pointer p_, typename DType::bad_rval_ref d_) noexcept = delete;


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