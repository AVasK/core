# CORE

## core::typesystem
```C++
using namespace core::typesystem
```

### Type:
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
constexpr auto tx = type(x);
static_assert( t1( is_lvalue_reference ), "");

constexpr auto t1 = type(1);
static_assert( tx( is_rvalue_reference ), "");
```

- you can *print* the types!
```C++
std::cout << Type<float&> << "\n" //>>> float&
          << type(x) << "\n";     //>>> int& (type gets the type w.r.t l- and r-valuedness)
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

- Finally, you can do *PATTERN MATCHING* on types! 
```C++
Type<A>.match(
    is_integral         >>  transform< /* Any metafunction, including ones from std type_traits */ >,
    is<void>            >>  Type< example::EmptyType >,
    is_lvalue_reference >>  transform< std::remove_reference >,
    otherwise           >>  ... // otherwise is an optional clause, like 'default' case in switch.
) 
-> Type< /*whatever the resulting type is after the transformation*/ >
```
    the `.match` uses the following syntax: 
    `predicate >> [transform | Type]`
    the predicate should be created via bind_predicate or rbind_predicate.

### Types:
```C++
struct TypeList<Ts...> 
Types<Ts...>  <->  TypeList<Ts...>( );
```

You can:
- apply same transformations as for TypeOf 
```C++
Types<int, float>.add_const().add_lvalue_ref();
```

- test if *all*, *any* or *none* of the types satisfy some Predicate.
- transform your TypeList<...>
- filter it
- use pattern matching on it

### Examples:

Testing predicates

```C++
Types<int, long>.all( is_integral | is_floating_point )
```

Filter, then Pattern matching
```C++
constexpr auto ts = Types<int, float, void, double, bool>.filter(!is_void).match(
    is<void>                    >>  Type<char>,
    is_lvalue_reference         >>  transform<std::remove_reference>,
    (is_integral & !is<bool>)   >>  transform<std::make_unsigned>
); // -> Types< unsigned int, float, double, bool >;
```

## core::lambda
```C++
using namespace core::lambda;
```

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
    no inner namespace

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