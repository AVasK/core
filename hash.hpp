#pragma once

#include "meta.hpp"
#include <type_traits>

namespace core {

template <typename T, typename = meta::check_valid>
struct hashable {
    static constexpr bool value = false;
};

template <typename T>
struct hashable<T, meta::is_valid<decltype( std::hash<T>{}(std::declval<T>()) )>> {
    static constexpr bool value = true;
};

}// namespace core;