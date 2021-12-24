# CORE

## core::typesystem

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