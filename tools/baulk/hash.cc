//
#include <bela/base.hpp>
#include <bela/endian.hpp>
#include <bela/match.hpp>
#include <bela/hash.hpp>
#include <bela/ascii.hpp>
#include "hash.hpp"

namespace baulk::hash {

template <typename Hasher> struct Sumizer {
  Hasher hasher;
  bool filechecksum(std::wstring_view file, std::wstring &hv, bela::error_code &ec) {
    HANDLE FileHandle = CreateFileW(file.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (FileHandle == INVALID_HANDLE_VALUE) {
      ec = bela::make_system_error_code();
      return false;
    }
    auto closer = bela::finally([&] { CloseHandle(FileHandle); });
    uint8_t bytes[32678];
    for (;;) {
      DWORD dwread = 0;
      if (ReadFile(FileHandle, bytes, sizeof(bytes), &dwread, nullptr) != TRUE) {
        ec = bela::make_system_error_code();
        return false;
      }
      hasher.Update(bytes, static_cast<size_t>(dwread));
      if (dwread < sizeof(bytes)) {
        break;
      }
    }
    hv = hasher.Finalize();
    return true;
  }
  std::optional<std::wstring> operator()(std::wstring_view file, bela::error_code &ec) {
    std::wstring hv;
    if (filechecksum(file, hv, ec)) {
      return std::make_optional(std::move(hv));
    }
    return std::nullopt;
  }
};

std::optional<std::wstring> FileHash(std::wstring_view file, hash_t method, bela::error_code &ec) {
  switch (method) {
  case hash_t::SHA224: {
    Sumizer<bela::hash::sha256::Hasher> sumizer;
    sumizer.hasher.Initialize(bela::hash::sha256::HashBits::SHA224);
    return sumizer(file, ec);
  }
  case hash_t::SHA256: {
    Sumizer<bela::hash::sha256::Hasher> sumizer;
    sumizer.hasher.Initialize();
    return sumizer(file, ec);
  }
  case hash_t::SHA384: {
    Sumizer<bela::hash::sha512::Hasher> sumizer;
    sumizer.hasher.Initialize(bela::hash::sha512::HashBits::SHA384);
    return sumizer(file, ec);
  }
  case hash_t::SHA512: {
    Sumizer<bela::hash::sha512::Hasher> sumizer;
    sumizer.hasher.Initialize();
    return sumizer(file, ec);
  }
  case hash_t::SHA3_224: {
    Sumizer<bela::hash::sha3::Hasher> sumizer;
    sumizer.hasher.Initialize(bela::hash::sha3::HashBits::SHA3224);
    return sumizer(file, ec);
  }
  case hash_t::SHA3_256:
    [[fallthrough]];
  case hash_t::SHA3: {
    Sumizer<bela::hash::sha3::Hasher> sumizer;
    sumizer.hasher.Initialize();
    return sumizer(file, ec);
  }
  case hash_t::SHA3_384: {
    Sumizer<bela::hash::sha3::Hasher> sumizer;
    sumizer.hasher.Initialize(bela::hash::sha3::HashBits::SHA3384);
    return sumizer(file, ec);
  }
  case hash_t::SHA3_512: {
    Sumizer<bela::hash::sha3::Hasher> sumizer;
    sumizer.hasher.Initialize(bela::hash::sha3::HashBits::SHA3512);
    return sumizer(file, ec);
  }
  case hash_t::BLAKE3: {
    Sumizer<bela::hash::blake3::Hasher> sumizer;
    sumizer.hasher.Initialize();
    return sumizer(file, ec);
  }
  default:
    break;
  }
  ec = bela::make_error_code(1, L"unkown hash method: ", static_cast<int>(method));
  return std::nullopt;
}

struct HashPrefix {
  const std::wstring_view prefix;
  hash_t method;
};
constexpr HashPrefix hnmaps[] = {
    {L"BLAKE3", hash_t::BLAKE3},     // BLAKE3
    {L"SHA224", hash_t::SHA224},     // SHA224
    {L"SHA256", hash_t::SHA256},     // SHA256
    {L"SHA384", hash_t::SHA384},     // SHA384
    {L"SHA512", hash_t::SHA512},     // SHA512
    {L"SHA3-224", hash_t::SHA3_224}, // SHA3-224
    {L"SHA3-256", hash_t::SHA3_256}, // SHA3-256
    {L"SHA3-384", hash_t::SHA3_384}, // SHA3-384
    {L"SHA3-512", hash_t::SHA3_512}, // SHA3-512
    {L"SHA3", hash_t::SHA3},         // SHA3 alias for SHA3-256
};
bool HashEqual(std::wstring_view file, std::wstring_view hashvalue, bela::error_code &ec) {

  std::wstring_view value = hashvalue;
  auto m = hash_t::SHA256;
  if (auto pos = hashvalue.find(':'); pos != std::wstring_view::npos) {
    value = hashvalue.substr(pos + 1);
    auto prefix = bela::AsciiStrToUpper(hashvalue.substr(0, pos));
    auto fn = [&]() {
      for (const auto &h : hnmaps) {
        if (h.prefix == prefix) {
          m = h.method;
          return true;
        }
      }
      return false;
    };
    if (!fn()) {
      ec = bela::make_error_code(1, L"unsupported hash method '", prefix, L"'");
      return false;
    }
  }
  auto ha = FileHash(file, m, ec);
  if (!ha) {
    return false;
  }
  if (!bela::EndsWithIgnoreCase(*ha, value)) {
    ec = bela::make_error_code(1, L"checksum mismatch expected ", value, L" actual ", *ha);
    return false;
  }
  return true;
}

} // namespace baulk::hash
