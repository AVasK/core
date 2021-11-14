#pragma once

//probably useless to wrap a __has_include, 
//since you'll still need to test if it's defined
//in order to do so portably

#if __cplusplus/100 >= 2014
#   define CORE_CPP14_CONSTEXPR constexpr 
#else
#   define CORE_CPP14_CONSTEXPR
#endif
