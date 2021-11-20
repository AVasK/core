#pragma once

//probably useless to wrap a __has_include, 
//since you'll still need to test if it's defined
//in order to do so portably

#if __cplusplus/100 >= 2014
#   define CORE_CPP14_CONSTEXPR_FUNC constexpr 
#else
#   define CORE_CPP14_CONSTEXPR_FUNC
#endif

#if __cplusplus/100 >= 2017
#   define CORE_CPP17_INLINE_VARIABLE inline
#else
#   define CORE_CPP17_INLINE_VARIABLE
#endif