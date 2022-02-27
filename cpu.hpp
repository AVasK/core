#pragma once

#if __APPLE__
#include "device_info/apple_platform.hpp"
#elif defined __has_include && __has_include(<sys/param.h>) && __has_include(<sys/sysctl.h>)
#include "device_info/sysctl_info.hpp"
// #elif __WIN32__
//! TODO: Add support for windows
// #elif (__x86_64__ || __amd64__)
//! TODO: Use x86 cpuid to query info
#else
#include "device_info/unknown_platform.hpp"
#endif
