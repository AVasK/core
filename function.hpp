#pragma once

#include <type_traits>
#include "typesystem.hpp"


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
    requires( core::Type<Callable>.remove_ref() != core::Type<func_ref> )
    func_ref(Callable && f) 
    : fptr {reinterpret_cast<void*>( &f )}
    , invoke_func{ [](void * fp, Args&&... args)-> Ret { return (*reinterpret_cast<Callable*>(fp)) (std::forward<Args>(args)...); } } 
    {}

    decltype(auto) operator() (Args&&... args) {
        return invoke_func(fptr, std::forward<Args>(args)... );
    }

private: 
    void * fp = nullptr;
};
