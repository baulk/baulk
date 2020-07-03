///
#ifndef BELA_HASH_INTERNAL_HPP
#define BELA_HASH_INTERNAL_HPP
#include <bela/bits.hpp>
#include <bela/endian.hpp>

/**
 * Copy a memory block with simultaneous exchanging byte order.
 * The byte order is changed from little-endian 32-bit integers
 * to big-endian (or vice-versa).
 *
 * @param to the pointer where to copy memory block
 * @param index the index to start writing from
 * @param from  the source block to copy
 * @param length length of the memory block
 */
inline void swap_copy_str_to_u32(void *to, int index, const void *from, size_t length) {
  /* if all pointers and length are 32-bits aligned */
  if (0 == (((int)((char *)to - (char *)0) | ((char *)from - (char *)0) | index | length) & 3)) {
    /* copy memory as 32-bit words */
    const uint32_t *src = (const uint32_t *)from;
    const uint32_t *end = (const uint32_t *)((const char *)src + length);
    uint32_t *dst = (uint32_t *)((char *)to + index);
    for (; src < end; dst++, src++) {
      *dst = bela::swap32(*src);
    }
  } else {
    const char *src = (const char *)from;
    for (length += index; (size_t)index < length; index++) {
      ((char *)to)[index ^ 3] = *(src++);
    }
  }
}

inline void swap_copy_str_to_u64(void *to, int index, const void *from, size_t length) {
  /* if all pointers and length are 64-bits aligned */
  if (0 == (((int)((char *)to - (char *)0) | ((char *)from - (char *)0) | index | length) & 7)) {
    /* copy aligned memory block as 64-bit integers */
    const uint64_t *src = (const uint64_t *)from;
    const uint64_t *end = (const uint64_t *)((const char *)src + length);
    uint64_t *dst = (uint64_t *)((char *)to + index);
    while (src < end) {
      *(dst++) = bela::swap64(*(src++));
    }
  } else {
    const char *src = (const char *)from;
    for (length += index; (size_t)index < length; index++) {
      ((char *)to)[index ^ 7] = *(src++);
    }
  }
}

inline void swap_copy_u64_to_str(void *to, const void *from, size_t length) {
  /* if all pointers and length are 64-bits aligned */
  if (0 == (((int)((char *)to - (char *)0) | ((char *)from - (char *)0) | length) & 7)) {
    /* copy aligned memory block as 64-bit integers */
    const uint64_t *src = (const uint64_t *)from;
    const uint64_t *end = (const uint64_t *)((const char *)src + length);
    uint64_t *dst = (uint64_t *)to;
    while (src < end) {
      *(dst++) = bela::swap64(*(src++));
    }
  } else {
    size_t index;
    char *dst = (char *)to;
    for (index = 0; index < length; index++) {
      *(dst++) = ((char *)from)[index ^ 7];
    }
  }
}

#if IS_BIG_ENDIAN
#define be2me_32(x) (x)
#define be2me_64(x) (x)
#define le2me_32(x) bela::swap32(x)
#define le2me_64(x) bela::swap64(x)

#define be32_copy(to, index, from, length) memcpy((to) + (index), (from), (length))
#define le32_copy(to, index, from, length) swap_copy_str_to_u32((to), (index), (from), (length))
#define be64_copy(to, index, from, length) memcpy((to) + (index), (from), (length))
#define le64_copy(to, index, from, length) swap_copy_str_to_u64((to), (index), (from), (length))
#define me64_to_be_str(to, from, length) memcpy((to), (from), (length))
#define me64_to_le_str(to, from, length) swap_copy_u64_to_str((to), (from), (length))

#else /* IS_BIG_ENDIAN */
#define be2me_32(x) bela::swap32(x)
#define be2me_64(x) bela::swap64(x)
#define le2me_32(x) (x)
#define le2me_64(x) (x)

#define be32_copy(to, index, from, length) swap_copy_str_to_u32((to), (index), (from), (length))
#define le32_copy(to, index, from, length) memcpy((to) + (index), (from), (length))
#define be64_copy(to, index, from, length) swap_copy_str_to_u64((to), (index), (from), (length))
#define le64_copy(to, index, from, length) memcpy((to) + (index), (from), (length))
#define me64_to_be_str(to, from, length) swap_copy_u64_to_str((to), (from), (length))
#define me64_to_le_str(to, from, length) memcpy((to), (from), (length))

#endif

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define I64(x) x##ui64
#else
#define I64(x) x##ULL
#endif

/* ROTL/ROTR macros rotate a 32/64-bit word left/right by n bits */
#define ROTL32(dword, n) ((dword) << (n) ^ ((dword) >> (32 - (n))))
#define ROTR32(dword, n) ((dword) >> (n) ^ ((dword) << (32 - (n))))
#define ROTL64(qword, n) ((qword) << (n) ^ ((qword) >> (64 - (n))))
#define ROTR64(qword, n) ((qword) >> (n) ^ ((qword) << (64 - (n))))

#define IS_ALIGNED_32(p) (0 == (3 & ((const char *)(p) - (const char *)0)))
#define IS_ALIGNED_64(p) (0 == (7 & ((const char *)(p) - (const char *)0)))

#endif