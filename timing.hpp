#pragma once

#include <chrono>
#include <iostream>

namespace core {
namespace timing {
    
namespace detail {
    
    // Conversion for printing
    template <typename T>
    struct convert {};

    template<>
    struct convert<std::chrono::milliseconds>
    {
        static constexpr char value = 'm';
    };

    template<>
    struct convert<std::chrono::nanoseconds>
    {
        static constexpr char value = 'n';
    };

    template<>
    struct convert<std::chrono::seconds>
    {
        static constexpr char value = ' ';
    };


    template <typename Clock, typename Units>
    class Timer {
    public:
        Timer(const char* _name="unnamed")
        : start_time {Clock::now()}
        , name{_name} {}
        
        void time(const char* _caption = "") {
            auto curr_time = Clock::now();
            auto elapsed = std::chrono::duration_cast<Units>(curr_time - start_time).count();
            if (*_caption != '\0')
                std::cout << name << " timer @ [" << _caption << "] time: "
                            << elapsed << " " << convert<Units>::value<<"s\n";
            else
                std::cout << name << " timer time: "
                            << elapsed << " " << convert<Units>::value<<"s\n";
        }
        
        ~Timer() {
            time();
        }
        
    private:
        std::chrono::time_point<Clock> start_time;
        const char* name;
    };

} // namespace detail


using ms = std::chrono::milliseconds;
using ns = std::chrono::nanoseconds;

template <typename Units = std::chrono::milliseconds>
using timer = detail::Timer<std::chrono::high_resolution_clock, Units>;

using timer_ms = timer<std::chrono::milliseconds>;
using timer_ns = timer<std::chrono::nanoseconds>;


template <
    class Units = std::chrono::milliseconds,
    class F,
    typename... Args,
    class Clock = std::chrono::high_resolution_clock
>
auto timeit(F && f, Args&&... args) {
    auto start_time = Clock::now();
    std::forward<F>(f)( std::forward<Args>(args)... );
    auto end_time = Clock::now();
    auto elapsed = std::chrono::duration_cast<Units>(end_time - start_time).count();
    return elapsed;
}

}; // namespace timing

using timing::timeit;
using timing::timer;
using timing::timer_ms;
using timing::timer_ns;

}; // namespace core
