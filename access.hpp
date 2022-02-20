// An access-point data-type for safe concurrent access throug the internal use of mutex
#pragma once

#include <mutex>
#include <iostream> // temp

namespace core {


template <typename T, class Mutex=std::mutex>
struct locked {
    explicit constexpr locked(T& obj, Mutex & mut) : ref{ obj }, m{ mut } { m.lock(); }
    locked( locked&& other ) = default;
    locked( locked const& ) = delete;
    locked& operator= (locked const& ) = delete;

    constexpr T const* operator->() const noexcept { return &ref; }
    constexpr T* operator->() noexcept { return &ref; }
    constexpr T& operator*() noexcept { return ref; } 
    operator T& () noexcept { return ref; }
    ~locked(){ m.unlock(); }
private:
    T & ref;
    Mutex & m;
};


template <typename T, class Mutex=std::mutex>
struct access {
    constexpr access (T const& v) : value{v}, mutex{} {}
    // if Mutex is a reference type, then can re-link to another mutex?
    // access (T const& v, Mutex other) : ...

    constexpr locked<T,Mutex> operator->() const noexcept { return locked<T,Mutex>(value, mutex); }
    constexpr locked<T,Mutex> operator->() noexcept { return locked<T,Mutex>(value, mutex); }

    constexpr locked<T,Mutex> lock() noexcept { return locked<T,Mutex>(value, mutex); }

private:
    T value;
    Mutex mutex;
};

}// namespace core