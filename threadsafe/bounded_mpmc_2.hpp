// My take on ring buffer using plain booleans for filled/empty
// Will we face the ABA problem?

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
            ring[i].tag.store(false);
        }
    }

    bool try_push(T const& data) {
        auto index = write_to.load(std::memory_order_relaxed);

        for (;;) {
            auto& slot = ring[index % N];
            auto filled = slot.tag.load(std::memory_order_acquire);

            if ( !filled ) { // empty & epoch matches
                // try to CAS write_to <- write_to + 1
                if ( write_to.compare_exchange_weak(index, index+1, std::memory_order_relaxed) ) {
                    slot.data = data;
                    slot.tag.store(true, std::memory_order_release);
                    return true;
                }
            }
            else { // Full -- that's our own tail...
                return false;
            }
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
        auto index = read_from.load(std::memory_order_relaxed);

        for (;;) {
            auto& slot = ring[index % N];
            auto filled = slot.tag.load(std::memory_order_acquire);

            if ( filled ) {
                if ( read_from.compare_exchange_weak(index, index+1, std::memory_order_relaxed) ) {
                    data = slot.data;
                    slot.tag.store(false, std::memory_order_release);
                    return true;
                }
            }
            else { // empty
                return false;
            }
        }
    }

    void close() { active.store(false, std::memory_order_release); }
    bool closed() const { return active.load(std::memory_order_acquire); }

    explicit operator bool () const {
        return active.load(std::memory_order_acquire) || (write_to.load(std::memory_order_acquire) != read_from.load(std::memory_order_acquire));
    }


    void print_state() const {
        std::cerr << "Q: [" << read_from.load() << " -> " << write_to.load() << "] | active: " << std::boolalpha << active.load() << "\n";
    }

private:
    std::atomic<size_t> write_to {0};

    std::vector<TaggedData<T, std::atomic<bool>>> ring {N};
    
    alignas(core::device::CPU::cacheline_size)
        std::atomic<size_t> read_from {0};
    
    alignas(core::device::CPU::cacheline_size)
        std::atomic<bool> active {true};
};