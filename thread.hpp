#pragma once

#include <thread>

namespace core {

/**
 * @brief Auto-joinable thread class
 */
class thread {
    using id = std::thread::id;
    std::thread t;
public:    

    template <typename... Ts>
    thread (Ts&&... args) noexcept(noexcept( std::thread(std::forward<Ts>(args)...) ))
    : t{ std::forward<Ts>(args)... } {}

    thread (thread const&) = delete;
    thread& operator= (thread const&) = delete;

    thread (thread && other) = default;
    thread& operator= (thread && other) = default;

    thread (std::thread && other) : t{ std::move(other) } {}
    thread& operator= (std::thread && other) {
        t = std::move(other);
        return *this;
    }

    bool joinable() const noexcept { return t.joinable(); }
    auto get_id() const noexcept -> std::thread::id { return t.get_id(); }
    auto native_handle() { return t.native_handle(); }
    static auto hardware_concurrency() noexcept { return std::thread::hardware_concurrency(); }

    void join() { t.join(); }
    void detach() { t.detach(); }

    ~thread(){
        if ( t.joinable() ) { t.join(); }
    }

};


//! TODO: Add mac-specific functions for scheduling on heterogeneous cores of M1 CPU family
///       using `native_handle`

}//namespace core