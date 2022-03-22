#pragma once

#include <atomic>
#include <cassert>
#include "../cpu.hpp" // cacheline_size
#include "../range.hpp" 
#include "core/threadsafe/spsc_queue.hpp" // used as a channel to transfer free_blocks 
#include "auxiliary/tagged.hpp" // TaggedData 
#include "io_descriptors.hpp" // core::{queue_reader, queue_writer}

#include <vector>



template <typename T, size_t chunk_size=256, typename size_type=unsigned>
class unbounded_spsc_queue {
public:
    using value_type = T;

private:
    friend core::queue_reader<unbounded_spsc_queue>;
    friend core::queue_writer<unbounded_spsc_queue>;

    struct too_many_readers : std::exception {};
    struct too_many_writers : std::exception {};

    struct Block {
        Block() { 
            for (auto i : core::range(chunk_size)) {
                cells[i].tag.store(false);
            }
        }

        core::TaggedData<T, std::atomic<bool>> cells[chunk_size];
        std::atomic<Block*> next {nullptr};
    };

public:

    unbounded_spsc_queue(unsigned n_free_blocks=8) : free_blocks{n_free_blocks} {
        #if LOG
        std::cerr << "[read: " << read_block << "]\n";
        #endif
    }

    ~unbounded_spsc_queue() {
        Block * mem;
        while (free_blocks.try_pop(mem)) {
            #if LOG
            std::cerr << "[~ "<<mem<<"]\n";
            #endif
            delete mem;
        }
        #if LOG
        std::cerr << "[~ "<<read_block<<"]\n";
        #endif
        delete read_block;
    }

    core::queue_reader<unbounded_spsc_queue> reader() {
        return (n_readers < 1) ? *this 
        : throw too_many_readers();
    }

    core::queue_writer<unbounded_spsc_queue> writer() {
        return (n_writers < 1)? *this
        : throw "fuck c++";
    }


    bool try_pop(T & data) {

        if (read_idx >= chunk_size) { // current block has been read through
            auto * next = read_block->next.load(std::memory_order_acquire);
            if (!next) return false;
            
            // advance to the next block
            read_block->next.store(nullptr, std::memory_order_release);
            if (free_blocks.try_push(read_block)) {
                // block successfully reused
            } else {
                // reuse space is full, delete the block:
                #if LOG
                std::cerr << "[~ "<<read_block<<"]\n";
                #endif
                delete read_block; 
            }
            read_block = next;
            read_idx = 0;

        }

        auto& cell = read_block->cells[read_idx];
        auto filled = cell.tag.load(std::memory_order_acquire);

        if ( filled ) 
        {
            data = cell.data;
            cell.tag.store(false, std::memory_order_release);
            read_idx += 1;
            return true;
        }
        return false;
        
    }


    bool try_push(T const& data) {

        if (write_idx >= chunk_size) {
            // assert(write_block->next.load(std::memory_order_acquire) == nullptr);
            
            Block * reused;
            if (!free_blocks.try_pop(reused)) {
                reused = new Block; 
                #if LOG
                std::cerr << "[new "<<reused<<"]\n";
                #endif
            }
            write_block->next.store(reused, std::memory_order_release);
            write_block = reused;
            write_idx = 0;
        }

        assert(write_block != nullptr);
        assert(write_idx < chunk_size);

        auto & cell = write_block->cells[write_idx];
        auto filled = cell.tag.load(std::memory_order_acquire);

        if ( !filled ) { 
                cell.data = data;
                cell.tag.store(true, std::memory_order_release);
                write_idx += 1;
                return true;
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
    

    void close() { active.store(false, std::memory_order_release); }
    bool closed() const { return !active.load(std::memory_order_acquire); }
    
    bool empty() const { return (read_block->cells[read_idx].tag.load(std::memory_order_acquire) == false); }

    explicit operator bool () const {
        return !(closed() && empty());
    }

private:
    spsc_queue< Block* > free_blocks; // free blocks (for reuse)

    // reader thread:
    alignas(core::device::CPU::cacheline_size) 
    Block * read_block = new Block{};
    size_type read_idx {0};
    unsigned n_readers {0};
    
    // writer thread:
    alignas(core::device::CPU::cacheline_size) 
    Block * write_block = read_block;
    size_type write_idx {0};
    unsigned n_writers {0};

    alignas(core::device::CPU::cacheline_size) 
    std::atomic<bool> active {true};
    
};