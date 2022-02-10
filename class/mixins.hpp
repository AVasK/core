#pragma once

namespace core {
namespace mixin {

/////////////// [ MACRO ] ///////////////
#define CORE_MIXIN(name, code)          \
template <class Base>                   \
struct name : public Base {             \
    using Base::Base;                   \
    code                                \
}                                       \
  

/////////////// [ MIXINS ] //////////////

/**
 * @brief provides upper-most Base class to the mixin hierarchy
 *        via typedef `Super` 
 *        (whereas Base provides only the direct base for each mixin)
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
