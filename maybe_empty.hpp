#pragma once

#include "macros.hpp"

namespace core {

// #if (__has_cpp_attribute(no_unique_address) || __cplusplus/100 >= 2020) // Clang
#if CORE_HAS_ATTR(NO_UNIQUE_ADDRESS)
#   if defined CORE_DEBUG_EBCO
#       warning [[no_unique_address]] feature is used
#   endif
template <class Base>
class MaybeEmpty {
public:
    // [[no_unique_address]] Base elem;
    NO_UNIQUE_ADDRESS Base elem;
    constexpr MaybeEmpty(Base const& b) : elem{b} {}

    constexpr Base& get() { return elem; }
};
#else
#   if defined CORE_DEBUG_EBCO
#       warning inheritance-based MaybeEmpty is used
#   endif
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

}