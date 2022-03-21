#pragma once

#include <atomic>
#include "../cpu.hpp" // cacheline_size
#include "../range.hpp" 
#include "auxiliary/tagged.hpp" // TaggedData 

#include <vector>

template <typename T, typename size_type=unsigned>
class spsc_queue;


template <typename T>
struct spsc_writer {
public:
    spsc_writer (spsc_queue<T> & qref) : q(qref) { q.n_writers += 1; }

    ~spsc_writer() { 
        q.close();
    }

    bool try_push(T const& data) {
        return q.try_push(data);
    }

    void push(T const& data) {
        q.push(data);
    }

private:
    spsc_queue<T> & q;
};


template <typename T>
class spsc_reader {
public:
    spsc_reader(spsc_queue<T> & ref) : q{ref} { q.n_readers += 1; }
    ~spsc_reader() { q.n_readers -= 1; }

    bool try_pop(T & data) {
        return q.try_pop(data);
    }

    operator bool() {
        return bool(q);
    }

private:
    spsc_queue<T> & q;
};


template <typename T, typename size_type>
class spsc_queue {
    friend spsc_reader<T>;
    friend spsc_writer<T>;

public:
    spsc_queue(size_t size=2048) : ring(size), _size(size) {
        for (auto& elem : ring) {
            elem.tag.store(false);
        }
    }


    struct oversubscription_error {};

    spsc_reader<T> reader() {
        if (n_readers < 1) return {*this};
        else throw oversubscription_error {};
    }

    spsc_writer<T> writer() {
        if (n_writers < 1) return {*this};
        else throw oversubscription_error {};
    }


    bool try_pop(T & data) {
        auto index = read_from + 1;

        auto& slot = ring[index % _size];
        auto filled = slot.tag.load(std::memory_order_acquire);

        if ( filled ) 
        {
            data = slot.data;
            slot.tag.store(false, std::memory_order_release);
            read_from = index;
            return true;
        }
        return false;
    }


    bool try_push(T const& data) {
        auto index = write_to + 1;

        auto& slot = ring[index % _size];
        auto filled = slot.tag.load(std::memory_order_acquire);

        if ( !filled ) { 
                slot.data = data;
                slot.tag.store(true, std::memory_order_release);
                write_to = index;
                return true;
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
    

    void close() { active.store(false, std::memory_order_release); }
    bool closed() const { return !active.load(std::memory_order_acquire); }
    bool empty() const { return write_to == read_from; }

    explicit operator bool () const {
        return !(closed() && empty());
    }


    void print_state() const {
        std::cerr << "Q: [" << read_from/*.load()*/ << " -> " << write_to/*.load()*/ << "] | active: " << std::boolalpha << active.load() << "\n";
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
    const size_type _size;

    unsigned n_writers {0};
    std::vector< core::TaggedData<T, std::atomic<bool>> > ring;

    alignas(core::device::CPU::cacheline_size) 
    size_type read_from {0};

    alignas(core::device::CPU::cacheline_size) 
    size_type write_to {0};

    alignas(core::device::CPU::cacheline_size) 
    std::atomic<bool> active {true};

    unsigned n_readers {0};
};