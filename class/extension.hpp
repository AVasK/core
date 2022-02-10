#pragma once

namespace core {

/////////////////// [ EXTEND ] /////////////////////
// Linear Chain: extend<Base, M1,M2> --> M2<M1<Base>>
template <class Base, template<class> class... Mixins>
class extend;

template <class Base, template<class> class C, template<class> class... Cs>
class extend<Base, C,Cs...> : public extend<C<Base>, Cs...> {
    using Super = extend<C<Base>, Cs...>;
    using Super::Super; // superclass c'tors
};

template <class Base>
class extend<Base> : public Base {
    using Base::Base;
};
  

} // namespace core