#pragma once
#include <thread>
#if __cpp_lib_hardware_interference_size && __cplusplus >= __cpp_lib_hardware_interference_size
    #include <new> // hardware_destructive_interference_size
#endif
#include "../ints.hpp"
#include "../endianness.hpp"

namespace core {
namespace device {

using namespace integral;

struct CPUInfo {
    static constexpr i64 cacheline_size =  
    #if __cpp_lib_hardware_interference_size && __cplusplus >= __cpp_lib_hardware_interference_size
        std::hardware_destructive_interference_size();
    #elif (__x86_64__ || __amd64__)    
        64; 
    #elif __powerpc__
        128;
    #else
        64; // A safe assumption if nothing else is known
    #endif

    static i32 hardware_concurrency(){ return std::thread::hardware_concurrency(); }
    static i32 n_cores(){ return 0; }
    static bool has_hyperthreading(){ return false; }
    static i32 active_cores(){ return 0; }
    static i64 ram() { return 0; }
    static i32 n_core_types() { return 0; }
    static bool has_hybrid_cores() { return false; }
    static i32 p_cores() { return 0; }
    static i32 e_cores() { return 0; }

    // static device::endian endianness() { 
    //     // manually check byte order
    // }

    // ARM:
    static bool ext_neon() { return false; }
    static bool ext_neon_fp16() { return false; }
    static bool ext_neon_hpfp() { return false; }
    static bool ext_advSIMD() { return false; }
    static bool ext_armv8_1_atomics() { return false; }

    // x86:
    // ....
};

}// namespace device
}// namespace core
