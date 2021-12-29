#pragma once

#include "compiler_detect.hpp"

//probably useless to wrap a __has_include, 
//since you'll still need to test if it's defined
//in order to do so portably

#if __cplusplus/100 >= 2014 || \
(defined CORE_CLANG && __has_feature(__cxx_generic_lambdas__) && __has_feature(__cxx_relaxed_constexpr__))
#   define CORE_CPP14_CONSTEXPR_FUNC constexpr 
#else
#   define CORE_CPP14_CONSTEXPR_FUNC
#endif

#if __cplusplus/100 >= 2017
#   define CORE_CPP17_INLINE_VARIABLE inline
#else
#   define CORE_CPP17_INLINE_VARIABLE
#endif


/// TODO: move to clang-specific header
#if defined CORE_CLANG && __has_feature(__cxx_variable_templates__)
#   define CORE_HAS_VARIABLE_TEMPLATES
#endif

#if defined CORE_CLANG && defined __has_builtin && __has_builtin(__builtin_unreachable)
#   define CORE_UNREACHABLE __builtin_unreachable();
#endif