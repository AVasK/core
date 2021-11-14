#pragma once
/* OS DETECTION */
#if (__APPLE__ || __MACH__)
    #define CORE__MAC_OS
#elif (WIN32 || _WIN32 || __WIN32__ || __NT__)
    #define CORE__WIN32_OS
    #if (_WIN64)
        #define CORE__WIN64_OS
    #endif
#elif __linux__
    #define CORE__LINUX_OS
#elif __unix__
    #define CORE__UNIX_OS
#elif defined(_POSIX_VERSION)
    #define CORE__POSIX_COMPLIANT
#else
    #define CORE__UNKNOWN_OS
#endif