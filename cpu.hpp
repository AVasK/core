#pragma once

#if __APPLE__
#include "device_info/apple_platform.hpp"
// #elif (__x86_64__ || __amd64__)
// ...
#else
#include "device_info/unknown_platform.hpp"
#endif

struct CPU {
    static i32 max_hardware_threads(){ return std::thread::hardware_concurrency(); }
    
    //! TODO: Add support for Windows & Linux (intel 12th gen has p-cores & e-cores)
    static bool has_hybrid_cores() { return false; /* for now */ } 

    // static i64 cacheline_size(){ return std::thread::... } // C++17? 
};
