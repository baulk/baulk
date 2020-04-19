#ifndef BAULK_ZLIB_HPP
#define BAULK_ZLIB_HPP

#if defined(_M_X64) || defined(_M_X86)
#include "zlib-intel/zlib.h"
#else
#include "zlib/zlib.h"
#endif

namespace baulk::archive::zlib {
//
}

#endif