#pragma once

#include <cstdint>
#include <limits>
#include "meta.hpp"

namespace core {
inline namespace integral {

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

}// namespace integral

template <typename From, typename To>
using is_narrowing_conversion = meta::select<
    (std::numeric_limits<From>::max() > std::numeric_limits<To>::max()),
    std::true_type, 
    std::false_type
>;

}// namespace core