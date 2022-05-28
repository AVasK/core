# CORE
> CORE is still in development stage, any feedback and support (such as stars/bug reports/issues) is appreciated!  
> Most of the interfaces are *not* fixed yet.

> - The README is gradually updated as the project evolves.

## variadic switch
> ```C++
> core::switch_, core::case_
> ```

> Note: variadic switch doesn't check if the value is out of bounds and doesn't match any case_. This is mainly a non-problem when substituting variadic packs, so a check seemed redundant.

May be useful, for example, while implementing 'visit' for variant types
Gets well-optimised by clang, producing a single jump-table. Things are a little bit more tricky on GCC though, so I may end up hand-unrolling some switch_ versions, e.g. for 4-, 8-, 16- e.t.c alternatives up to some number for which a jump table still makes sense.

> The problem: the standard switch/case cannot be used in a variadic context. Usually that's ok and fine until you really need this functionality. 

Motivating example: implementing a simple `visit` for a single variant type
```C++
template <class V, typename F>
constexpr decltype(auto) dispatch (V && v, F && f) {
    return dispatch_impl(std::make_index_sequence<core::extract_types< std::remove_reference_t<V> >().size()>{},
     std::forward<V>(v), std::forward<F>(f));
}

template <class V, typename F, size_t... Is>
constexpr decltype(auto) dispatch_impl (std::index_sequence<Is...>, V && v, F&& f) {
    return core::switch_(v.index(), 
        core::case_<size_t, Is>([&]{ return core::invoke( std::forward<F>(f), std::forward<V>(v).template at<Is>() ); })... // <- unrolling the Is pack here
    );
}
```

Without a variadic switch there would be more mess and less fun.

A simpler example:
```C++
size_t index;

auto res = core::switch_( index, 
    core::case_<size_t, 0>([&]{ ... }),
    core::case_<size_t, 1>([&]{ ... })
);
```


## core::typesystem

> #### C++14

> ```C++
> using namespace core::typesystem
> ```

### Working with a single Type
```C++
struct TypeOf<T> 
Type<T>  <->  TypeOf<T>( );
```

