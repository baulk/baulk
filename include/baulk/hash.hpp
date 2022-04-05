//
#ifndef BAULK_HASH_HPP
#define BAULK_HASH_HPP
#include <bela/base.hpp>

namespace baulk::hash {
enum class hash_t {
  SHA224,   //
  SHA256,   //
  SHA384,   //
  SHA512,   //
  SHA3,     //
  SHA3_224, //
  SHA3_256, //
  SHA3_384, //
  SHA3_512, //
  BLAKE3
};
bool HashEqual(std::wstring_view file, std::wstring_view hash_value, bela::error_code &ec);
std::optional<std::wstring> FileHash(std::wstring_view file, hash_t method, bela::error_code &ec);
struct file_hash_sums {
  std::wstring sha256sum;
  std::wstring blake3sum;
};
std::optional<file_hash_sums> HashSums(std::wstring_view file, bela::error_code &ec);
} // namespace baulk::hash

#endif