//! Queue Simple Benchmark

#include <iostream>
#include <cassert>
#include "../../thread.hpp"
#include "../../range.hpp"
#include "../../timing.hpp"
#include "../../access.hpp"


// #include "core/threadsafe/queue/mutex_queue.hpp" 
#include "b_mpmc.hpp"
#include "spsc_queue.hpp"
#include "unbounded_spsc_queue.hpp"
#include "../bounded_mpmc.hpp" 
// #include "core/threadsafe/unbounded_spsc_queue_beta.hpp"
// #include "core/threadsafe/mpmc_queue.hpp"
// #include "core/threadsafe/mpmc_proto.hpp"
// #include "core/threadsafe/mpmc_wraparound_ring.hpp"


int main() {
    using core::timing::ms;

    using Queue = 
        // spsc_queue<size_t>;
        // unbounded_spsc_queue<size_t>;
        bounded_mpmc<size_t, 512>;//1'048'576>;
        // B_MPMC_Queue<size_t, 512>;


    Queue q{};

    constexpr size_t N = 1'000'000;//1'000'000;
    size_t s1, s2;

    std::atomic<size_t> global_sum {0};
    std::atomic<bool> set_exit {false};
    std::atomic<unsigned> n_closed {0};
    size_t n_producers = 5;
    size_t n_consumers = 5;

    core::access<int> time {{}}; 

    {// threads

        std::vector<core::thread> producers;
        producers.reserve(n_producers);
        for (auto i : core::range(n_producers)) {
            producers.emplace_back( [&] {
                auto relay = q.writer();
                for (size_t i : core::range(N/n_producers)) {
                    relay.push(i);
                }
            });
        }

        // core::thread observer {[&] {
        //     using namespace std::literals;
        //     std::this_thread::sleep_for(1000ms);
        //     for (;;) {
        //         std::cerr << "======debug=====\n";
        //         q.print_state();
        //         q.debug_ring();
        //         std::cerr << "exit: " << set_exit.load() << "\n";
        //         std::cerr << "================\n";
        //         std::this_thread::sleep_for(500ms);
        //     }
        // }};
        // observer.detach();

       
        std::vector<core::thread> consumers;
        consumers.reserve(n_consumers);
        for (auto i : core::range(n_consumers)) {
            consumers.emplace_back( [&, i] {
                auto relay = q.reader();
                auto t = core::timeit([&]{
                    size_t sum = 0;
                    size_t v;
                    bool mb_empty = false;
                    
                    while (true) {
                        if ( relay.try_pop(v) ) {
                            sum += v;
                        } else if (!relay) {
                            break;
                        } 
                    }
                    global_sum += sum;
                });
                *time.lock() = t.in<ms>();
            });
        }
        
    } // threads join

    std::cout << time.lock() << "ms";
    std::cout << "\n" << global_sum << "\n";
    // q.print_state();
    // q.debug_ring();
}