// ---------------------------------------------------------------------------
// Copyright (c) 2020, Force Charlie
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Includes work from abseil-cpp (https://github.com/abseil/abseil-cpp)
// with modifications.
//
// Copyright 2019 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ---------------------------------------------------------------------------
#ifndef BELA_MEMUTIL_HPP
#define BELA_MEMUTIL_HPP
#pragma once
#include <cstring>
#include <cstdlib>
#include <cwctype>
#include <cwchar>
#include "ascii.hpp"

namespace bela {
namespace strings_internal {

template <typename T> inline void memcopy(T *dest, const T *src, size_t n) noexcept {
  if (n != 0) {
    memcpy(dest, src, sizeof(T) * n);
  }
}

int memcasecmp(const wchar_t *s1, const wchar_t *s2, size_t len) noexcept;
int memcasecmp_narrow(const char *s1, const char *s2, size_t len) noexcept;
[[nodiscard]] wchar_t *memdup(const wchar_t *s, size_t slen) noexcept;
wchar_t *memrchr(const wchar_t *s, int c, size_t slen) noexcept;
size_t memspn(const wchar_t *s, size_t slen, const wchar_t *accept) noexcept;
size_t memcspn(const wchar_t *s, size_t slen, const wchar_t *reject) noexcept;
wchar_t *mempbrk(const wchar_t *s, size_t slen, const wchar_t *accept) noexcept;

// This is for internal use only.  Don't call this directly
template <bool case_sensitive>
const wchar_t *int_memmatch(const wchar_t *haystack, size_t haylen, const wchar_t *needle, size_t neelen) noexcept {
  if (0 == neelen) {
    return haystack; // even if haylen is 0
  }
  const wchar_t *hayend = haystack + haylen;
  const wchar_t *needlestart = needle;
  const wchar_t *needleend = needlestart + neelen;

  for (; haystack < hayend; ++haystack) {
    wchar_t hay = case_sensitive ? *haystack : bela::ascii_tolower(*haystack);

    wchar_t nee = case_sensitive ? *needle : bela::ascii_tolower(*needle);
    if (hay == nee) {
      if (++needle == needleend) {
        return haystack + 1 - neelen;
      }
    } else if (needle != needlestart) {
      // must back up haystack in case a prefix matched (find "aab" in "aaab")
      haystack -= needle - needlestart; // for loop will advance one more
      needle = needlestart;
    }
  }
  return nullptr;
}

// These are the guys you can call directly
inline const wchar_t *memstr(const wchar_t *phaystack, size_t haylen, const wchar_t *pneedle) noexcept {
  return int_memmatch<true>(phaystack, haylen, pneedle, wcslen(pneedle));
}

inline const wchar_t *memcasestr(const wchar_t *phaystack, size_t haylen, const wchar_t *pneedle) noexcept {
  return int_memmatch<false>(phaystack, haylen, pneedle, wcslen(pneedle));
}

inline const wchar_t *memmem(const wchar_t *phaystack, size_t haylen, const wchar_t *pneedle,
                             size_t needlelen) noexcept {
  return int_memmatch<true>(phaystack, haylen, pneedle, needlelen);
}

inline const wchar_t *memcasemem(const wchar_t *phaystack, size_t haylen, const wchar_t *pneedle,
                                 size_t needlelen) noexcept {
  return int_memmatch<false>(phaystack, haylen, pneedle, needlelen);
}

// This is significantly faster for case-sensitive matches with very
// few possible matches.  See unit test for benchmarks.
const wchar_t *memmatch(const wchar_t *phaystack, size_t haylen, const wchar_t *pneedle, size_t neelen) noexcept;
} // namespace strings_internal

} // namespace bela

#endif
