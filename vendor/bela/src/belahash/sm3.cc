/// SM3
// Thanks NEWPLAN(newplan001@163.com)
// https://github.com/NEWPLAN/SMx/blob/master/SM3/Windows/SM3/src/sm3.c
#include <bela/hash.hpp>
#include "hashinternal.hpp"

#if IS_BIG_ENDIAN
#define GET32(n, b, i)                                                                                                 \
  {                                                                                                                    \
    (n) = ((uint32_t)(b)[(i) + 3] << 24) | ((uint32_t)(b)[(i) + 2] << 16) | ((uint32_t)(b)[(i)] << 8) |                \
          ((uint32_t)(b)[(i)]);                                                                                        \
  }
#define PUT32(n, b, i)                                                                                                 \
  {                                                                                                                    \
    (b)[(i) + 3] = (uint8_t)((n) >> 24);                                                                               \
    (b)[(i) + 2] = (uint8_t)((n) >> 16);                                                                               \
    (b)[(i) + 1] = (uint8_t)((n) >> 8);                                                                                \
    (b)[(i)] = (uint8_t)((n));                                                                                         \
  }
#else
#define GET32(n, b, i)                                                                                                 \
  {                                                                                                                    \
    (n) = ((uint32_t)(b)[(i)] << 24) | ((uint32_t)(b)[(i) + 1] << 16) | ((uint32_t)(b)[(i) + 2] << 8) |                \
          ((uint32_t)(b)[(i) + 3]);                                                                                    \
  }
#define PUT32(n, b, i)                                                                                                 \
  {                                                                                                                    \
    (b)[(i)] = (uint8_t)((n) >> 24);                                                                                   \
    (b)[(i) + 1] = (uint8_t)((n) >> 16);                                                                               \
    (b)[(i) + 2] = (uint8_t)((n) >> 8);                                                                                \
    (b)[(i) + 3] = (uint8_t)((n));                                                                                     \
  }
#endif

