#include <cstring> // std::memcpy
#include <type_traits> // std::enable_if
#include "compiler_detect.hpp"
#include "macros.hpp"

namespace core {

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

}