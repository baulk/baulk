/*
 * Port from rhash. origin license:
 * sha3.c - an implementation of Secure Hash Algorithm 3 (Keccak).
 * based on the
 * The Keccak SHA-3 submission. Submission to NIST (Round 3), 2011
 * by Guido Bertoni, Joan Daemen, Michaël Peeters and Gilles Van Assche
 *
 * Copyright: 2013 Aleksey Kravchenko <rhash.admin@gmail.com>
 *
 * Permission is hereby granted,  free of charge,  to any person  obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,  including without limitation
 * the rights to  use, copy, modify,  merge, publish, distribute, sublicense,
 * and/or sell copies  of  the Software,  and to permit  persons  to whom the
 * Software is furnished to do so.
 *
 * This program  is  distributed  in  the  hope  that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  Use this program  at  your own risk!
 */
#include <cassert>
#include <bela/hash.hpp>
#include "hashinternal.hpp"

namespace bela::hash::sha3 {
/* constants */
#define NumberOfRounds 24

/* SHA3 (Keccak) constants for 24 rounds */
static constexpr uint64_t keccak_round_constants[NumberOfRounds] = {
    I64(0x0000000000000001),
    I64(0x0000000000008082),
    I64(0x800000000000808A),
    I64(0x8000000080008000),
    I64(0x000000000000808B),
    I64(0x0000000080000001),
    I64(0x8000000080008081),
    I64(0x8000000000008009),
    I64(0x000000000000008A),
    I64(0x0000000000000088),
    I64(0x0000000080008009),
    I64(0x000000008000000A),
    I64(0x000000008000808B),
    I64(0x800000000000008B),
    I64(0x8000000000008089),
    I64(0x8000000000008003),
    I64(0x8000000000008002),
    I64(0x8000000000000080),
    I64(0x000000000000800A),
    I64(0x800000008000000A),
    I64(0x8000000080008081),
    I64(0x8000000000008080),
    I64(0x0000000080000001),
    I64(0x8000000080008008)
    //
};
void Hasher::Initialize(HashBits hb_) {
  hb = hb_;
  /* NB: The Keccak capacity parameter = bits * 2 */
  uint32_t rate = 1600 - (static_cast<int>(hb) * 2);
  memset(message, 0, sizeof(message));
  memset(hash, 0, sizeof(hash));
  rest = 0;
  block_size = rate / 8;
}

#define XORED_A(i) A[(i)] ^ A[(i) + 5] ^ A[(i) + 10] ^ A[(i) + 15] ^ A[(i) + 20]
#define THETA_STEP(i)                                                                                                  \
  A[(i)] ^= D[(i)];                                                                                                    \
  A[(i) + 5] ^= D[(i)];                                                                                                \
  A[(i) + 10] ^= D[(i)];                                                                                               \
  A[(i) + 15] ^= D[(i)];                                                                                               \
  A[(i) + 20] ^= D[(i)]

/* Keccak theta() transformation */
static void keccak_theta(uint64_t *A) {
  uint64_t D[5];
  D[0] = ROTL64(XORED_A(1), 1) ^ XORED_A(4);
  D[1] = ROTL64(XORED_A(2), 1) ^ XORED_A(0);
  D[2] = ROTL64(XORED_A(3), 1) ^ XORED_A(1);
  D[3] = ROTL64(XORED_A(4), 1) ^ XORED_A(2);
  D[4] = ROTL64(XORED_A(0), 1) ^ XORED_A(3);
  THETA_STEP(0);
  THETA_STEP(1);
  THETA_STEP(2);
  THETA_STEP(3);
  THETA_STEP(4);
}

/* Keccak pi() transformation */
static void keccak_pi(uint64_t *A) {
  uint64_t A1;
  A1 = A[1];
  A[1] = A[6];
  A[6] = A[9];
  A[9] = A[22];
  A[22] = A[14];
  A[14] = A[20];
  A[20] = A[2];
  A[2] = A[12];
  A[12] = A[13];
  A[13] = A[19];
  A[19] = A[23];
  A[23] = A[15];
  A[15] = A[4];
  A[4] = A[24];
  A[24] = A[21];
  A[21] = A[8];
  A[8] = A[16];
  A[16] = A[5];
  A[5] = A[3];
  A[3] = A[18];
  A[18] = A[17];
  A[17] = A[11];
  A[11] = A[7];
  A[7] = A[10];
  A[10] = A1;
  /* note: A[ 0] is left as is */
}

#define CHI_STEP(i)                                                                                                    \
  A0 = A[0 + (i)];                                                                                                     \
  A1 = A[1 + (i)];                                                                                                     \
  A[0 + (i)] ^= ~A1 & A[2 + (i)];                                                                                      \
  A[1 + (i)] ^= ~A[2 + (i)] & A[3 + (i)];                                                                              \
  A[2 + (i)] ^= ~A[3 + (i)] & A[4 + (i)];                                                                              \
  A[3 + (i)] ^= ~A[4 + (i)] & A0;                                                                                      \
  A[4 + (i)] ^= ~A0 & A1

/* Keccak chi() transformation */
static void keccak_chi(uint64_t *A) {
  uint64_t A0;
  uint64_t A1;
  CHI_STEP(0);
  CHI_STEP(5);
  CHI_STEP(10);
  CHI_STEP(15);
  CHI_STEP(20);
}

static void sha3_permutation(uint64_t *state) {
  for (unsigned long long keccak_round_constant : keccak_round_constants) {
    keccak_theta(state);

    /* apply Keccak rho() transformation */
    state[1] = ROTL64(state[1], 1);
    state[2] = ROTL64(state[2], 62);
    state[3] = ROTL64(state[3], 28);
    state[4] = ROTL64(state[4], 27);
    state[5] = ROTL64(state[5], 36);
    state[6] = ROTL64(state[6], 44);
    state[7] = ROTL64(state[7], 6);
    state[8] = ROTL64(state[8], 55);
    state[9] = ROTL64(state[9], 20);
    state[10] = ROTL64(state[10], 3);
    state[11] = ROTL64(state[11], 10);
    state[12] = ROTL64(state[12], 43);
    state[13] = ROTL64(state[13], 25);
    state[14] = ROTL64(state[14], 39);
    state[15] = ROTL64(state[15], 41);
    state[16] = ROTL64(state[16], 45);
    state[17] = ROTL64(state[17], 15);
    state[18] = ROTL64(state[18], 21);
    state[19] = ROTL64(state[19], 8);
    state[20] = ROTL64(state[20], 18);
    state[21] = ROTL64(state[21], 2);
    state[22] = ROTL64(state[22], 61);
    state[23] = ROTL64(state[23], 56);
    state[24] = ROTL64(state[24], 14);

    keccak_pi(state);
    keccak_chi(state);

    /* apply iota(state, round) */
    *state ^= keccak_round_constant;
  }
}

/**
 * The core transformation. Process the specified block of data.
 *
 * @param hash the algorithm state
 * @param block the message block to process
 * @param block_size the size of the processed block in bytes
 */
static void sha3_process_block(uint64_t hash[25], const uint64_t *block, size_t block_size) {
  /* expanded loop */
  hash[0] ^= le2me_64(block[0]);
  hash[1] ^= le2me_64(block[1]);
  hash[2] ^= le2me_64(block[2]);
  hash[3] ^= le2me_64(block[3]);
  hash[4] ^= le2me_64(block[4]);
  hash[5] ^= le2me_64(block[5]);
  hash[6] ^= le2me_64(block[6]);
  hash[7] ^= le2me_64(block[7]);
  hash[8] ^= le2me_64(block[8]);
  /* if not sha3-512 */
  if (block_size > 72) {
    hash[9] ^= le2me_64(block[9]);
    hash[10] ^= le2me_64(block[10]);
    hash[11] ^= le2me_64(block[11]);
    hash[12] ^= le2me_64(block[12]);
    /* if not sha3-384 */
    if (block_size > 104) {
      hash[13] ^= le2me_64(block[13]);
      hash[14] ^= le2me_64(block[14]);
      hash[15] ^= le2me_64(block[15]);
      hash[16] ^= le2me_64(block[16]);
      /* if not sha3-256 */
      if (block_size > 136) {
        hash[17] ^= le2me_64(block[17]);
#ifdef FULL_SHA3_FAMILY_SUPPORT
        /* if not sha3-224 */
        if (block_size > 144) {
          hash[18] ^= le2me_64(block[18]);
          hash[19] ^= le2me_64(block[19]);
          hash[20] ^= le2me_64(block[20]);
          hash[21] ^= le2me_64(block[21]);
          hash[22] ^= le2me_64(block[22]);
          hash[23] ^= le2me_64(block[23]);
          hash[24] ^= le2me_64(block[24]);
        }
#endif
      }
    }
  }
  /* make a permutation of the hash */
  sha3_permutation(hash);
}

#define SHA3_FINALIZED 0x80000000

void Hasher::Update(const void *input, size_t input_len) {
  auto msg = reinterpret_cast<const uint8_t *>(input);
  auto index = (size_t)rest;
  if ((rest & SHA3_FINALIZED) != 0) {
    return; /* too late for additional input */
  }
  rest = (uint32_t)((rest + input_len) % block_size);

  /* fill partial block */
  if (index != 0U) {
    size_t left = block_size - index;
    memcpy((char *)message + index, msg, (input_len < left ? input_len : left));
    if (input_len < left) {
      return;
    }

    /* process partial block */
    sha3_process_block(hash, message, block_size);
    msg += left;
    input_len -= left;
  }
  while (input_len >= block_size) {
    uint64_t *aligned_message_block;
    if (IS_ALIGNED_64(msg)) {
      /* the most common case is processing of an already aligned message
      without copying it */
      aligned_message_block = (uint64_t *)msg;
    } else {
      memcpy(message, msg, block_size);
      aligned_message_block = message;
    }

    sha3_process_block(hash, aligned_message_block, block_size);
    msg += block_size;
    input_len -= block_size;
  }
  if (input_len != 0) {
    memcpy(message, msg, input_len); /* save leftovers */
  }
}
void Hasher::Finalize(uint8_t *out, size_t out_len) {
  size_t digest_length = 100 - (block_size / 2);
  if ((rest & SHA3_FINALIZED) == 0U) {
    /* clear the rest of the data queue */
    memset((char *)message + rest, 0, block_size - rest);
    ((char *)message)[rest] |= 0x06;
    ((char *)message)[block_size - 1] |= 0x80;

    /* process final block */
    sha3_process_block(hash, message, block_size);
    rest = SHA3_FINALIZED; /* mark context as finalized */
  }

  assert(block_size > digest_length);
  if (out != nullptr && out_len >= digest_length) {
    me64_to_le_str(out, hash, digest_length);
  }
}
} // namespace bela::hash::sha3