#pragma once

#include <type_traits>

namespace core {

template < class T >
using remove_cv_t = typename std::remove_cv<T>::type;

template< class T >
struct remove_cvref {
    using type = remove_cv_t<std::remove_reference_t<T>>;
};

template< class T >
using remove_cvref_t = typename remove_cvref<T>::type;

}// namespace core