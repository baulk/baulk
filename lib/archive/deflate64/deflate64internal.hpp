///
#ifndef DEFLATE64_INTERNAL_HPP
#define DEFLATE64_INTERNAL_HPP
#include <cstdint>
#include <archive.hpp>

namespace baulk::archive::flate {
//
struct code {
  uint8_t op;
  uint8_t bits;
  uint16_t val;
};
}; // namespace baulk::archive::flate

#endif