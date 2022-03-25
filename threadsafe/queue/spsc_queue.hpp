#pragma once

#include <atomic>
#include "../../cpu.hpp" // cacheline_size
#include "../../range.hpp" 
#include "../auxiliary/tagged.hpp" // TaggedData 
#include "io_descriptors.hpp" // core::{queue_reader, queue_writer}

#include <vector>


template <typename T, typename size_type=unsigned>
class spsc_queue {
    friend core::queue_reader<spsc_queue>;
    friend core::queue_writer<spsc_queue>;

    struct too_many_readers : std::exception {};
    struct too_many_writers : std::exception {};

public:
    using value_type = T;
    static constexpr core::u8 max_writers = 1;
    static constexpr core::u8 max_readers = 1;

    spsc_queue(size_t size=2048) : ring(size), _size(size) {
        for (auto& elem : ring) {
            elem.tag.store(false);
        }
    }

    core::queue_reader<spsc_queue> reader() {
        if (n_readers < 1) return {*this};
        else throw too_many_readers{};
    }

    core::queue_writer<spsc_queue> writer() {
        if (n_writers < 1) return {*this};
        else throw too_many_writers{};
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

    bool try_move_pop(T & data) {
        auto index = read_from + 1;

        auto& slot = ring[index % _size];
        auto filled = slot.tag.load(std::memory_order_acquire);

        if ( filled ) 
        {
            assert (slot.data != nullptr);
            data = std::move(slot.data);
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

    bool try_push(T && data) {
        auto index = write_to + 1;

        auto& slot = ring[index % _size];
        auto filled = slot.tag.load(std::memory_order_acquire);

        if ( !filled ) { 
                slot.data = std::move(data);
                slot.tag.store(true, std::memory_order_release);
                write_to = index;
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
    std::vector< core::TaggedData<T, std::atomic<bool>> > ring;
    const size_type _size;

    alignas(core::device::CPU::cacheline_size) 
    std::atomic<bool> active {true};

    alignas(core::device::CPU::cacheline_size) 
    size_type read_from {0};
    unsigned n_readers {0};

    alignas(core::device::CPU::cacheline_size) 
    size_type write_to {0};
    unsigned n_writers {0};

};