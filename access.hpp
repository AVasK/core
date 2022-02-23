// An access-point data-type for safe concurrent access throug the internal use of mutex
#pragma once

#include <mutex>
#include <iostream> // temp

#if __cplusplus/100 >= 2017
#include <optional>
#endif

namespace core {


template <typename T, class Mutex=std::mutex>
struct locked {
    explicit constexpr locked(T& obj, Mutex & mut) : ref{ obj }, m{ &mut } { m->lock(); }
    explicit constexpr locked(T& obj, Mutex & mut, std::adopt_lock_t) : ref{ obj }, m{ &mut } {/*adopting the locked mutex*/}
    locked( locked&& other ) : ref{ other.ref }, m{ other.m } { other.m = nullptr; };
    locked( locked const& ) = delete;
    locked& operator= (locked const& ) = delete;
    locked& operator= (locked && other) { 
        ref = other.ref;  
        m = other.m;
        other.m = nullptr;
    }

    constexpr T const* operator->() const noexcept { return &ref; }
    constexpr T* operator->() noexcept { return &ref; }
    constexpr T& operator*() noexcept { return ref; } 
    operator T& () noexcept { return ref; }
    ~locked(){ if (m) m->unlock(); }
private:
    T & ref;
    Mutex * m;
};


template <typename T, class Mutex=std::mutex>
struct access {
    constexpr access (T const& v) : value{v}, _mutex{} {}

    constexpr locked<T,Mutex> operator->() const noexcept { return locked<T,Mutex>(value, _mutex); }
    constexpr locked<T,Mutex> operator->() noexcept { return locked<T,Mutex>(value, _mutex); }

    constexpr Mutex mutex() { return _mutex; }

    constexpr locked<T,Mutex> lock() { return locked<T,Mutex>(value, _mutex); }
    
    #if __cplusplus/100 >= 2017
    std::optional<locked<T,Mutex>> try_lock() { 
        if (_mutex.try_lock()) { return locked<T,Mutex>(value, _mutex, std::adopt_lock); }
        else return {};
    }
    #endif

private:
    T value;
    Mutex _mutex;
};

}// namespace core