namespace bela::hash::sm3 {
void Hasher::Initialize() {
  Nl = 0;
  Nh = 0;
  digest[0] = 0x7380166F;
  digest[1] = 0x4914B2B9;
  digest[2] = 0x172442D7;
  digest[3] = 0xDA8A0600;
  digest[4] = 0xA96F30BC;
  digest[5] = 0x163138AA;
  digest[6] = 0xE38DEE4D;
  digest[7] = 0xB0FB0E4E;
}

static void sm3_process_block(Hasher *ctx, const uint8_t data[64]) {
  uint32_t SS1, SS2, TT1, TT2, W[68], W1[64];
  uint32_t A, B, C, D, E, F, G, H;
  uint32_t T[64];
  uint32_t T1, T2, T3, T4, T5;
  int j;

  for (j = 0; j < 16; j++) {
    T[j] = 0x79CC4519;
  }
  for (j = 16; j < 64; j++) {
    T[j] = 0x7A879D8A;
  }

  GET32(W[0], data, 0);
  GET32(W[1], data, 4);
  GET32(W[2], data, 8);
  GET32(W[3], data, 12);
  GET32(W[4], data, 16);
  GET32(W[5], data, 20);
  GET32(W[6], data, 24);
  GET32(W[7], data, 28);
  GET32(W[8], data, 32);
  GET32(W[9], data, 36);
  GET32(W[10], data, 40);
  GET32(W[11], data, 44);
  GET32(W[12], data, 48);
  GET32(W[13], data, 52);
  GET32(W[14], data, 56);
  GET32(W[15], data, 60);

#define FF0(x, y, z) ((x) ^ (y) ^ (z))
#define FF1(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))

#define GG0(x, y, z) ((x) ^ (y) ^ (z))
#define GG1(x, y, z) (((x) & (y)) | ((~(x)) & (z)))

#define SHL(x, n) (((x)&0xFFFFFFFF) << n)
#define ROTL(x, n) (SHL((x), n) | ((x) >> (32 - n)))

#define P0(x) ((x) ^ ROTL((x), 9) ^ ROTL((x), 17))
#define P1(x) ((x) ^ ROTL((x), 15) ^ ROTL((x), 23))

  for (j = 16; j < 68; j++) {
    T1 = W[j - 16] ^ W[j - 9];
    T2 = ROTL(W[j - 3], 15);
    T3 = T1 ^ T2;
    T4 = P1(T3);
    T5 = ROTL(W[j - 13], 7) ^ W[j - 6];
    W[j] = T4 ^ T5;
  }

  for (j = 0; j < 64; j++) {
    W1[j] = W[j] ^ W[j + 4];
  }

  A = ctx->digest[0];
  B = ctx->digest[1];
  C = ctx->digest[2];
  D = ctx->digest[3];
  E = ctx->digest[4];
  F = ctx->digest[5];
  G = ctx->digest[6];
  H = ctx->digest[7];

  for (j = 0; j < 16; j++) {
    SS1 = ROTL((ROTL(A, 12) + E + ROTL(T[j], j)), 7);
    SS2 = SS1 ^ ROTL(A, 12);
    TT1 = FF0(A, B, C) + D + SS2 + W1[j];
    TT2 = GG0(E, F, G) + H + SS1 + W[j];
    D = C;
    C = ROTL(B, 9);
    B = A;
    A = TT1;
    H = G;
    G = ROTL(F, 19);
    F = E;
    E = P0(TT2);
  }

  for (j = 16; j < 64; j++) {
    SS1 = ROTL((ROTL(A, 12) + E + ROTL(T[j], j)), 7);
    SS2 = SS1 ^ ROTL(A, 12);
    TT1 = FF1(A, B, C) + D + SS2 + W1[j];
    TT2 = GG1(E, F, G) + H + SS1 + W[j];
    D = C;
    C = ROTL(B, 9);
    B = A;
    A = TT1;
    H = G;
    G = ROTL(F, 19);
    F = E;
    E = P0(TT2);
  }

  ctx->digest[0] ^= A;
  ctx->digest[1] ^= B;
  ctx->digest[2] ^= C;
  ctx->digest[3] ^= D;
  ctx->digest[4] ^= E;
  ctx->digest[5] ^= F;
  ctx->digest[6] ^= G;
  ctx->digest[7] ^= H;
}

void Hasher::Update(const void *input, size_t input_len) {
  int fill;
  uint32_t left;
  auto data = reinterpret_cast<const uint8_t *>(input);
  if (input_len == 0) {
    return;
  }

  left = Nl & 0x3F;
  fill = 64 - left;

  Nl += static_cast<uint32_t>(input_len);
  Nl &= 0xFFFFFFFF;

  if (Nl < static_cast<uint32_t>(input_len)) {
    Nh++;
  }

  if (left && static_cast<int32_t>(input_len) >= fill) {
    memcpy(block + left, input, fill);
    sm3_process_block(this, block);
    data += fill;
    input_len -= fill;
    left = 0;
  }

  while (input_len >= 64) {
    sm3_process_block(this, data);
    data += 64;
    input_len -= 64;
  }

  if (input_len > 0) {
    memcpy(block + left, data, input_len);
  }
}

static constexpr const uint8_t sm3_padding[64] = {0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void Hasher::Finalize(uint8_t *out, size_t out_len) {
  auto high = (Nl >> 29) | (Nh << 3);
  auto low = (Nl << 3);
  auto last = Nl & 0x3F;
  auto padn = (last < 56) ? (56 - last) : (120 - last);
  Update(sm3_padding, padn);
  uint8_t msglen[8];
  PUT32(high, msglen, 0);
  PUT32(low, msglen, 4);
  Update(msglen, 8);

  if (out_len >= 32) {
    PUT32(digest[0], out, 0);
    PUT32(digest[1], out, 4);
    PUT32(digest[2], out, 8);
    PUT32(digest[3], out, 12);
    PUT32(digest[4], out, 16);
    PUT32(digest[5], out, 20);
    PUT32(digest[6], out, 24);
    PUT32(digest[7], out, 28);
  }
}

} // namespace bela::hash::sm3