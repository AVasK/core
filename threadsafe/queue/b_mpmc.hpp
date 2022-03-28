// My take on ring-buffer-based-mpmc-queue using epochs for filled/empty to battle the ABA
// AVasK.

#pragma once

#include <atomic>
#include <vector>
#include <type_traits>

#include "../../cpu.hpp" // cacheline_size
#include "../../ints.hpp"
#include "../auxiliary/tagged.hpp"
#include "io_descriptors.hpp"


template <typename T, size_t N, typename tag_type=unsigned>
class bounded_mpmc {
    static_assert(std::is_unsigned<tag_type>::value, "tag_type should be unsigned!");

    friend core::queue_reader<bounded_mpmc>;
    friend core::queue_writer<bounded_mpmc>;
public:
    using value_type = T;
    static constexpr size_t max_writers = -1;
    static constexpr size_t max_readers = -1;

    bounded_mpmc() {
        for (size_t i : core::range(N)) {
            ring[i].tag.store(0);
        }
    }

    core::queue_reader<bounded_mpmc> reader() {
        return {*this};
    }

    core::queue_writer<bounded_mpmc> writer() {
        return {*this};
    }


    bool try_push(T const& data) {
        auto index = write_to.load(std::memory_order_acquire);//relaxed);

        auto& slot = ring[index % N];
        auto tag = slot.tag.load(std::memory_order_acquire);

        auto epoch = tag_type( index/N );
        if ( tag % 2 == 0 && tag == tag_type(2*epoch) ) { // empty 
            if ( write_to.compare_exchange_weak(index, index+1, std::memory_order_relaxed) ) {
                slot.data = data;
                slot.tag.store(tag+1, std::memory_order_release);
                return true;
                // return slot.tag.compare_exchange_weak(tag, tag+1, std::memory_order_acq_rel);
            }
        }
        
        return false;
    }

    void push(T const& data) {
        constexpr size_t n_spinwaits = 1;//100000;
        // std::cerr << "push...\n";
        for (;;) {
            // std::cerr << ".";
            for (size_t _ : core::range(n_spinwaits)) { if (try_push(data)) return; }
            std::this_thread::yield();
        }
    }

    bool try_pop(T & data) {
        auto index = read_from.load(std::memory_order_acquire);//relaxed);

        auto& slot = ring[index % N];
        auto tag = slot.tag.load(std::memory_order_acquire);

        auto epoch = tag_type( index / N );
        if ( tag % 2 != 0 && (tag_type(2*epoch) == (tag-1)) ) {
            if ( read_from.compare_exchange_weak(index, index+1, std::memory_order_relaxed) ) {
                data = std::move(slot.data);
                slot.tag.store(tag+1, std::memory_order_release);
                return true;
                // return slot.tag.compare_exchange_weak(tag, tag+1, std::memory_order_acq_rel);
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
    std::vector<core::TaggedData<T, std::atomic<tag_type>>> ring {N};
    
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