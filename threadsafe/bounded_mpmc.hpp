// Bounded MPMC Queue impl based on the algorithm from D.Vyukov's site 1024cores.net
// https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
// ...and maaan it's fast :) 

/* License from 1024cores.net
Copyright (c) 2010-2011 Dmitry Vyukov. All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.
THIS SOFTWARE IS PROVIDED BY DMITRY VYUKOV "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DMITRY VYUKOV OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
The views and conclusions contained in the software and documentation are those of the authors and should not be interpreted as representing official policies, either expressed or implied, of Dmitry Vyukov.
*/

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


template <class Q>
class BMPMCQ_Writer {
public:
    using value_type = typename Q::value_type;

    BMPMCQ_Writer(Q & ref) : q{ref} { q.writers.fetch_add(1); }

    bool try_push(value_type const& data) { return q.try_push(data); }
    void push(value_type const& data) { q.push(data); }

    ~BMPMCQ_Writer() { if (q.writers.fetch_sub(1) == 1) q.close(); }

private:
    Q & q;
};


template <class Q>
class BMPMCQ_Reader {
public:
    using value_type = typename Q::value_type;

    BMPMCQ_Reader(Q & ref) : q{ref} { }

    bool try_pop(value_type & data) { return q.try_pop(data); }

    explicit operator bool () const { return bool(q); }

private:
    Q & q;
};

template <typename T, size_t N>
class B_MPMC_Queue {
public:
    using value_type = T;

    B_MPMC_Queue() {
        for (size_t i : core::range(N)) {
            ring[i].tag.store(i);
        }
    }

    auto reader() -> BMPMCQ_Reader<B_MPMC_Queue> {
        return {*this};
    }

    auto writer() -> BMPMCQ_Writer<B_MPMC_Queue> {
        return {*this};
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
            else {
                index = write_to.load(std::memory_order_relaxed);
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
            else if ( tag < index+1 ) { // empty
                return false;
            }
            else {
                index = read_from.load(std::memory_order_relaxed);
            }
        }
    }

    void close() { active.store(false); }
    bool closed() const { return active.load(); }
    
    explicit operator bool () const {
        return active || (write_to.load() != read_from.load());
    }

    void print_state() const {
        std::cerr << "Q: [" << read_from.load() << " -> " << write_to.load() << "] | active: " << std::boolalpha << active.load() << "\n";
    }

private:
    std::atomic<int> write_to {0};

    std::vector<TaggedData<T, std::atomic<int>>> ring {N};
    
    alignas(core::device::CPU::cacheline_size)
        std::atomic<int> read_from {0};
    
    alignas(core::device::CPU::cacheline_size)
        std::atomic<bool> active {true};

public:
    std::atomic<unsigned> writers {0};
};