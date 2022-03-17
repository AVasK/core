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

//! From Apple's code:
//  These are the supported HW selectors for sysctlbyname [...]
//  Parameters that are byte counts or frequencies are 64 bit numbers.
//  All other parameters are 32 bit numbers.

struct CPU : CPUInfo {
    // Apple perflevel0 = max power cores.
    // On M1 arch. currently 2 types of cores
    # if !defined __cpp_lib_hardware_interference_size || __cplusplus < __cpp_lib_hardware_interference_size
    static constexpr i64 cacheline_size =
    #   if defined __aarch64__ 
            128; // M1 
    #   else 
            64; // x86?
    #   endif
    #endif
    
    //return query<i64>("hw.cachelinesize"); }

    static i32 hardware_concurrency(){ return std::thread::hardware_concurrency(); }
    static i32 n_cores(){ return query<i32>("hw.ncpu"); }
    static bool has_hyperthreading(){ return test_query<i32>("hw.cputhreadtype"); }
    static i32 active_cores(){ return query<i32>("hw.activecpu"); }
    static i64 ram() { return query<i64>("hw.memsize")/(1024*1024*1024); }//in GB
    static i32 n_core_types() { i32 n; bool test = test_query<i32>("hw.nperflevels", &n); return test ? n : 1; }
    static bool has_hybrid_cores() { return n_core_types() > 1; }
    static i32 p_cores() { return query<i32>("hw.perflevel0.physicalcpu"); }
    static i32 e_cores() { return query<i32>("hw.perflevel1.physicalcpu"); }

    static bool _has_ext(const char * const name) {
        i32 has_ext = 0;
        bool success = test_query(name, &has_ext);
        return success && has_ext;
    }

    static bool ext_neon() { return _has_ext("hw.optional.neon"); }
    static bool ext_neon_fp16() { return _has_ext("hw.optional.neon_fp16"); }
    static bool ext_neon_hpfp() { return _has_ext("hw.optional.neon_hpfp"); }
    static bool ext_advSIMD() { return _has_ext("hw.optional.AdvSIMD"); }
    static bool ext_armv8_1_atomics() { return _has_ext("hw.optional.armv8_1_atomics"); }

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