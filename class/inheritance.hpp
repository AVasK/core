#pragma once

namespace core {

// Inherit all
template <class... Bases>
class inherit : public Bases... { using Bases::Bases...; };


// Pairwise inheritance
template <class... Bases>
class inherit_pairwise;

template <class B, class... Bases>
class inherit_pairwise<B,Bases...> : public B, public inherit_pairwise<Bases...> {
    using LBase = B;
    using RBase = inherit_pairwise<Bases...>;

    using LBase::LBase;
    using RBase::RBase;
};

template <class L, class R>
class inherit_pairwise<L,R> : public L, public R {
    using L::L;
    using R::R;
};


// Reverse Chain: Link1 <- ... <- LinkN <- Base
template <class Base, template<class> class... Chain>
class reverse_chain;

template <class Base, template<class> class Head, template<class> class... Tail>
class reverse_chain<Base, Head,Tail...> : public Head< reverse_chain<Base, Tail...> > {
    using X = Head< reverse_chain<Base,Tail...> >;
    using X::X;
};

template <class Base, template<class> class Link>
class reverse_chain<Base, Link> : public Link<Base> {
    using X = Link<Base>;
    using X::X;
};

} //  namespace core