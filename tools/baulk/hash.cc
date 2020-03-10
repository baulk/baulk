//
#include <bela/finaly.hpp>
#include <blake3.h>
#include "hash.hpp"

namespace baulk::hash {

inline void HashEncode(const unsigned char *b, size_t len, std::wstring &hv) {
  hv.resize(len * 2);
  auto p = hv.data();
  constexpr char hex[] = "0123456789abcdef";
  for (size_t i = 0; i < len; i++) {
    unsigned int val = b[i];
    *p++ = hex[val >> 4];
    *p++ = hex[val & 0xf];
  }
}

bool b3sum(std::wstring_view file, std::wstring &hv, bela::error_code &ec) {
  blake3_hasher ctx;
  blake3_hasher_init(&ctx);
  HANDLE FileHandle =
      CreateFileW(file.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  auto deleter = bela::finally([&] { CloseHandle(FileHandle); });
  char buffer[8192];
  for (;;) {
    DWORD dwread = 0;
    if (ReadFile(FileHandle, buffer, sizeof(buffer), &dwread, nullptr) !=
        TRUE) {
      ec = bela::make_system_error_code();
      return false;
    }
    blake3_hasher_update(&ctx, buffer, static_cast<size_t>(dwread));
    if (dwread < sizeof(buffer)) {
      break;
    }
  }
  unsigned char buf[BLAKE3_OUT_LEN];
  blake3_hasher_finalize(&ctx, buf, BLAKE3_OUT_LEN);
  HashEncode(buf, BLAKE3_OUT_LEN, hv);
  return true;
}
struct HashPrefix {
  const std::wstring_view prefix;
  HashMethod method;
};
bool HashEqual(std::wstring_view file, std::wstring_view hashvalue,
               bela::error_code &ec) {
  constexpr HashPrefix hv[] = {{L"BLAKE3:", HashMethod::SHA256},
                               {L"SHA256:", HashMethod::SHA256}};
  return false;
}

std::optional<std::wstring> FileHash(std::wstring_view file, HashMethod method,
                                     bela::error_code &ec) {
  std::wstring hv;
  switch (method) {
  case HashMethod::SHA256:
    break;
  case HashMethod::BLAKE3:
    if (b3sum(file, hv, ec)) {
      return std::make_optional(std::move(hv));
    }
    return std::nullopt;
  default:
    break;
  }
  ec = bela::make_error_code(1, L"unkown hash method: ",
                             static_cast<int>(method));
  return std::nullopt;
}

} // namespace baulk::hash