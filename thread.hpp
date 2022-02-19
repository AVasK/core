#pragma once

#include <thread>

namespace core {

/**
 * @brief Auto-joinable thread class
 */
class thread : public std::thread {
public:
    using std::thread::thread; // using std::thread c'tors

    ~thread(){
        if ( joinable() ) { join(); }
    }

};


//! TODO: Add mac-specific functions for scheduling on heterogeneous cores of M1 CPU family
///       using `native_handle`

}//namespace core