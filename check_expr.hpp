#pragma once

#include "typesystem.hpp"

namespace core {

namespace detail {
    struct no_match {
        constexpr auto operator() (...) const noexcept -> no_match { return {}; }
    };

    template <typename F1, typename F2>
    struct overloads : F1, F2 {
        overloads(F1 const& f1, F2 const& f2) : F1{f1}, F2{f2} {}
        using F1::operator();
        using F2::operator();
    };

    template <class F>
    struct expr : F {
        constexpr expr(F const& f) : F{f} {}

        template <typename T>
        constexpr bool operator() (T&& obj) const noexcept { return is_valid_for<T&&>(); }

        template <typename T>
        constexpr bool is_valid_for() const noexcept {return (Type<decltype( overloads<F, no_match>{*this, no_match()}(std::declval<T>()) )> != Type<no_match>); }
    };
}

/**
 * @brief can be used to test if some expression is valid
 * 
 * @tparam F 
 * @param f 
 * @return detail::expr<F> 
 */
template <class F>
constexpr auto expr(F const& f) -> detail::expr<F> { return {f}; }

#define CORE_TEST(x, expr) [](auto x)-> expr {}
#define CORE_HAS_TYPEDEF(name) [](auto _)-> typename decltype(_):: name {}
#define CORE_HAS_MEMBER(name) [](auto _)-> decltype(_.name) {}

// Examples:
// constexpr bool is_valid = expr([](auto _)-> typename decltype(*_) {}).is_valid_for<SomeType>();
///TODO: Maybe add check<>() func.
// or: check<T>([](auto type)-> typename decltype(t)::static_member {});
}