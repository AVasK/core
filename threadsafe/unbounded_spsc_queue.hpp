#pragma once

#include <atomic>
#include <cassert>
#include "../cpu.hpp" // cacheline_size
#include "../range.hpp" 
#include "core/threadsafe/spsc_queue.hpp"
#include "auxiliary/tagged.hpp" // TaggedData 

#include <vector>

template <typename T, typename size_type=unsigned>
class unbounded_spsc_queue;


template <typename T>
struct u_spsc_writer {
public:
    u_spsc_writer (unbounded_spsc_queue<T> & qref) : q(qref) { q.n_writers += 1; }

    ~u_spsc_writer() { 
        q.close();
    }

    bool try_push(T const& data) {
        return q.try_push(data);
    }

    void push(T const& data) {
        q.push(data);
    }

private:
    unbounded_spsc_queue<T> & q;
};


template <typename T>
class u_spsc_reader {
public:
    u_spsc_reader(unbounded_spsc_queue<T> & ref) : q{ref} { q.n_readers += 1; }
    ~u_spsc_reader() { q.n_readers -= 1; }

    bool try_pop(T & data) {
        return q.try_pop(data);
    }

    operator bool() {
        return bool(q);
    }

private:
    unbounded_spsc_queue<T> & q;
};


template <typename T, typename size_type>
class unbounded_spsc_queue {
    static constexpr size_t CHUNK_SIZE = 1024;//256;
    friend u_spsc_reader<T>;
    friend u_spsc_writer<T>;

public:


    struct Block {
        Block() { 
            for (auto i : core::range(CHUNK_SIZE)) {
                cells[i].tag.store(false);
            }
        }

        core::TaggedData<T, std::atomic<bool>> cells[CHUNK_SIZE];
        std::atomic<Block*> next {nullptr};
    };


    unbounded_spsc_queue() {}

    struct oversubscription_error {};

    u_spsc_reader<T> reader() {
        if (n_readers < 1) return {*this};
        else throw oversubscription_error {};
    }

    u_spsc_writer<T> writer() {
        if (n_writers < 1) return {*this};
        else throw oversubscription_error {};
    }


    bool try_pop(T & data) {

        if (read_idx >= CHUNK_SIZE) { // current block has been read through
            auto * next = read_block->next.load(std::memory_order_acquire);
            if (!next) return false;
            
            assert(read_block != nullptr);
            // advance to the next block
            read_block->next.store(nullptr, std::memory_order_release);
            if (free_blocks.try_push(read_block)) {
                // block successfully reused
            } else {
                // reuse space is full, delete the block:
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

        if (write_idx >= CHUNK_SIZE) {
            assert(write_block->next.load(std::memory_order_acquire) == nullptr);
            
            Block * reused;
            if (!free_blocks.try_pop(reused)) {
                reused = new Block;
            }
            write_block->next.store(reused, std::memory_order_release);
            write_block = reused;
            write_idx = 0;
        }

        assert(write_block != nullptr);
        assert(write_idx < CHUNK_SIZE);

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
    
    bool empty() const { return (read_block->cells[read_idx].tag == false); }

    explicit operator bool () const {
        return !(closed() && empty());
    }

private:

    unsigned n_writers {0};
    
    Block * read_block = new Block{};
    Block * write_block = read_block;
    spsc_queue< Block* > free_blocks {4}; // free blocks (for reuse)

    alignas(core::device::CPU::cacheline_size) 
    size_type read_idx {0};

    alignas(core::device::CPU::cacheline_size) 
    size_type write_idx {0};

    alignas(core::device::CPU::cacheline_size) 
    std::atomic<bool> active {true};

    unsigned n_readers {0};
};