// My take on ring-buffer-based-mpmc-queue using epochs for filled/empty to battle the ABA
// AVasK.

#pragma once

#include <atomic>
#include <vector>

#include "../cpu.hpp" // cacheline_size
#include "auxiliary/tagged.hpp"
#include "io_descriptors.hpp"
#include "../ints.hpp"


template <typename T, size_t N>
class B_MPMC_Queue {
    friend core::queue_reader<B_MPMC_Queue>;
    friend core::queue_writer<B_MPMC_Queue>;
public:
    using value_type = T;
    static constexpr size_t max_writers = -1;
    static constexpr size_t max_readers = -1;

    B_MPMC_Queue() {
        for (size_t i : core::range(N)) {
            ring[i].tag.store(0);
        }
    }

    core::queue_reader<B_MPMC_Queue> reader() {
        return {*this};
    }

    core::queue_writer<B_MPMC_Queue> writer() {
        return {*this};
    }


    bool try_push(T const& data) {
        auto index = write_to.load(std::memory_order_relaxed);

        auto& slot = ring[index % N];
        auto tag = slot.tag.load(std::memory_order_acquire);

        auto epoch = index/N;
        if ( tag % 2 == 0 && tag == 2*epoch ) { // empty 
            if ( write_to.compare_exchange_weak(index, index+1, std::memory_order_relaxed) ) {
                slot.data = data;
                return slot.tag.compare_exchange_weak(tag, tag+1, std::memory_order_acq_rel);
            }
        }
        
        return false;
    }

    void push(T const& data) {
        constexpr size_t n_spinwaits = 100000;
        // std::cerr << "push...\n";
        for (;;) {
            // std::cerr << ".";
            for (size_t _ : core::range(n_spinwaits)) { if (try_push(data)) return; }
            std::this_thread::yield();
        }
    }

    bool try_pop(T & data) {
        auto index = read_from.load(std::memory_order_relaxed);

        auto& slot = ring[index % N];
        auto tag = slot.tag.load(std::memory_order_acquire);

        auto epoch = index / N;
        if ( tag % 2 != 0 && ((2*epoch+1) == tag) ) {
            if ( read_from.compare_exchange_weak(index, index+1, std::memory_order_relaxed) ) {
                data = slot.data;
                return slot.tag.compare_exchange_weak(tag, tag+1, std::memory_order_acq_rel);
            }
        }

        return false;

    }

    void close() { active.store(false, std::memory_order_release); }
    bool closed() const { return active.load(std::memory_order_acquire); }

    explicit operator bool () const {
        return active.load(std::memory_order_acquire) || (write_to.load(std::memory_order_acquire) != read_from.load(std::memory_order_acquire));
    }


    void print_state() const {
        std::cerr << "Q: [" << read_from.load() << " -> " << write_to.load() << "] | active: " << std::boolalpha << active.load() << "\n";
        std::cerr << "readers: " << n_readers << "; writers: " << n_writers << "\n";
    }

private:
    std::vector<core::TaggedData<T, std::atomic<size_t>>> ring {N};
    
    alignas(core::device::CPU::cacheline_size)
    std::atomic<size_t> write_to {0};

    alignas(core::device::CPU::cacheline_size)
        std::atomic<size_t> read_from {0};
    
    alignas(core::device::CPU::cacheline_size)
        std::atomic<bool> active {true};

    alignas(core::device::CPU::cacheline_size)
    std::atomic<unsigned> n_readers{0};

    alignas(core::device::CPU::cacheline_size)
    std::atomic<unsigned> n_writers{0};
};