- For starters, you can easily compare types like that  
( and use those anywhere you wish, since they're constexpr )
```C++
Type<X> == Type<Y> 
Type<X> != Type<Y>
```
instead of this:
```C++
std::is_same<X, Y>::value
// or in C++17 and beyond:
std::is_same_v<X, Y>
```

- You can get the type of the variable like that
```C++
int x;
constexpr auto t1 = type(x);
static_assert( t1( is_lvalue_reference ), "");

constexpr auto t2 = type(1);
static_assert( t2( is_rvalue_reference ), "");
```

- You can **print** the types!
```C++
std::cout << Type<float&> << "\n" //>>> float&
          << type(x) << "\n";     //>>> int& (type() gets the type w.r.t l- and r-valuedness)
```
    the PRETTY_FUNCTION of clang/gcc is used for printing


- You can test different predicates for your types...
    as a matter of fact, there're pre-written predicates that almost 100% mimick the std::type_traits ones!
    ... but if you need something special, just use `core::bind_predicate` as shown below

```C++
Type<A>( is_subclass_of<B> || is_base_of<C> ) -> bool
```
you can either use the predicates from *core* or use core::bind_predicate<> and core::rbind_predicate<> 
as follows:
```C++
constexpr auto is_signed = bind_predicate< std::is_signed >();
// or, to bind predicate to a chosen argument
constexpr auto is_void = core::bind_predicate< std::is_same, void >();
// equivalent to saying std::is_same<void, _> 

// sometimes you'll need the rbind:
template <typename X>
constexpr auto is_subclass_of = rbind_predicate<std::is_base_of, X>();
// equivalent to saying std::is_base_of<_, X> 

```

- Finally, you can do **PATTERN MATCHING** on types! 
    <!> Pattern matching matches the first alternative and returns, there is no fallthrough.
```C++
Type<A>.match(
    is_integral         >>  transform< /* Any metafunctions, including ones from std type_traits */ >,
    is<void>            >>  Type< example::EmptyType >,
    is_lvalue_reference >>  transform< std::remove_reference_t >,
    otherwise           >>  ... // otherwise is an optional clause, like 'default' case in switch.
) 
-> Type< /*whatever the resulting type is after the transformation*/ >
```
    the `.match` uses the following syntax: 
    `predicate >> [transform | Type]`
    the predicate should be created via bind_predicate or rbind_predicate.

#### TODO: Pattern Matching with _ and ___ (for single-type and variadic-type-pack respectively)
> Example:
Pending rewrite or at least a bugfix

```C++
using Ref = typename decltype(
    Type<Del>.match(
        pattern< _ const& >  >>  pattern< _ const& >,
        pattern< _& >        >>  pattern< _& >,
        pattern< _ >         >>  pattern< _ const& >
    )
)::type;
```

### Types:
```C++
struct TypeList<Ts...> 
Types<Ts...>  <->  TypeList<Ts...>( );
```

You can:
- [x] apply same transformations as for TypeOf 
- [x] concatenate Type<> + Types<> and Types<> + Types<> easily!
- [x] get .head() and .tail() of a TypeList
- [x] test if **all**, **any** or **none** of the types satisfy some Predicate.
- [x] transform TypeLists
- [x] filter TypeLists
- [x] use pattern matching on them
- [ ] iterate via for_each (on TODO list)

### Tested TypeList with 5000 template parameters:
```C++
using namespace core::typesystem;
    constexpr auto ts = meta::apply< TypeList, meta::repeat<5000, int> >();
    constexpr auto t = ts.at<400>();
    std::cout << ts.match(
        is_integral >> Type<float>,
        is_void     >> Type<char>
    ).all( is<float> ) << "\n";

    auto nt = ts + ts;
    std::cout << nt.sizes().max() << "\n";
```
`time: g++ -std=c++17 stest.cpp -I ../../GitHub/  0.82s user 0.05s system 97% cpu 0.887 total`

### Examples:

- Simple transformations: the same transformation is applied to all types in the TypeList 
```C++
Types<int, float>.add_const().add_lvalue_ref(); // -> Types<const int &, const float &>
// or, using transform and std type_traits
Types<int, float>.transform< std::add_const_t, std::add_lvalue_reference_t >(); // -> Types<const int &, const float &>
```

Testing predicates
```C++
Types<int, long, void>.all( is_integral || is_floating_point ) // -> false
Types<int, long, void>.any( is_integral || is_floating_point ) // -> true
Types<int, long, void>.none( is_void )                         // -> false
```

*Filter*ing and pattern *match*ing combined
```C++
constexpr auto ts = Types<int, float&, void, double, bool>.filter(!is_void).match(
    is<void>                     >>  Type<char>,
    is_lvalue_reference          >>  transform<std::remove_reference_t, std::add_const_t>,
    (is_integral && !is<bool>)   >>  transform<std::make_unsigned_t>
); // -> Types< unsigned int, const float, double, bool >;
```

### NOTE: Extracting the type when crossing the compile-time and runtime boundary
If you want to make some type calculation and then need a value, it's perfectly OK.
If, on the other hand, you need to get a type, use `decltype()`.

```C++
typename decltype( Types<...>.filter(...).transform<...>().match(...) )::type
```


## Checking if an expression is valid:
> #### C++14 (uses generic lambdas)
>    check_expr.hpp

expr([](auto t) -> ... {}).is_valid_for<T>();

defines macros:
- CORE_HAS_MEMBER(name)  <=> x.name 
- CORE_HAS_TYPEDEF(name) <=> T::name

Example:
```C++
struct P { using pointer = int*; enum { x = 7 }; };

using core::expr; 
constexpr auto test = expr([](auto t) -> decltype(*t){ }).is_valid_for<int*>(); // true
constexpr auto t2 = expr([](auto t)-> typename decltype(t)::pointer {}).is_valid_for<int>(); // false
constexpr auto t3 = expr(CORE_HAS_MEMBER(x)).is_valid_for<P>(); // true
constexpr auto t4 = expr(CORE_HAS_TYPEDEF(pointer)).is_valid_for<P>(); // true
```


## core::lambda

> #### C++11

> ```C++
> using namespace core::lambda;
> ```

Create comparison and arithmetic lambdas quickly and on the spot
Like when using sort, instead of this:
```C++
sort(..., ..., [](auto& a, auto& b){return a < b;});
```
simply write this:
```C++
sort(..., ..., $a < $b); 
```
and that's more readable, too!

Some arithmetics? Easy.
```C++
using namespace core::lambda;
constexpr auto lambda_test = ($a + $b)/2 == $c;
constexpr auto result = lambda_test(2, 4, 3);
```

- Supported operations:
- > Equality testing: { ==, !=, <, >, <=, >= }

- > Arithmetic: { +, -, *, /, % }

Don't like the $a, $b, $c, $d and $e?
They're just variables, after all: 
```C++
constexpr auto $a = detail::Arg<0>();
```

core::lambda::detail::Arg<...>() provides a way to extend to more arguments


## core::range

> #### C++11

> no inner namespace
    

Use Python-style for-loops instead of C/C++'s `for (_;_;_)`
```C++
using core::range;

for (auto i : range(end)) {...}
for (auto i : range(start, end)) {...}
for (auto i : range(start, end, step)) {...}
```
    unlike Python ranges doesn't have negative indexing, but has some functions instead :)
```C++
for (auto i : range(..., ..., ...).reverse()) {...}
```

## Iteration primitives:
- range (as described above)
- zip (zips two Iterables, producing an iterator pair)
- enumerate(Iterable, start=0) implemented as zip(Iterable, core::Range) under the hood.

## Iterable Traits: 
> iter_traits.hpp
Enables checking if a given class:
- `is_iterable<T>::value` -- can be used with begin() and end()
- `is_indexable<T, IndexT>` -- T::operator[](IndexT) is well-defined (SFINAE) 
    + `::value` - returns true/false
    + `::type`  - returns the result of T[IndexT]
- `has_size<T>` -- T.size() is defined
    + `::value` - true/false
    + `::type`  - returns the result type of T.size()


## Function Traits:
> func_traits.hpp
Enables checking if a given callable (func/func.object/...) is:
- `callable_with<F, Args...>`
- `callable_with<Result, F, Args...>`

and the result of function invocation:
- `result_of<F, Args...>`


## core::thread

> #### C++14

An auto-joinable wrapper class for std::thread


## core::access

> #### C++14

```C++
struct A { int x = 7; };
    core::access<A> a {A()};
    std::cout << a->x; // OK: access is serialized 

    core::access<int> x {7};
    std::cout << x.lock(); // OK: Access is serialized here
    {
        auto data = x.lock();
        std::cout << data << "\n";
    }
    // OK: data goes out of scope

    std::cout << x.lock(); // deadlock if data *was* still here!

```

Grabbing the lock on the object and passing it around:
here, the Locked<> acts as a *gateway* object

>Here, the the Locked<> class was modified to print "Lock" and "Unlock" to show exactly where the locking happens

```C++
struct A { int x = 7; double y = 2.2; };

void change_a( core::locked<A>&& locked_a ) {
    // and remember, no lock-guard
    locked_a->x += 1;
    locked_a->y = 3.14;
    // the gateway object (here, `locked_a`) will unlock() the mutex when going out of scope
}

int main() {
    core::access<A> a {A()};
    
    // (1) Smallest lock granularity: 
    //     This way we lock on each `->` access
    std::cerr << "==========\n";
    // Lock
    std::cout << a->x << "\n";
    // Unlock

    // Lock
    std::cout << a->y << "\n";
    // Unlock 
    std::cerr << "==========\n";

    std::cerr << "\nVS\n";
    change_a( a.lock() ); // safely changing `a` through the gateway object 

    // (2) Locking the object for the lifetime of the gateway object:
    {
        std::cerr << "==========\n";
        auto gateway = a.lock();
        // ===== access ======
        std::cout << gateway->x << "\n";
        std::cout << gateway->y << "\n";
    }
    std::cerr << "==========\n";
}
```

    output: 
    ==========
    Lock
    7
    Unlock
    Lock
    2.2
    Unlock
    ==========

    VS
    Lock
    ...change...
    ...auto-unlock...
    Unlock
    ==========
    Lock
    8
    3.14
    Unlock
    ==========


A simple (and pretty slow) example of incrementing a counter [it works, tho... unlike plain int :) ]
```C++
core::access<int> counter {0};
core::access<bool> flag {false};

{
    std::vector< core::thread > threads;
    using core::range;
    for (auto i : range(10)) {
        threads.emplace_back([&]{ 
            while( !flag.lock() ) { /* spin-wait */ }
            for (auto i : range(100)){ counter.lock()+=1; } 
        });
    }

    *flag.lock() = true;
}

std::cout << counter.lock() << "\n";

```


