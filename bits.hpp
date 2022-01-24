#include <cstring> // std::memcpy
#include <type_traits> // std::enable_if
#include "compiler_detect.hpp"
#include "macros.hpp"

#if defined __cpp_lib_launder && __cplusplus >= __cpp_lib_launder
#include <new> // std::launder
#endif

namespace core {

/**
 * @brief pre-c++20 bit-cast
 * @remark constexpr when using clang's builtin `__builtin_bit_cast`
 */
template <typename To, typename From>
#if __has_builtin(__builtin_bit_cast)
constexpr
#endif
auto bit_cast (const From& from) noexcept -> typename std::enable_if<
    std::is_trivially_copyable<To>::value && 
    std::is_trivially_copyable<From>::value &&
    (sizeof(To) == sizeof(From)),
To >::type {
#   if __has_builtin(__builtin_bit_cast)
// #   warning Clang __builtin_bit_cast used
    return __builtin_bit_cast(To, from);
#   else
// #   warning std::memcpy used for bit_cast, non-constexpr
    To res;
    std::memcpy(&res, &from, sizeof To);
    return res;
#   endif
}


template <class T>
#if CORE_HAS_ATTR(NODISCARD)
[[nodiscard]]
#endif
constexpr auto launder(T* p) noexcept -> T* {
    static_assert (!(std::is_function<T>::value), "can't launder functions" );
    static_assert (!(std::is_same<void, typename std::remove_cv<T>::type>::value), "can't launder (cv) void" );
#if defined __cpp_lib_launder && __cplusplus >= __cpp_lib_launder
    return std::launder(p);
#elif __has_builtin(__builtin_launder)
    return __builtin_launder(p);
#else
    return p;
#endif
}


template <class As, class From> 
auto alias (From & mem) -> As& {
    return *core::launder(reinterpret_cast<As*>(&mem));
}


}