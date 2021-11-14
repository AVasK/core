#pragma once

//probably useless to wrap a __has_include, 
//since you'll still need to test if it's defined
//in order to do so portably

// COMPILER SUPPORT
#ifdef __clang__ 
#   define CORE_CLANG
#elif defined(__GNUC__)
#   define CORE_GCC
#elif defined(_MSC_VER)
#   define CORE_MSVC
#endif

