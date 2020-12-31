// ---------------------------------------------------------------------------
// Copyright (C) 2021, Force Charlie
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
#include <cwctype>
// https://en.cppreference.com/w/cpp/header/cwctype
#include <bela/memutil.hpp>

namespace bela {

namespace strings_internal {

int memcasecmp(const wchar_t *s1, const wchar_t *s2, size_t len) noexcept {
  for (size_t i = 0; i < len; i++) {
    const auto diff = bela::ascii_tolower(s1[i]) - bela::ascii_tolower(s2[i]);
    if (diff != 0) {
      return static_cast<int>(diff);
    }
  }
  return 0;
}

int memcasecmp_narrow(const char *s1, const char *s2, size_t len) noexcept {
  for (size_t i = 0; i < len; i++) {
    const auto diff = std::tolower(s1[i]) - std::tolower(s2[i]);
    if (diff != 0) {
      return static_cast<int>(diff);
    }
  }
  return 0;
}

[[nodiscard]] wchar_t *memdup(const wchar_t *s, size_t slen) noexcept {
  void *copy;
  if ((copy = malloc(slen * sizeof(wchar_t))) == nullptr) {
    return nullptr;
  }
  memcpy(copy, s, slen * sizeof(wchar_t));
  return reinterpret_cast<wchar_t *>(copy);
}

wchar_t *memrchr(const wchar_t *s, int c, size_t slen) noexcept {
  for (const wchar_t *e = s + slen - 1; e >= s; e--) {
    if (*e == c) {
      return const_cast<wchar_t *>(e);
    }
  }
  return nullptr;
}

size_t memspn(const wchar_t *s, size_t slen, const wchar_t *accept) noexcept {
  const wchar_t *p = s;
  const wchar_t *spanp;
  wchar_t c, sc;

cont:
  c = *p++;
  if (slen-- == 0) {
    return p - 1 - s;
  }
  for (spanp = accept; (sc = *spanp++) != '\0';) {
    if (sc == c) {
      goto cont;
    }
  }
  return p - 1 - s;
}

size_t memcspn(const wchar_t *s, size_t slen, const wchar_t *reject) noexcept {
  const wchar_t *p = s;
  const wchar_t *spanp;
  wchar_t c, sc;

  while (slen-- != 0) {
    c = *p++;
    for (spanp = reject; (sc = *spanp++) != '\0';) {
      if (sc == c) {
        return p - 1 - s;
      }
    }
  }
  return p - s;
}

wchar_t *mempbrk(const wchar_t *s, size_t slen, const wchar_t *accept) noexcept {
  const wchar_t *scanp;
  int sc;

  for (; slen; ++s, --slen) {
    for (scanp = accept; (sc = *scanp++) != '\0';) {
      if (sc == *s) {
        return const_cast<wchar_t *>(s);
      }
    }
  }
  return nullptr;
}

// This is significantly faster for case-sensitive matches with very
// few possible matches.  See unit test for benchmarks.
const wchar_t *memmatch(const wchar_t *phaystack, size_t haylen, const wchar_t *pneedle, size_t neelen) noexcept {
  if (0 == neelen) {
    return phaystack; // even if haylen is 0
  }
  if (haylen < neelen) {
    return nullptr;
  }

  const wchar_t *match;
  const wchar_t *hayend = phaystack + haylen - neelen + 1;
  // A static cast is used here to work around the fact that memchr returns
  // a void* on Posix-compliant systems and const void* on Windows.
  while ((match = static_cast<const wchar_t *>(wmemchr(phaystack, pneedle[0], (hayend - phaystack)))) != nullptr) {
    if (memcmp(match, pneedle, neelen * sizeof(wchar_t)) == 0) {
      return match;
    }
    phaystack = match + 1;
  }
  return nullptr;
}

} // namespace strings_internal
} // namespace bela
