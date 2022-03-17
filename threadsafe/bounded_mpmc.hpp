// Bounded MPMC Queue impl based on the algorithm from D.Vyukov's site 1024cores.net
// https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
// ...and maaan it's fast :) 

#pragma once

#include <atomic>
#include <vector>

#include "../cpu.hpp" // cacheline_size

template <typename T, typename Tag>
struct TaggedData {
    using value_type = T;
    using tag_type = Tag;

    TaggedData() = default;
    TaggedData(T const& d, Tag t) : data{d}, tag{t} {}

    T data;
    Tag tag;
};


template <typename T, size_t N>
class B_MPMC_Queue {
public:
    B_MPMC_Queue() {
        for (size_t i : core::range(N)) {
            ring[i].tag.store(i);
        }
    }

    bool try_push(T const& data) {
        auto index = write_to.load(std::memory_order_relaxed);

        for (;;) {
            auto& slot = ring[index % N];
            auto tag = slot.tag.load(std::memory_order_acquire);

            if ( tag == index ) { // empty & epoch matches
                // try to CAS write_to <- write_to + 1
                if ( write_to.compare_exchange_weak(index, index+1, std::memory_order_relaxed) ) {
                    slot.data = data;
                    slot.tag.store(index+1, std::memory_order_release);
                    return true;
                }
            }
            else if ( tag < index ) { // Full -- that's our own tail...
                return false;
            }
        } 
    }

    void push(T const& data) {
        constexpr size_t n_spinwaits = 1000;
        // std::cerr << "push...\n";
        for (;;) {
            // std::cerr << ".";
            for (size_t _ : core::range(n_spinwaits)) { if (try_push(data)) return; }
            std::this_thread::yield();
        }
    }

    bool try_pop(T & data) {
        auto index = read_from.load(std::memory_order_relaxed);

        for (;;) {
            auto& slot = ring[index % N];
            auto tag = slot.tag.load(std::memory_order_acquire);

            if ( tag == index+1 ) { // filled & epoch matches
                if ( read_from.compare_exchange_weak(index, index+1, std::memory_order_relaxed) ) {
                    data = slot.data;
                    slot.tag.store(index + N, std::memory_order_release);
                    return true;
                }
            }
            else { // empty
                return false;
            }
        }
    }

    void close() { active.store(false); }

    explicit operator bool () const {
        return active || (write_to.load() != read_from.load());
    }

private:
    std::atomic<int> write_to {0};

    std::vector<TaggedData<T, std::atomic<int>>> ring {N};
    
    alignas(core::device::CPU::cacheline_size)
        std::atomic<int> read_from {0};
    
    alignas(core::device::CPU::cacheline_size)
        std::atomic<bool> active {true};
};