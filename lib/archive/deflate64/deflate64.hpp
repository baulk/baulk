//
#ifndef DEFLATE64_HPP
#define DEFLATE64_HPP
#include <cstdint>
#include <archive.hpp>
#include <zlib.h>

namespace baulk::archive::flate {
// deflate64 inflate
class Decompressor {
public:
  Decompressor() = default;
  bool Initialize();
  bool Do();
private:
  z_stream zs;
};

}; // namespace baulk::archive::flate

#endif