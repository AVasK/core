#pragma once

#include "inheritance.hpp"

namespace core {

template <class Base, template<class> class... Mixins>
using extend = chain<Base, Mixins...>;

#define CORE_MIXIN(name, code)          \
template <class Base>                   \
struct name : public Base {             \
    using Base::Base;                   \
    code                                \
}                                       \
    

namespace mixin {

/**
 * @brief provides ultimate Base class to the mixin hierarchy
 *        via typedef `Super`
 * @remark Should go first in the extension mixin list
 * 
 * @tparam Base 
 */
template <class Base>
struct SuperMixin : public Base {
    using Base::Base;
    using Super = Base;
};
}//namespace mixin

}//namespace core