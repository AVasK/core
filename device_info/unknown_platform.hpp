#pragma once

#include <thread> 

#include "ints.hpp"
#include "endianness.hpp"
#include "cpuinfo_base.hpp"

namespace core {
namespace device {

using namespace integral;

// template <typename T>
// T query(const char * feature) {
//     // ...;
// }

// template <typename T>
// bool test_query(const char * feature, T * ret=nullptr) {
//     //....
//     return false;
// }

struct CPU : CPUInfo {};


}// namespace device
}// namespace core