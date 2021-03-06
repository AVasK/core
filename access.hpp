// An access-point data-type for safe concurrent access throug the internal use of mutex
#pragma once

#include <mutex>
#include <iostream> // temp

#if __cplusplus/100 >= 2017
#include <optional>
#endif

namespace core {

// Stricter version of locker (similar to std::lock_guard)
template <typename T, class Mutex=std::mutex>
struct RAII_locker {
    constexpr RAII_locker(T& obj, Mutex & mut) : ref{ obj }, m{ mut } { m.lock(); }
    constexpr RAII_locker(T& obj, Mutex & mut, std::adopt_lock_t) : ref{ obj }, m{ mut } {/*adopting the locked mutex*/}
    RAII_locker( RAII_locker&& other ) = delete;
    RAII_locker( RAII_locker const& ) = delete;
    RAII_locker& operator= (RAII_locker const& ) = delete;
    RAII_locker& operator= (RAII_locker && other) = delete;

    constexpr T const* operator->() const noexcept { return &ref; }
    constexpr T* operator->() noexcept { return &ref; }

    ~RAII_locker(){ m.unlock(); }
private:
    T & ref;
    Mutex & m;
};


// Move-enabled RAII locking class (similar to unique_ptr)
template <typename T, class Mutex=std::mutex>
struct locked {
    constexpr locked(T& obj, Mutex & mut) : ref{ obj }, m{ &mut }, _owns_lock{true} { m->lock(); }
    constexpr locked(T& obj, Mutex & mut, std::adopt_lock_t) : ref{ obj }, m{ &mut }, _owns_lock{true} {/*adopting the locked mutex*/}
    locked( locked&& other ) : ref{ other.ref }, m{ other.m }, _owns_lock{ other._owns_lock } { other.m = nullptr; };
    locked( locked const& ) = delete;
    locked& operator= (locked const& ) = delete;
    locked& operator= (locked && other) { 
        ref = other.ref;  
        m = other.m;
        _owns_lock = other._owns_lock;
        other.m = nullptr;
    }

    void unlock() { if (m) m->unlock(); _owns_lock = false; }

    constexpr T const* operator->() const noexcept { return &ref; }
    constexpr T* operator->() noexcept { return &ref; }
    constexpr T& operator*() noexcept { return ref; }
    // constexpr T& operator*() noexcept { return ref; } 
    operator T& () noexcept { return ref; }

    ~locked(){ if (m && _owns_lock) { m->unlock(); _owns_lock=false; } }
private:
    T & ref;
    Mutex * m;
    bool _owns_lock = false;
};


template <typename T, class Mutex=std::mutex>
struct access {
    constexpr access (T const& v) : value{v}, _mutex{} {}

    constexpr RAII_locker<T,Mutex> operator->() noexcept { return {value, _mutex}; }

    constexpr Mutex& mutex() { return _mutex; }

    constexpr locked<T,Mutex> lock() { return {value, _mutex}; }

    constexpr RAII_locker<T,Mutex> grab() { return {value, _mutex}; }
    
    #if __cplusplus/100 >= 2017
    std::optional<locked<T,Mutex>> try_lock() { 
        if (_mutex.try_lock()) { return {value, _mutex, std::adopt_lock}; }
        else return {};
    }
    #endif

private:
    T value;
    Mutex _mutex;
};

}// namespace core