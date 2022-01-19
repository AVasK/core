#pragma once 

#include <cstddef>
#include <type_traits>

namespace core {

/////// MACRO DEFINITIONS FOR FUNCTIONS:
/////// CMP
#define CMP(FName, cmpSign)                         \
template <typename F1, typename F2>                 \
struct FName : FGen {                               \
    F1 _f1; F2 _f2;                                 \
    constexpr FName(F1 f1, F2 f2)                   \
    : _f1 {f1}                                      \
    , _f2 {f2}                                      \
    {}                                              \
                                                    \
    template <typename... Ts>                       \
    constexpr                                       \
    auto operator() (Ts... args) const              \
    -> bool {                                       \
        return _f1(args...) cmpSign _f2(args...);   \
    }                                               \
};                                                  \
                                                    \
                                                    \
template <typename F1, typename F2,                 \
typename = typename std::enable_if<(                \
    std::is_base_of<FGen, F1>::value                \
    &&                                              \
    std::is_base_of<FGen, F2>::value                \
)>::type>                                           \
constexpr                                           \
auto operator cmpSign (F1 f1, F2 f2)                \
 -> FName<F1,F2>                                    \
{                                                   \
    return FName<F1, F2>( f1, f2 );                 \
}                                                   \
                                                    \
template <typename F, typename T>                   \
constexpr                                           \
auto operator cmpSign (F f, T value)                \
-> typename std::enable_if<                         \
    (std::is_base_of<FGen, F>::value                \
    &&                                              \
    !std::is_base_of<FGen, T>::value),              \
    FName<F, Value<T>>                              \
>::type                                             \
{                                                   \
    return FName<F, Value<T>>( f, value );          \
}                                                                      

/////// ARITHM
#define ARITHM(FName, opSign)                       \
template <typename F1, typename F2>                 \
struct FName : FGen {                               \
    F1 _f1; F2 _f2;                                 \
    constexpr FName(F1 f1, F2 f2)                   \
    : _f1{f1}                                       \
    , _f2{f2}                                       \
    {}                                              \
                                                    \
    template <typename... Ts>                       \
    constexpr                                       \
    auto operator() (Ts... args) const              \
    -> decltype(std::declval<F1>()(args...) opSign std::declval<F2>()(args...)) \
    {                                               \
        return _f1(args...) opSign _f2(args...);    \
    }                                               \
};                                                  \
                                                    \
template <typename F1, typename F2,                 \
typename = typename std::enable_if<(                \
    std::is_base_of<FGen, F1>::value                \
    &&                                              \
    std::is_base_of<FGen, F2>::value                \
)>::type>                                           \
constexpr                                           \
auto operator opSign (F1 f1, F2 f2)                 \
 -> FName<F1,F2>                                    \
{                                                   \
    return FName<F1, F2>( f1, f2 );                 \
}                                                   \
                                                    \
template <typename F, typename T>                   \
constexpr                                           \
auto operator opSign (F f, T value)                 \
-> typename std::enable_if<                         \
    (std::is_base_of<FGen, F>::value                \
    &&                                              \
    !std::is_base_of<FGen, T>::value),              \
    FName<F, Value<T>>                              \
>::type                                             \
{                                                   \
    return FName<F, Value<T>>( f, value );          \
}                                                   


namespace lambda {
namespace tools {
    
    template <size_t Index, typename T, typename... Ts>
    struct typeAtIndex {
        using type = typename typeAtIndex<Index-1, Ts...>::type;
    };

    template <typename T, typename... Ts>
    struct typeAtIndex<0, T, Ts...> {
        using type = T;
    };

    template <size_t Index>
    struct extractArg {
        template <typename T, typename... Ts>
        constexpr static auto from(T&&, Ts&&... args) -> typename typeAtIndex<Index, T,Ts...>::type
        {
            return extractArg<Index-1>::from( args... );
        }
    };

    template <>
    struct extractArg<0> {
        template <typename T, typename... Ts>
        constexpr static auto from(T&& arg, Ts&&... args) -> decltype(arg)
        { return arg; }
    };
}; // namespace tools


namespace detail {
struct FGen {};

template <size_t Index>
struct Arg : FGen {
    template <typename... Ts>
    constexpr auto operator() (Ts&&... args) const noexcept-> typename tools::typeAtIndex<Index, Ts...>::type
    {
        return tools::extractArg<Index>::from(args...);
    }
};

template <typename T>
struct Value : FGen {
    constexpr Value(T val) : value{val} {}

    template <typename... Ts>
    constexpr T operator() (Ts const&...) const noexcept 
    {return value;} 

    T value;
};

CMP(FunEq, ==)
CMP(FunNeq, !=)
CMP(FunLt, <)
CMP(FunGt, >)
CMP(FunLe, <=)
CMP(FunGe, >=)
#undef CMP

ARITHM(FSum, +)
ARITHM(FDiff, -)
ARITHM(FMul, *)
ARITHM(FDiv, /)
ARITHM(FMod, %)
#undef ARITHM

}//namespace detail


namespace numeric_args {
    auto $0 = detail::Arg<0>();
    auto $1 = detail::Arg<1>();
    auto $2 = detail::Arg<2>();
    auto $3 = detail::Arg<3>();
    auto $4 = detail::Arg<4>();
}

constexpr auto $a = detail::Arg<0>();
constexpr auto $b = detail::Arg<1>();
constexpr auto $c = detail::Arg<2>();
constexpr auto $d = detail::Arg<3>();
constexpr auto $e = detail::Arg<4>();

} // namespace lambda
} //namespace core