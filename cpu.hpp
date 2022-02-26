#pragma once

#if __APPLE__
#include "device_info/apple_platform.hpp"
// #elif (__x86_64__ || __amd64__)
// ...
#else
#include "device_info/unknown_platform.hpp"
#endif
