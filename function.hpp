#pragma once

#include <type_traits>
#include "typesystem.hpp"

namespace core {

#if __cplusplus/100 >= 2017
    using std::invoke;
#else
// invoke:
template<
    typename Functor, typename... Args,
    meta::require<
        Type<Functor>.decay()( is_member_pointer )
    >* = nullptr
>
constexpr decltype(auto) invoke(Functor&& f, Args&&... args) 
noexcept( noexcept(std::mem_fn(f)(std::forward<Args>(args)...)) )
{ 
    return std::mem_fn(f)(std::forward<Args>(args)...); 
}
   
template<typename Functor, typename... Args,
    meta::require<
        Type<Functor>.decay()( !is_member_pointer )
    >* = nullptr
>
constexpr decltype(auto) invoke(Functor&& f, Args&&... args) 
noexcept( noexcept(std::forward<Functor>(f)(std::forward<Args>(args)...)) )
{ 
    return std::forward<Functor>(f)(std::forward<Args>(args)...); 
}
#endif

// Function reference (non-owning)
template <typename Signature>
class func_ref;

template <typename Ret, typename... Args>
class func_ref< Ret(Args...) > {
    void * fptr;
    Ret (*invoke_func) (void * fp, Args...) = nullptr;

public:

    template <typename Callable,
        typename = std::enable_if_t< core::Type<Callable>.remove_ref() != core::Type<func_ref> >
    >
    #if __cplusplus/100 >= 2020
    requires( core::Type<Callable>.remove_ref() != core::Type<func_ref> )
    #endif
    func_ref(Callable && f) 
    : fptr {static_cast<void *>( &f )}
    , invoke_func{ [](void * fp, Args&&... args)-> Ret { return (*reinterpret_cast<Callable*>(fp)) (std::forward<Args>(args)...); } } 
    {}

    decltype(auto) operator() (Args&&... args) {
        return invoke_func(fptr, std::forward<Args>(args)... );
    }

private: 
    void * fp = nullptr;
};

}//namespace core