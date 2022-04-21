#pragma once

#include <iterator>

#include "../meta.hpp"
#include "../range.hpp"
#include "../macros.hpp"

namespace core {

// Is iterable?
template <class Iter, class IsIterable = meta::check_valid>
struct is_iterable : std::false_type {};

template <class Iter>
struct is_iterable<Iter, meta::is_valid<
    decltype( std::begin(std::declval<Iter>()) ),
    decltype( std::end(std::declval<Iter>()) )
>> 
: std::true_type {};

template <class Iter>
CORE_CPP17_INLINE_VARIABLE
constexpr auto is_iterable_v = is_iterable<Iter>::value;


// Is indexable via operator[Index] ?
template <class Iter, typename Index, class HasIndexingOperator = meta::check_valid>
struct is_indexable_with 
: std::false_type {};

template <class Iter, typename Index>
struct is_indexable_with<Iter, Index, meta::is_valid<decltype( std::declval<Iter>()[ std::declval<Index>() ] )>> 
: std::true_type {
    using type = decltype( std::declval<Iter>()[ std::declval<Index>() ] ); // Iter[Index] -> type
};

template <class Iter, typename Index>
CORE_CPP17_INLINE_VARIABLE
constexpr auto is_indexable_with_v = is_indexable_with<Iter, Index>::value;


// has size() class method?
template <class T, class HasSizeMethod = meta::check_valid>
struct has_size 
: std::false_type {};

template <class T>
struct has_size<T, meta::is_valid<decltype( std::declval<T>().size() )>> 
: std::true_type {
    // T.size() -> type:
    using type = decltype( std::declval<T>().size() );
};

template <class T>
CORE_CPP17_INLINE_VARIABLE
constexpr auto has_size_v = has_size<T>::value;

} // namespace core