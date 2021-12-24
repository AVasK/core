# CORE

## core::typesystem

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
```

- you can print the types!
```C++
std::cout << Type<float&> << "\n" //>>> float&
          << type(x) << "\n";     //>>> int& (type gets the type w.r.t l- and r-valuedness)
```
    the PRETTY_FUNCTION of clang/gcc is used for printing


- You can test different predicates for your types...
    as a matter of fact, there're pre-written predicates that almost 100% mimick the std::type_traits ones!

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

```C++
Type<A>.match(
    is_integral         >>  transform< /* Any metafunction, including ones from std type_traits */ >,
    is<void>            >>  Type< example::EmptyType >,
    is_lvalue_reference >>  transform< std::remove_reference >,
    otherwise           >>  ... // otherwise is an optional clause, like 'default' case in switch.
) 
-> Type< /*whatever the resulting type is after the transformation*/ >
```