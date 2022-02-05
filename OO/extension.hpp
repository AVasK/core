#pragma once

#include "inheritance.hpp"

namespace core {

template <class Base, template<class> class... Mixins>
using extend = chain<Base, Mixins...>;

#define CORE_MIXIN(name, code)          \
template <class Base>                   \
struct name : public Base {             \
    using Super = Base;                 \
    using Base::Base;                   \
    code                                \
}                                       \
    


}//namespace core