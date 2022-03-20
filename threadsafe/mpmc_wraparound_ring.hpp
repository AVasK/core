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
    ready,
    jetissoned
};


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
            
            if ( tag == tag_status::ready ) {
                std::cerr << "[[ " << slot.data << "]]\n\n\n\n\n\n\n";
            } // read from it and push to the queue?
            
            // then:
            // -> we try to set its status as 'jetissoned'
            // empty -> jetissoned
            // wait -> (wait for other value?)
            
        }
    }


    bool try_pop(value_type & data) {
        auto index = use_cached ? cached_read_index : q.read_from.fetch_add(1, std::memory_order_acq_rel);

        auto& slot = q.ring[index % q.ring.size()];
        auto tag = slot.tag.load(std::memory_order_acquire);

        if ( tag == tag_status::ready ) {
            // slot.tag.store(false, std::memory_order_release);
            if (slot.tag.compare_exchange_strong(tag, tag_status::wait, std::memory_order_acq_rel)) {
                data = slot.data;
                use_cached = false;
                slot.tag.store(tag_status::empty, std::memory_order_release);
                return true;
            }
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
        auto tag = q.ring[cached_read_index % q.ring.size()].tag.load();
        char sym;
        switch(tag) {
                case tag_status::empty : sym = ' '; break;
                case tag_status::wait  : sym = '?'; break;
                case tag_status::ready: sym = '#'; break;
                case tag_status::jetissoned: sym = '^'; break;
            }
        std::cerr << "tag: " << sym << "\n";
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
    ~queue_writer() noexcept { q.n_writers.fetch_sub(1); }

    bool try_push(value_type const& data) {
        auto index = use_cached ? cached_write_index : q.write_to.fetch_add(1, std::memory_order_acq_rel);

        auto& slot = q.ring[index % q.ring.size()];
        auto tag = slot.tag.load(std::memory_order_acquire);

        if ( tag == tag_status::empty ) { 
                if (slot.tag.compare_exchange_strong(tag, tag_status::wait, std::memory_order_acq_rel)) {
                    slot.data = data;
                    use_cached = false;
                    slot.tag.store( tag_status::ready, std::memory_order_release );
                    return true;
                }
        }
        cached_write_index = index;
        use_cached = true;
        return false;
    }

    void push(value_type const& data) {
        constexpr size_t n_spinwaits = 100000;
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
            elem.tag.store( tag_status::empty );
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

    
    bool closed() const noexcept { return ( n_writers.load(std::memory_order_acquire) == 0 ); } //!active.load(std::memory_order_acquire); }

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
            char sym; 

            auto tag = elem.tag.load();
            switch(tag) {
                case tag_status::empty : sym = ' '; break;
                case tag_status::wait  : sym = '?'; break;
                case tag_status::ready: sym = '#'; break;
                case tag_status::jetissoned: sym = '^'; break;
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

    std::atomic<size_t> n_writers {0};
    std::atomic<size_t> n_readers {0};
};