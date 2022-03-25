#pragma once
// #include "../class/interface.hpp"
#include <exception>

namespace core {

template <typename Q>
struct queue_writer {
public:
    using value_type = typename Q::value_type;

    queue_writer (Q & ref) : q{ref} {
        q.n_writers += 1;
    }

    ~queue_writer() { 
        if constexpr (Q::max_writers > 1) {
            if (q.n_writers.fetch_sub(1) == 1) q.close();
        } else {
            q.n_writers -= 1;
            q.close();
        }
    }

    bool try_push(value_type const& data) { return q.try_push(data); }
    void push(value_type const& data) { q.push(data); }

private: 
    Q & q;
};


template <typename Q>
class queue_reader {
public:
    using value_type = typename Q::value_type;

    queue_reader(Q & ref) : q{ref} { q.n_readers += 1; }
    ~queue_reader() { q.n_readers -= 1; }

    bool try_pop(value_type & data) {
        return q.try_pop(data);
    }

    operator bool() {
        return bool(q);
    }

private:
    Q & q;
};

}//namespace core