// gzip
#ifndef BAULK_GZIP_HPP
#define BAULK_GZIP_HPP
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <memory>
#include <bela/fmt.hpp>
#include "zlib.hpp"

namespace baulk::archive::gzip {
class Decompressor {
public:
  Decompressor() = default;
  Decompressor(const Decompressor &) = delete;
  Decompressor &operator=(const Decompressor &) = delete;
  ~Decompressor() {
    if (initialized_) {
      inflateEnd(&stream_);
    }
  }
  bool receive(char *data, size_t datalen) {
    if (datalen == 0) {
      return false;
    }
    stream_.avail_in = datalen;
    stream_.next_in = reinterpret_cast<Bytef *>(data);
    return true;
  }
  bela::ssize_t decompress() {
    stream_.avail_out = bufsize_;
    stream_.next_out = reinterpret_cast<Bytef *>(refbuf_);
    if (stream_.avail_in == 0) {
      /// need more data
      return 0;
    }
    auto ret = inflate(&stream_, Z_NO_FLUSH);
    if (ret != Z_OK && ret != Z_STREAM_END) {
      return -1;
    }
    lastbytes_ = bufsize_ - stream_.avail_out;
    return lastbytes_;
  }
  /// completed not input data
  bool availzero() { return (stream_.avail_out == 0); }
  static Decompressor *New(char *buf, size_t buflen) {
    auto decor = new Decompressor();
    if (decor == nullptr) {
      return nullptr;
    }
    decor->refbuf_ = buf;
    decor->bufsize_ = buflen;
    memset(&(decor->stream_), 0, sizeof(z_stream));
    /// WINDOW_BITS 15
    //+32
    if (inflateInit2(&(decor->stream_), 47) != Z_OK) {
      delete decor;
      return nullptr;
    }
    decor->initialized_ = true;
    return decor;
  }

private:
  z_stream stream_; // zstream
  char *refbuf_;
  size_t bufsize_;
  size_t lastbytes_{0};
  bool initialized_{false};
};

} // namespace baulk::archive::gzip
#endif