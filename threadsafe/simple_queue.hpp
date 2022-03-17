#pragma once 

#include <queue>
#include "../access.hpp"
#include "../cpu.hpp"

template <typename T>
class SimpleQueue {
public:
    SimpleQueue() : queue{ std::queue<T>{} } {}

    void push(T const& v) {
        queue->push(v);
    }

    T pop() {
        auto q = queue.lock();
        auto r = q->front();
        q->pop();
        return r;
    }

    bool try_pop(T & v) {
        auto q = queue.lock();
        if (q->empty()) return false;
        v = q->front(); q->pop();
        return true;
    }

    std::vector<T> pop_batch(size_t expected) {
        std::vector<T> batch; 
        batch.reserve(expected);
        auto q = queue.lock();
        for (size_t i=0; i < std::min(expected, q->size()); i++) {
            // if (q->empty()) break;
            batch.push_back(q->front());
            q->pop();
        }
        return batch;
    }

    void pop_into(std::vector<T> & batch) {
        auto q = queue.lock();
        const auto size = q->size();
        for (size_t i=0; i < std::min(batch.capacity(), size); i++) {
            batch.push_back(q->front());
            q->pop();
        }
    }

    bool empty() {
        return queue->empty();
    }

    void close() {
        _closed.store(true, std::memory_order_release);
    }

    bool closed() const {
        return _closed.load(std::memory_order_acquire);
    }

    explicit operator bool() { return !closed() || !empty(); }

private:
    core::access< std::queue<T> > queue;
    
    alignas(core::device::CPU::cacheline_size)
    std::atomic<bool> _closed {false};
};