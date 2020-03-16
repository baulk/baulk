//
#include <bela/finaly.hpp>
#include <bela/endian.hpp>
#include <bela/match.hpp>
#include <blake3.h>
#include "hash.hpp"

#if defined(_MSC_VER)

/* instrinsic rotate */
#include <cstdlib>
#pragma intrinsic(_rotr, _rotl)
#define ROR(x, n) _rotr(x, n)
#define ROL(x, n) _rotl(x, n)
#define RORc(x, n) ROR(x, n)
#define ROLc(x, n) ROL(x, n)

#elif defined(__clang__) || defined(__GNUC__)

#define ROR(x, n) __builtin_rotateright32(x, n)
#define ROL(x, n) __builtin_rotateleft32(x, n)
#define ROLc(x, n) ROL(x, n)
#define RORc(x, n) ROR(x, n)

#endif

namespace baulk::hash {
namespace sha256 {
// https://github.com/libtom/libtomcrypt/blob/develop/src/hashes/sha2/sha256.c
static constexpr const unsigned long K[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL,
    0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL, 0xd807aa98UL, 0x12835b01UL,
    0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL,
    0xc19bf174UL, 0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL, 0x983e5152UL,
    0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL,
    0x06ca6351UL, 0x14292967UL, 0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL,
    0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL,
    0xd6990624UL, 0xf40e3585UL, 0x106aa070UL, 0x19a4c116UL, 0x1e376c08UL,
    0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL,
    0x682e6ff3UL, 0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL};

/* Various logical functions */
#define Ch(x, y, z) (z ^ (x & (y ^ z)))
#define Maj(x, y, z) (((x | y) & z) | (x & y))
#define S(x, n) RORc((x), (n))
#define R(x, n) (((x)&0xFFFFFFFFUL) >> (n))
#define Sigma0(x) (S(x, 2) ^ S(x, 13) ^ S(x, 22))
#define Sigma1(x) (S(x, 6) ^ S(x, 11) ^ S(x, 25))
#define Gamma0(x) (S(x, 7) ^ S(x, 18) ^ R(x, 3))
#define Gamma1(x) (S(x, 17) ^ S(x, 19) ^ R(x, 10))

static inline void EncodeBigEndian(void *ptr, uint32_t value) {
  uint8_t *p = reinterpret_cast<uint8_t *>(ptr);
  p[0] = value >> 24;
  p[1] = value >> 16;
  p[2] = value >> 8;
  p[3] = value >> 0;
}

constexpr size_t sha256_block_size = 64;
constexpr size_t sha256_hash_size = 32;
struct SHA256_CTX {
  uint32_t state[8];
  uint64_t size;
  uint32_t offset;
  uint8_t buf[sha256_block_size];
  void Initialize() {
    offset = 0;
    size = 0;
    state[0] = 0x6a09e667ul;
    state[1] = 0xbb67ae85ul;
    state[2] = 0x3c6ef372ul;
    state[3] = 0xa54ff53aul;
    state[4] = 0x510e527ful;
    state[5] = 0x9b05688cul;
    state[6] = 0x1f83d9abul;
    state[7] = 0x5be0cd19ul;
  }

  void Transform(const unsigned char *buf) {

    uint32_t S[8], W[64], t0, t1;
    int i;

    /* copy state into S */
    for (i = 0; i < 8; i++) {
      S[i] = state[i];
    }

    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++, buf += sizeof(uint32_t)) {
      W[i] = bela::readbe<uint32_t>(buf);
    }

