#pragma once

#include <atomic>
#include "../cpu.hpp" // cacheline_size
#include "../range.hpp" 
#include "auxiliary/tagged.hpp" // TaggedData 
#include "../ints.hpp"

#include <vector>

enum class tag_status : core::u8 {
    empty = 0,
    wait,
    ready
};


template <typename T>
class mpmc_queue {
public:
    mpmc_queue(size_t size=2048) : ring(size) {
        for (auto& elem : ring) {
            elem.tag.store( tag_status::empty );
        }
    }

    bool try_push(T const& data) {
        static thread_local size_t cached_index = 0;
        static thread_local bool previous_failed = false;

        auto index = previous_failed ? cached_index : write_to.fetch_add(1, std::memory_order_acq_rel);

        auto& slot = ring[index % ring.size()];
        auto tag = slot.tag.load(std::memory_order_acquire);

        if ( tag == tag_status::empty ) { 
                if (slot.tag.compare_exchange_strong(tag, tag_status::wait, std::memory_order_acq_rel)) {
                    slot.data = data;
                    previous_failed = false;
                    slot.tag.store( tag_status::ready, std::memory_order_release );
                    return true;
                }
        }
        cached_index = index;
        previous_failed = true;
        return false;
    }

    void push(T const& data) {
        constexpr size_t n_spinwaits = 100000;
        // Try using exponential backoff?
        for (;;) {
            // std::cerr << ".";
            for (size_t _ : core::range(n_spinwaits)) { if (try_push(data)) return; }
            std::this_thread::yield();
        }
    }

    bool try_pop(T & data) {
        static thread_local size_t cached_index = 0;
        static thread_local bool previous_failed = false;

        auto index = previous_failed ? cached_index : read_from.fetch_add(1, std::memory_order_acq_rel);

        auto& slot = ring[index % ring.size()];
        auto tag = slot.tag.load(std::memory_order_acquire);

        if ( tag == tag_status::ready ) {
            // slot.tag.store(false, std::memory_order_release);
            if (slot.tag.compare_exchange_strong(tag, tag_status::wait, std::memory_order_acq_rel)) {
                data = slot.data;
                previous_failed = false;
                slot.tag.store(tag_status::empty, std::memory_order_release);
                return true;
            }
        }

        cached_index = index;
        previous_failed = true;
        return false;
    }
    

    void close() { active.store(false, std::memory_order_release); }
    bool closed() const { return active.load(std::memory_order_acquire); }

    explicit operator bool () const {
        return active.load(std::memory_order_acquire) || (write_to.load(std::memory_order_acquire) > read_from.load(std::memory_order_acquire));
    }


    void print_state() const {
        std::cerr << "Q: [" << read_from.load() << " -> " << write_to.load() << "] | active: " << std::boolalpha << active.load() << "\n";
        std::cerr << "[ " << bool(*this) << " ]\n";
    }


    void debug_ring() const {
        for (auto i : core::range(ring.size()) ) {
            auto& elem = ring[i];
            char sym; 

            auto tag = elem.tag.load();
            switch(tag) {
                case tag_status::empty : sym = ' '; break;
                case tag_status::wait  : sym = '?'; break;
                case tag_status::ready: sym = '#'; break;
            }
            if ( tag == tag_status::wait || tag == tag_status::ready ) {
                std::cout << "[" << sym << "| " << elem.data << "]" << " @ " << i << "/" << ring.size() << "\n";
            }
        }
    }

private:
    std::vector< TaggedData<T, std::atomic<tag_status>> > ring;

    alignas(core::device::CPU::cacheline_size) 
    std::atomic<size_t> read_from {0};

    alignas(core::device::CPU::cacheline_size) 
    std::atomic<size_t> write_to {0};

    alignas(core::device::CPU::cacheline_size) 
    std::atomic<bool> active {true};
};