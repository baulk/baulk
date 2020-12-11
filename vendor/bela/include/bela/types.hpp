///
#ifndef BELA_TYPES_HPP
#define BELA_TYPES_HPP
#include <cstddef>

namespace bela {
#ifndef __BELA__SSIZE_DEFINED_T
#define __BELA__SSIZE_DEFINED_T
using __bela__ssize_t = std::ptrdiff_t;
#endif
using ssize_t = __bela__ssize_t;

} // namespace bela

#endif