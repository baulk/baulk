//////
#ifndef BELA_ALGORITHM_HPP
#define BELA_ALGORITHM_HPP
#pragma once
#include <algorithm>

namespace bela {
/*
 * Compute the length of an array with constant length.  (Use of this method
 * with a non-array pointer will not compile.)
 *
 * Beware of the implicit trailing '\0' when using this with string constants.
 */
template <typename T, size_t N> constexpr size_t ArrayLength(T (&aArr)[N]) {
  (void)aArr;
  return N;
}

template <typename T, size_t N> constexpr T *ArrayEnd(T (&aArr)[N]) { return aArr + ArrayLength(aArr); }

/**
 * std::equal has subpar ergonomics.
 */

template <typename T, typename U, size_t N> bool ArrayEqual(const T (&a)[N], const U (&b)[N]) {
  return std::equal(a, a + N, b);
}

template <typename T, typename U> bool ArrayEqual(const T *const a, const U *const b, const size_t n) {
  return std::equal(a, a + n, b);
}

} // namespace bela

#endif
