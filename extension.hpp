// Probably easier to just inherit + using Base::Base + write some ext.code...

template <class...> struct null_v : std::integral_constant<int, 0> {};

template <class Base, template<class> class... Extensions>
class extend : public Base, public Extensions<Base>... {
public:

    // using Base::Base; 

    template <typename T, long= null_v<std::enable_if_t<std::is_convertible<T, Base>::value>>::value >
    constexpr extend(T&& arg) : Base{std::forward<T>(arg)}, Extensions<Base>{ static_cast<Base&>(*this) }... {}

    template <typename T, typename = std::enable_if_t<!std::is_convertible<T, Base>::value> >
    explicit constexpr extend(T&& arg) : Base{std::forward<T>(arg)}, Extensions<Base>{ static_cast<Base&>(*this) }... {}

    template <typename... Ts, typename = std::enable_if_t<(sizeof...(Ts)!=1) && std::is_constructible<Base, Ts...>::value> >
    constexpr extend(Ts&&... args) : Base{std::forward<Ts>(args)...}, Extensions<Base>{ static_cast<Base&>(*this) }... {}

    template <typename T, typename= std::enable_if_t<std::is_constructible<Base, std::initializer_list<T>>::value> >
    constexpr extend(std::initializer_list<T> list) : Base(list), Extensions<Base>{ static_cast<Base&>(*this) }... {}

    operator Base() {return *this;}
};


// Example:

// template <class T>
// struct range_ext {
//     T& self;
//     range_ext(T& ref) : self{ref} {}

//     auto operator[] (range_type range) -> typename T::value_type {
//         std::cout << "vector ["<< range.start << " : " << range.end <<"] = ";
//         for (size_t i=range.start; i < range.end; ++i) {
//             std::cout << self[i] << ", ";
//         }
//         std::cout << "\n";
//         return {};
//     }

//     void operator() (int s, int e) {
//         operator[](range_type(s,e));
//     }
// };



// template <typename T, class Alloc=std::allocator<T>>
// using vex = extend< std::vector<T,Alloc>, range_ext >;