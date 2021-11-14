// COMPILER SUPPORT
#if defined __clang__ 
#   define CORE_CLANG

#elif defined __GNUC__
#   define CORE_GCC

#elif defined _MSC_VER
#   define CORE_MSVC

#elif defined __EDG__
#   define CORE_EDG
#endif