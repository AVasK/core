#pragma once

#include <atomic>
#include "../cpu.hpp" // cacheline_size
#include "../range.hpp" 
#include "auxiliary/tagged.hpp" // TaggedData 
#include "../ints.hpp"

#include <vector>


template <class Queue> 
class queue_reader {
public:
    using value_type = typename Queue::value_type;

    queue_reader(Queue & ref) : q{ref} {}


    ~queue_reader() noexcept { 
        // exiting the queue:
        // if the queue is closed && locally-empty (we read throught the end), then proceed as usual:
        if ( empty() ) { 
            q.n_readers.fetch_sub(1); 
        }
        else 
        if ( use_cached ) { // we have a cached slot
            std::cerr << "unfinished read! @" << cached_read_index << "\n";
            auto & slot = q.ring[cached_read_index % q.ring.size()];
            auto tag = slot.tag.load(std::memory_order_acquire);
            
            // then:
            // -> we try to set its status as 'jetissoned'
            
        }
    }


    bool try_pop(value_type & data) {
        auto index = use_cached ? cached_read_index : q.read_from.fetch_add(1, std::memory_order_acq_rel);
        auto current_epoch = index / q.ring.size();
        auto& slot = q.ring[index % q.ring.size()];
        auto stamp = slot.tag.load(std::memory_order_acquire);

        if ( stamp == (2*current_epoch+1) ) {
            data = slot.data;
            use_cached = false;
            slot.tag.store(stamp+1, std::memory_order_release);
            return true;
        }   

        cached_read_index = index;
        use_cached = true;
        return false;
    }
   

    bool empty() const noexcept {
        auto index = use_cached? cached_read_index : q.read_from.load(std::memory_order_acquire);
        return q.closed() 
        &&
        ( index >= q.write_to.load(std::memory_order_acquire) );
    }

    explicit operator bool () const noexcept {
        return !empty();
        // return !q.closed() 
        // ||
        // ( cached_read_index < q.write_to.load(std::memory_order_acquire) );
    }

    void print_state() const {
        std::cerr << "R: [ *" << cached_read_index << " -> " << q.write_to.load() << "] | closed: " << std::boolalpha << q.closed() 
        << " readers: " << q.n_readers << " | writers: " << q.n_writers << "\n";
        auto epoch = q.ring[cached_read_index % q.ring.size()].tag.load();
        std::cerr << "epoch: " << epoch << "\n";
        std::cerr << "[ " << bool(*this) << " ]\n";
    }

private:
    Queue & q;
    size_t cached_read_index {0};
    bool use_cached {false};
};


template <class Queue> 
class queue_writer {
public:
    using value_type = typename Queue::value_type;

    queue_writer(Queue & ref) : q{ref} {}
    ~queue_writer() noexcept { if (q.n_writers.fetch_sub(1) == 1) q.close(); }

    bool try_push(value_type const& data) {
        auto index = use_cached ? cached_write_index : q.write_to.fetch_add(1, std::memory_order_acq_rel);
        auto current_epoch = index / q.ring.size();
        auto& slot = q.ring[index % q.ring.size()];
        auto stamp = slot.tag.load(std::memory_order_acquire);

        if ( stamp == 2*current_epoch ) { 
                slot.data = data;
                use_cached = false;
                slot.tag.store( stamp+1, std::memory_order_release ); // mark as written
                return true;
        }
        cached_write_index = index;
        use_cached = true;
        return false;
    }

    void push(value_type const& data) {
        static thread_local size_t n_spinwaits = 10 * (std::hash<std::thread::id>{}(std::this_thread::get_id()) % 100);
        // Try using exponential backoff?
        for (;;) {
            // std::cerr << ".";
            for (size_t _ : core::range(n_spinwaits)) { if (try_push(data)) return; }
            std::this_thread::yield();
        }
    }

private:
    Queue & q;
    size_t cached_write_index {0};
    bool use_cached {false};
};


template <typename T>
class mpmc_queue {
    friend queue_reader<mpmc_queue>;
    friend queue_writer<mpmc_queue>;
public:
    using value_type = T;

    mpmc_queue(size_t size=2048) : ring(size) {
        for (auto& elem : ring) {
            elem.tag.store( 0 );
        }
    }

    auto reader() -> queue_reader<mpmc_queue> {
        n_readers.fetch_add(1);
        return {*this};
    }

    auto writer() -> queue_writer<mpmc_queue> {
        n_writers.fetch_add(1);
        return {*this};
    }

    
    void close() noexcept { active.store(false, std::memory_order_release); }

    bool closed() const noexcept { return !active.load(std::memory_order_acquire); }

    explicit operator bool () const noexcept {
        return !closed() 
        ||
        ( (read_from.load(std::memory_order_acquire) - write_to.load(std::memory_order_acquire)) != n_readers.load(std::memory_order_acquire) );
    }


    void print_state() const {
        std::cerr << "Q: [" << read_from.load() << " -> " << write_to.load() << "] | active: " << std::boolalpha << !closed() 
        << " readers: " << n_readers << " | writers: " << n_writers << "\n";
        std::cerr << "[ " << bool(*this) << " ]\n";
    }


    void debug_ring() const {
        for (auto i : core::range(ring.size()) ) {
            auto& elem = ring[i];
            auto epoch = elem.tag.load();

            std::cout << "[" << epoch << "| " << elem.data << "]" << " @ " << i << "/" << ring.size() << "\n";
        }
    }

private:
    std::vector< TaggedData<T, std::atomic<unsigned>> > ring;

    alignas(core::device::CPU::cacheline_size) 
    std::atomic<size_t> read_from {0};

    alignas(core::device::CPU::cacheline_size) 
    std::atomic<size_t> write_to {0};

    alignas(core::device::CPU::cacheline_size) 
    std::atomic<bool> active {true};

    std::atomic<size_t> n_writers {0};
    std::atomic<size_t> n_readers {0};
};