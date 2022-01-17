#pragma once

#include "compiler_detect.hpp"

#ifndef __has_builtin         
#   define __has_builtin(x) 0
#endif

#ifndef __has_feature         
#   define __has_feature(x) 0
#endif

#ifndef __has_extension        
#   define __has_extension(x) 0
#endif

#ifndef __has_cpp_attribute 
#   define __has_cpp_attribute(x) 0
#endif

#if __cplusplus/100 >= 2014 || \
   (__has_extension(cxx_generic_lambdas) && __has_extension(cxx_relaxed_constexpr))

#   define CORE_CPP14_CONSTEXPR_FUNC constexpr 
#else
#   define CORE_CPP14_CONSTEXPR_FUNC
#endif


#if __cplusplus/100 >= 2020
#   define CORE_CONSTEXPR_DESTRUCTOR constexpr
#else
#   define CORE_CONSTEXPR_DESTRUCTOR
#endif


#if __cplusplus/100 >= 2017 || \
    __has_extension(cxx_inline_variables)
#   define CORE_CPP17_INLINE_VARIABLE inline
#else
#   define CORE_CPP17_INLINE_VARIABLE
#endif


#if __has_feature(__cxx_variable_templates__)
#   define CORE_HAS_VARIABLE_TEMPLATES
#endif

#if __has_builtin(__builtin_unreachable)
#   define CORE_UNREACHABLE __builtin_unreachable();
#endif


#define CORE_HAS_ATTR(x) defined(CORE_HAS_ATTR_ ## x)

#if __cplusplus/100 >= 2020 || \
    __has_cpp_attribute(no_unique_address) || \
    (defined CORE_MSVC && _MSC_VER >= 1929)
#   define CORE_HAS_ATTR_NO_UNIQUE_ADDRESS
#endif


#if __cplusplus/100 >= 2020 || __has_cpp_attribute(no_unique_address)
#   define NO_UNIQUE_ADDRESS [[no_unique_address]]
// MSVC:
// https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/#msvc-extensions-and-abi
#elif defined CORE_MSVC && _MSC_VER >= 1929 // VS2019 v16.10 and later (_MSC_FULL_VER >= 192829913 for VS 2019 v16.9)
// Works with /std:c++14 and /std:c++17, and performs optimization
#   define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
// no-op in MSVC v14x ABI
#define NO_UNIQUE_ADDRESS

#endif