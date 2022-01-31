#include <iostream>
#include <memory>
#include <type_traits>

#include "typesystem.hpp"
#include "macros.hpp"
#include "maybe_empty.hpp"

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


    template <typename T>
    using unref = std::remove_reference_t<T>;

    template <class Del>
    using Ref = typename decltype(
        Type<Del>.match(
            is<unref<Del> const&>   >>  Type<Del>,
            is_lvalue_reference     >>  Type<Del &>,
            !is_reference           >>  Type<Del const&>
        )
    )::type;

    // using Test = Ref<int&>;

    template <typename T>
    using add_ref = std::add_lvalue_reference_t<T>;
}



// view
template <typename T, typename pointer=std::add_pointer_t<T>>
class view {
public:
    using value_type = T;

    constexpr view (pointer __p=nullptr) noexcept : p {__p} {}

    detail::add_ref<T> operator* () const noexcept { return *p; }
    pointer operator->() const noexcept { return p; }

    constexpr auto get() -> pointer { return p; }

    template <class TBase, std::enable_if_t< Type<TBase>(is_base_of<T>) >* = nullptr>
    constexpr operator view<TBase>() noexcept {
        return view<TBase>(p);
    }

protected:
    pointer p;
};



// By inheriting from view<T> we can pass templated ptr<T> into functions expecting view<T>  
template <typename T, class Deleter=std::default_delete<T>>
class ptr : public view<T, detail::get_pointer_type<Deleter, T>>, private MaybeEmpty<Deleter> {
    using _view = view<T,detail::get_pointer_type<Deleter, T>>;
    using _view::p;
public:
    using pointer = detail::get_pointer_type<Deleter, T>;
    using element_type = T;
    using deleter_type = Deleter;    

    // static_assert(Type<Deleter>( is_invocable_with< typename view<T>::ptr_type > ), "");
    static_assert(Type<Deleter>(!is_rvalue_reference),
        "the specified deleter type cannot be an rvalue reference");
    

    /// Constructors:
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


    // (3.)
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
    template <
        class D = Deleter,
        class RawD = std::remove_reference_t<D>
    >
    constexpr ptr (pointer, std::enable_if_t<Type<D>(is_lvalue_reference), RawD&&>) = delete;


    /// Copy & Move:
    ptr(ptr const&) = delete;

    template <
        class D = Deleter,
        typename= std::enable_if_t<
            Type<D>(!is_reference && is_move_constructible) || Type<D>(is_reference && is_nothrow_move_constructible)
    >>
    constexpr ptr(ptr&& other) : _view{other.release()}, MaybeEmpty<Deleter>{other} {}

    constexpr ptr& operator= (ptr const&) = delete;

    constexpr ptr& operator= (ptr&& other) { p = other.p; other.p = nullptr; return *this; }

    CORE_CONSTEXPR_DESTRUCTOR
    ~ptr() noexcept { if (p) MaybeEmpty<Deleter>::get()(p); }
    
    explicit operator bool() const noexcept { return p != nullptr; }

    detail::add_ref<T> operator* () const noexcept { return _view::operator*(); }
    pointer operator->() const noexcept { return _view::operator->(); }

    constexpr auto unsafe_raw_ptr() const noexcept -> pointer { return p; }
    constexpr auto release() noexcept -> pointer { pointer tmp = p; p = nullptr; return tmp; }
    constexpr auto get_deleter() noexcept -> deleter_type& { return *this; }
    constexpr auto get_deleter() const noexcept -> deleter_type const& { return *this; }
    constexpr auto view() const noexcept -> _view { return *this; }

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