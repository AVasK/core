#pragma once 

#include <type_traits>
#include <vector>
#include "range.hpp"

template <typename Callable>
class Generator;

template <typename Callable>
inline auto generator(Callable f) -> Generator<Callable>
{
    return Generator<Callable>(f);
}

template <typename Callable>
class Generator {
public:
    
    using return_type = typename std::result_of<Callable(int)>::type;
    
    explicit Generator(Callable _f) noexcept
    : f{ _f }
    , idx{0} {}
    
    // Copy C'tor
    Generator(Generator const& other)
    : f { other.f }
    , idx{ other.idx }
    {}
    
    // Copy assignment
    auto operator= (Generator const& other) -> Generator&
    {
        f = other.f;
        idx = other.idx;
        return *this;
    }
    
    inline auto next() const -> typename std::result_of<Callable(int)>::type
    {
        return f(idx++);
    }
    
    inline auto take(size_t n) const -> std::vector<return_type>
    {
        std::vector<return_type> tmp{};
        tmp.reserve(n);
        
        for (auto i : range(n))
        {
            tmp.emplace_back( this->next() );
        }
        
        return std::move(tmp);
    }
    
    /*
    template <typename Cond>
    inline auto filter(Cond condition) -> Generator
    {
        auto tmp_f = [condition, this](int& idx)
                   {
                       auto ret = f(idx++);
                       auto flag = condition( ret );
                       while (!flag)
                       {
                           ret = f(idx++);
                           flag = condition( ret );
                       }
                       
                       return ret;
                   };
        
        
        auto tmpGen = Generator<decltype(tmp_f)>{tmp_f};
        tmpGen.idx = this->idx;
        
        
        return tmpGen;
    }
    */
    
private:
    Callable f;
    mutable size_t idx;
};


