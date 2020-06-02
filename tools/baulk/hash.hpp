//
#ifndef BAULK_HASH_HPP
#define BAULK_HASH_HPP
#include <bela/base.hpp>

namespace baulk::hash {
enum HashMethod { SHA256, BLAKE3 };
bool HashEqual(std::wstring_view file, std::wstring_view hashvalue, bela::error_code &ec);
std::optional<std::wstring> FileHash(std::wstring_view file, HashMethod method,
                                     bela::error_code &ec);
inline std::optional<std::wstring> FileHashBLAKE3(std::wstring_view file, bela::error_code &ec) {
  return FileHash(file, HashMethod::BLAKE3, ec);
}
inline std::optional<std::wstring> FileHashSHA256(std::wstring_view file, bela::error_code &ec) {
  return FileHash(file, HashMethod::SHA256, ec);
}
} // namespace baulk::hash

#endif