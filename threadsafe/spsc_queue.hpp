#pragma once

#include <atomic>
#include "../cpu.hpp" // cacheline_size
#include "../range.hpp" 
#include "auxiliary/tagged.hpp" // TaggedData 

#include <vector>


template <typename T>
class spsc_queue {
public:
    spsc_queue(size_t size=2048) : ring(size) {
        for (auto& elem : ring) {
            elem.tag.store(false);
        }
    }

    bool try_push(T const& data) {
        static thread_local size_t cached_index = 0;
        static thread_local bool previous_failed = false;

        auto index = previous_failed ? cached_index : write_to.fetch_add(1, std::memory_order_relaxed);

        auto& slot = ring[index % ring.size()];
        auto filled = slot.tag.load(std::memory_order_acquire);

        if ( !filled ) { 
                slot.data = data;
                slot.tag.store(true, std::memory_order_release);
                previous_failed = false;
                return true;
        }
        else { // Full
            cached_index = index;
            previous_failed = true;
            return false;
        }
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
        static thread_local size_t cached_index = 0;
        static thread_local bool previous_failed = false;

        auto index = previous_failed ? cached_index : read_from.fetch_add(1, std::memory_order_relaxed);

        auto& slot = ring[index % ring.size()];
        auto filled = slot.tag.load(std::memory_order_acquire);

        if ( filled ) {
            data = slot.data;
            slot.tag.store(false, std::memory_order_release);
            previous_failed = false;
            return true;
        }

        cached_index = index;
        previous_failed = true;
        return false;

    }
    

    void close() { active.store(false, std::memory_order_release); }
    bool closed() const { return active.load(std::memory_order_acquire); }

    explicit operator bool () const {
        return active.load(std::memory_order_acquire) || (write_to.load(std::memory_order_acquire) - read_from.load(std::memory_order_acquire) == 1);
    }


    void print_state() const {
        std::cerr << "Q: [" << read_from.load() << " -> " << write_to.load() << "] | active: " << std::boolalpha << active.load() << "\n";
        std::cerr << "[ " << bool(*this) << " ]\n";
    }


    void debug_ring() const {
        for (auto i : core::range(ring.size()) ) {
            auto& elem = ring[i];
            char sym; 

            auto filled = elem.tag.load();
            
            if ( filled ) sym = '#';
            else sym = ' ';

            if ( filled ) {
                std::cout << "[" << sym << "| " << elem.data << "]" << " @ " << i << "/" << ring.size() << "\n";
            }
        }
    }


private:
    std::vector< TaggedData<T, std::atomic<bool>> > ring;

    alignas(core::device::CPU::cacheline_size) 
    std::atomic<size_t> read_from {0};

    alignas(core::device::CPU::cacheline_size) 
    std::atomic<size_t> write_to {0};

    alignas(core::device::CPU::cacheline_size) 
    std::atomic<bool> active {true};
};