## core::device::CPU
> Used to access the information about CPU (including the p-cores/e-cores counts, cacheline size, e.t.c)
> core::device::CPU::cacheline_size() uses std::minimum_hardware_interference() if possible and supported by the compiler. Otherwise it uses the most common fallbacks as written below.

Supported OSes:
- Apple Mac OS: using sysctl
- POSIX-compliant OSes: using minimum sysctl subset (ncpus, endianness)

Otherwise, the fallback provides only the information that comes in the std.

> cacheline_size() is constexpr and thus somewhat speculative :) 
> but as a rule of thumb x86-64 has a standard cacheline size of 64 bytes. Where things get a little more tricky is new Apple's harware with 128-byte cachelines. This case is thus hardcoded. The question of determining cacheline sizes @ compile-time is a nice one and as far as I can see it hasn't been properly addressed in many compilers to this day, e.g. my apple Clang compiler doesn't have the std::max_hardware_interference, thus the need for core::device::CPU::cacheline_size() is pretty much justified by that observation. 


TODO:
- Add support for Intel's 12th Gen hybrid CPUs
- Add Windows support
- Add Linux support 
- Add OS-independent fallback using x86 cpuid


```C++
#include <iostream>
#include "core/cpu.hpp"

int main() {
    std::cout << "#cpus: " << core::device::CPU::n_cores() << "\n";
    std::cout << "cacheLineSize: " << core::device::CPU::cacheline_size << "\n"; // -> constexpr int64
    std::cout << "RAM: " << core::device::CPU::ram() << "GB \n";
    std::cout << "hybrid cores: " << core::device::CPU::has_hybrid_cores() << "\n";

    if (core::device::CPU::has_hybrid_cores()) {
        std::cout << "e-cores: " << core::device::CPU::e_cores() << "\n";
        std::cout << "p-cores: " << core::device::CPU::p_cores() << "\n";
    }

    std::cout << std::boolalpha;
    std::cout << "neon: " << core::device::CPU::ext_neon() << "\n";
}
```


