#pragma once

#include <cerrno>
#include "../ints.hpp"
#include "../endianness.hpp"
#include "cpuinfo_base.hpp"

// Sysctl
#include <sys/param.h>
#include <sys/sysctl.h>


namespace core {
namespace device {

using namespace integral;

struct ErrorCode {
    int code;
};

template <typename T>
T query(const char * feature) {
    T ret;
    size_t size = sizeof(ret);

    if (sysctlbyname(feature, &ret, &size, NULL, 0) == -1) {
        throw ErrorCode{errno};
    }
    return ret;
}

template <typename T>
bool test_query(const char * feature, T * ret=nullptr) {
    size_t size = sizeof(ret);
    if ( sysctlbyname(feature, ret, &size, NULL, 0) != -1 ) return true;
    return false;
}

struct CPU : CPUInfo {

    static i32 hardware_concurrency(){ return std::thread::hardware_concurrency(); }
    static i32 n_cores(){ return query<i32>("hw.ncpu"); }
    static device::endian endianness() { 
        auto order = query<i32>("hw.byteorder"); 
        if (order == 1234) {
            return device::endian::little;
        } else { // order == 4321
            return device::endian::big;
        }
    }
};


}// namespace device
}// namespace core