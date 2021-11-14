#include "core/meta_core.hpp"
#include "core/typesystem.hpp"
#include "core/lambda.hpp"

using namespace core::typesystem;
// using core::Type;

// template <typename T,
//     typename = std::enable_if_t<
//         std::is_integral_v<T> || std::is_same_v<T,bool>
//     >
// >
// void f(T value) { std::cout << "f\n"; }


// using namespace core::type_predicates;
template <typename T,
    typename = std::enable_if_t<
        TypeOf<T>{}(is_integral | is<bool>)
    >
>
void g(T value) noexcept(TypeOf<T>{}(is_nothrow_default_constructible))
{ std::cout << "g\n"; }


int main() {

    using T1 = meta::append< meta::typelist<int, float>, double>;
    using T2 = meta::prepend< meta::typelist<float, double>, int>;
    // std::cout << Type<T1> << "\n";
    // static_assert(Type<T1> == Type<T2>, "...");
        
    using namespace core::lambda;
    auto cmp = $a < $b;

    // // using namespace core::type_predicates;
    // using namespace core::typesystem;
    // static_assert( 
    //     // Type<T1> == Type< meta::typelist<int,float,double> > 
    //     // &&
    //     Type<T1>( 
    //         is< meta::typelist<int,float,double> > 
    //         &
    //         is_default_constructible 
    //         &
    //         !core::is_void
    //     )
    //     && 
    //     Type<T2>(is_empty)
    //     ,
    //  "OOPS"
    // );

}