    /* fill W[16..63] */
    for (i = 16; i < 64; i++) {
      W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
    }

#define RND(a, b, c, d, e, f, g, h, i, ki)                                     \
  t0 = h + Sigma1(e) + Ch(e, f, g) + ki + W[i];                                \
  t1 = Sigma0(a) + Maj(a, b, c);                                               \
  d += t0;                                                                     \
  h = t0 + t1;

    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 0, 0x428a2f98);
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 1, 0x71374491);
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 2, 0xb5c0fbcf);
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 3, 0xe9b5dba5);
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 4, 0x3956c25b);
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 5, 0x59f111f1);
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 6, 0x923f82a4);
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 7, 0xab1c5ed5);
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 8, 0xd807aa98);
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 9, 0x12835b01);
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 10, 0x243185be);
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 11, 0x550c7dc3);
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 12, 0x72be5d74);
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 13, 0x80deb1fe);
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 14, 0x9bdc06a7);
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 15, 0xc19bf174);
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 16, 0xe49b69c1);
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 17, 0xefbe4786);
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 18, 0x0fc19dc6);
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 19, 0x240ca1cc);
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 20, 0x2de92c6f);
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 21, 0x4a7484aa);
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 22, 0x5cb0a9dc);
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 23, 0x76f988da);
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 24, 0x983e5152);
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 25, 0xa831c66d);
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 26, 0xb00327c8);
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 27, 0xbf597fc7);
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 28, 0xc6e00bf3);
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 29, 0xd5a79147);
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 30, 0x06ca6351);
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 31, 0x14292967);
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 32, 0x27b70a85);
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 33, 0x2e1b2138);
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 34, 0x4d2c6dfc);
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 35, 0x53380d13);
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 36, 0x650a7354);
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 37, 0x766a0abb);
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 38, 0x81c2c92e);
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 39, 0x92722c85);
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 40, 0xa2bfe8a1);
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 41, 0xa81a664b);
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 42, 0xc24b8b70);
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 43, 0xc76c51a3);
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 44, 0xd192e819);
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 45, 0xd6990624);
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 46, 0xf40e3585);
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 47, 0x106aa070);
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 48, 0x19a4c116);
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 49, 0x1e376c08);
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 50, 0x2748774c);
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 51, 0x34b0bcb5);
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 52, 0x391c0cb3);
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 53, 0x4ed8aa4a);
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 54, 0x5b9cca4f);
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 55, 0x682e6ff3);
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 56, 0x748f82ee);
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 57, 0x78a5636f);
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 58, 0x84c87814);
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 59, 0x8cc70208);
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 60, 0x90befffa);
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 61, 0xa4506ceb);
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 62, 0xbef9a3f7);
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 63, 0xc67178f2);

    for (i = 0; i < 8; i++) {
      state[i] += S[i];
    }
  }

  void Update(const void *data, size_t len) {
    unsigned int len_buf = size & 63;
    size += len;

    /* Read the data into buf and process blocks as they get full */
    if (len_buf != 0) {
      unsigned int left = 64 - len_buf;
      if (len < left) {
        left = len;
      }
      memcpy(len_buf + buf, data, left);
      len_buf = (len_buf + left) & 63;
      len -= left;
      data = ((const char *)data + left);
      if (len_buf)
        return;
      Transform(buf);
    }
    while (len >= 64) {
      Transform(reinterpret_cast<const uint8_t *>(data));
      data = ((const char *)data + 64);
      len -= 64;
    }
    if (len != 0) {
      memcpy(buf, data, len);
    }
  }

  void Final(uint8_t out[32]) {
    static const unsigned char pad[64] = {0x80};
    unsigned int padlen[2];
    int i;

    /* Pad with a binary 1 (ie 0x80), then zeroes, then length */
    padlen[0] = bela::swapbe((uint32_t)(size >> 29));
    padlen[1] = bela::swapbe((uint32_t)(size << 3));

    i = size & 63;
    Update(pad, 1 + (63 & (55 - i)));
    Update(padlen, 8);
    for (i = 0; i < 8; i++, out += sizeof(uint32_t))
      EncodeBigEndian(out, state[i]);
  }
};

} // namespace sha256

inline void HashEncode(const uint8_t *b, size_t len, std::wstring &hv) {
  hv.resize(len * 2);
  auto p = hv.data();
  constexpr char hex[] = "0123456789abcdef";
  for (size_t i = 0; i < len; i++) {
    unsigned int val = b[i];
    *p++ = hex[val >> 4];
    *p++ = hex[val & 0xf];
  }
}

bool sha256sum(std::wstring_view file, std::wstring &hv, bela::error_code &ec) {
  HANDLE FileHandle =
      CreateFileW(file.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  auto deleter = bela::finally([&] { CloseHandle(FileHandle); });
  sha256::SHA256_CTX ctx;
  ctx.Initialize();
  char buffer[8192];
  for (;;) {
    DWORD dwread = 0;
    if (ReadFile(FileHandle, buffer, sizeof(buffer), &dwread, nullptr) !=
        TRUE) {
      ec = bela::make_system_error_code();
      return false;
    }
    ctx.Update(reinterpret_cast<const uint8_t *>(buffer),
               static_cast<size_t>(dwread));
    if (dwread < sizeof(buffer)) {
      break;
    }
  }
  unsigned char buf[sha256::sha256_hash_size];
  ctx.Final(buf);
  HashEncode(buf, sha256::sha256_hash_size, hv);
  return true;
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
  decltype(b3sum) *impl;
};
bool HashEqual(std::wstring_view file, std::wstring_view hashvalue,
               bela::error_code &ec) {
  auto pos = hashvalue.find(':');
  if (pos == std::wstring_view::npos) {
    std::wstring nv;
    if (!b3sum(file, nv, ec)) {
      return false;
    }
    if (bela::EqualsIgnoreCase(nv, hashvalue)) {
      return true;
    }
    ec = bela::make_error_code(1, L"checksum mismatch expected ", hashvalue,
                               L" actual ", nv);
    return false;
  }
  auto hash = hashvalue.substr(pos + 1);
  auto hashname = hashvalue.substr(0, pos);
  constexpr HashPrefix hv[] = {{L"BLAKE3", b3sum}, {L"SHA256", sha256sum}};
  for (const auto &h : hv) {
    if (!bela::EqualsIgnoreCase(hashname, h.prefix)) {
      continue;
    }
    std::wstring hv;
    if (!h.impl(file, hv, ec)) {
      return false;
    }
    if (bela::EqualsIgnoreCase(hv, hash)) {
      return true;
    }
    ec = bela::make_error_code(1, L"checksum mismatch expected ", hash,
                               L" actual ", hv);
    return false;
  }
  ec = bela::make_error_code(1, L"unsupported hash method '", hashname, L"'");
  return false;
}

std::optional<std::wstring> FileHash(std::wstring_view file, HashMethod method,
                                     bela::error_code &ec) {
  std::wstring hv;
  switch (method) {
  case HashMethod::SHA256:
    if (sha256sum(file, hv, ec)) {
      return std::make_optional(std::move(hv));
    }
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