## threadsafe/queue: 
- b_mpmc - bounded mpmc tagged-ring-buffer queue.
- mutex_queue - a general-purpose queue using std::queue and mutex
- spsc_queue - fast bounded Single Producer Single Consumer Queue
- unbounded_spsc_queue - fast unbounded Single Producer Single Consumer Queue

> Preliminary performance tests of both `spsc_queue` and `unbounded_spsc_queue` show some great performance characteristics. (Benchmark results and code will be published soon)


## threadsafe queue example (using D.Vyukov's Queue from 1024cores as a great example of Bounded MPMC)
```
//! Queue Simple Benchmark

#include <iostream>
#include "core/thread.hpp"
#include "core/range.hpp"
#include "core/timing.hpp"
#include "core/threadsafe/bounded_mpmc.hpp"

int main() {
    using core::timing::ms;

    B_MPMC_Queue<size_t, 1'048'576> q; // size passed as a template param to simplify things a bit:
    // since the compiler will use the bit-mask for modulo all by itself...

    constexpr size_t N = 1'000'000;
    size_t s1, s2;

    {// threads

        core::thread t1 = [&q] {
            for (size_t i : core::range(N)) {
                q.push(1);
            }
            q.close();
        };

        core::thread t2 = [&] {
            auto t = core::timeit([&]{
                size_t sum = 0;
                size_t v;
                
                // simpler syntax
                while (q) {
                    if ( q.try_pop(v) ) {
                        sum += v;
                    } 
                }
                s1 = sum;
            });
            std::cout << "time:" << t.in<ms>() << "\n";
        };

        core::thread t3 = [&] {
            size_t sum = 0;
            size_t v;

            // faster:
            while (true) {
                if ( q.try_pop(v) ) {
                    sum += v;
                } else 
                if (!q) break;
            }
            s2 = sum;
        };

    } // threads joined here

    std::cout << "\n" << s1 + s2;
}
```


## core::apply:
> #include "apply.hpp"

Allows to apply function objects to any objects that implement the fmap

Currently supports iterating over std::vector, std::list, std::deque and many other iterables & even std::tuple

```C++
using namespace core::lambda;
using core::apply;

// Types supported both in-place and returning new iterable:
// [uncomment any one and all of the below code will compile & work]
// auto v = std::deque<int> {1,2,3,4,5};
// auto v = std::list<int> {1,2,3,4,5};
// auto v = std::vector<int> {1,2,3,4,5};
auto v = std::make_tuple(1,2,3,4,5);
// Only in-place:
// auto v = std::array<int, 5> {1,2,3,4,5};

auto printer = [](auto && e){std::cout << e << ", ";};

// Examples:
// 
//      +--lvalue 
//      |    |   
auto&& ret = v | apply(printer);
//             |
//         fmap( lvalue, in-place )   ->   lvalue
//                                           |
std::cout << "Ret: " << core::Type<decltype(ret)> << "\n\n";


//         lvalue
//           |     
auto && v2 = v | apply($a * 2) | apply(printer);
//       |          |
//       |   lvalue |  create new   ->   rvalue
//      rvalue                             |
std::cout << "V2: " << core::Type<decltype(v2)> << "\n\n";


auto w = v;

// apply to temporary
//        +------------rvalue
//        |              |
auto&& new_v = std::move(w) | apply([](auto && v){ v = v * 3; }) | apply(printer);
//                          |
//               fmap(rvalue, in-place)  ->  rvalue
//                                             |
std::cout << "new_v: " << core::Type<decltype(new_v)> << "\n\n";

w = v;
// apply to temporary + create new (should move from temporary when possible)
//         +------------rvalue
//         |              |
auto&& new_v2 = std::move(v) | apply($a * 4) | apply(printer);
//                           |                   |
//                fmap(rvalue, create new)  ->  rvalue
//                                               |
std::cout << "new_v2: " << core::Type<decltype(new_v2)> << "\n\n";
```
