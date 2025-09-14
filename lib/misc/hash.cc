//
#include <bela/base.hpp>
#include <bela/endian.hpp>
#include <bela/match.hpp>
#include <bela/hash.hpp>
#include <bela/ascii.hpp>
#include <baulk/hash.hpp>

namespace baulk::hash {

template <typename Hasher> struct Sumizer {
  Hasher hasher;
  bool filechecksum(const std::filesystem::path &file, std::wstring &hv, bela::error_code &ec) {
    HANDLE FileHandle = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
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
  std::optional<std::wstring> operator()(const std::filesystem::path &file, bela::error_code &ec) {
    std::wstring hv;
    if (filechecksum(file, hv, ec)) {
      return std::make_optional(std::move(hv));
    }
    return std::nullopt;
  }
};

std::optional<std::wstring> FileHash(const std::filesystem::path &file, hash_t method, bela::error_code &ec) {
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
  ec = bela::make_error_code(bela::ErrGeneral, L"unkown hash method: ", static_cast<int>(method));
  return std::nullopt;
}

struct HashPrefix {
  const std::wstring_view prefix;
  hash_t method;
};
constexpr HashPrefix hnmaps[] = {
    {.prefix = L"BLAKE3", .method = hash_t::BLAKE3},     // BLAKE3
    {.prefix = L"SHA224", .method = hash_t::SHA224},     // SHA224
    {.prefix = L"SHA256", .method = hash_t::SHA256},     // SHA256
    {.prefix = L"SHA384", .method = hash_t::SHA384},     // SHA384
    {.prefix = L"SHA512", .method = hash_t::SHA512},     // SHA512
    {.prefix = L"SHA3-224", .method = hash_t::SHA3_224}, // SHA3-224
    {.prefix = L"SHA3-256", .method = hash_t::SHA3_256}, // SHA3-256
    {.prefix = L"SHA3-384", .method = hash_t::SHA3_384}, // SHA3-384
    {.prefix = L"SHA3-512", .method = hash_t::SHA3_512}, // SHA3-512
    {.prefix = L"SHA3", .method = hash_t::SHA3},         // SHA3 alias for SHA3-256
};
bool HashEqual(const std::filesystem::path &file, std::wstring_view hash_value, bela::error_code &ec) {
  std::wstring_view value = hash_value;
  auto m = hash_t::SHA256;
  if (auto pos = hash_value.find(':'); pos != std::wstring_view::npos) {
    value = hash_value.substr(pos + 1);
    auto prefix = bela::AsciiStrToUpper(hash_value.substr(0, pos));
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
      ec = bela::make_error_code(bela::ErrGeneral, L"unsupported hash method '", prefix, L"'");
      return false;
    }
  }
  auto ha = FileHash(file, m, ec);
  if (!ha) {
    return false;
  }
  if (!bela::EndsWithIgnoreCase(*ha, value)) {
    ec = bela::make_error_code(bela::ErrGeneral, L"checksum mismatch expected ", value, L" actual ", *ha);
    return false;
  }
  return true;
}

std::optional<file_hash_sums> HashSums(const std::filesystem::path &file, bela::error_code &ec) {
  HANDLE FileHandle = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  bela::hash::sha256::Hasher s;
  bela::hash::blake3::Hasher b;
  s.Initialize();
  b.Initialize();
  auto closer = bela::finally([&] { CloseHandle(FileHandle); });
  uint8_t bytes[32678];
  for (;;) {
    DWORD dwread = 0;
    if (ReadFile(FileHandle, bytes, sizeof(bytes), &dwread, nullptr) != TRUE) {
      ec = bela::make_system_error_code();
      return std::nullopt;
    }
    s.Update(bytes, static_cast<size_t>(dwread));
    b.Update(bytes, static_cast<size_t>(dwread));
    if (dwread < sizeof(bytes)) {
      break;
    }
  }
  return std::make_optional(file_hash_sums{.sha256sum = s.Finalize(), .blake3sum = b.Finalize()});
}

} // namespace baulk::hash
