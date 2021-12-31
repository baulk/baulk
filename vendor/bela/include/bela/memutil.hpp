// ---------------------------------------------------------------------------
// Copyright (C) 2022, Bela contributors
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
// Like memcmp, but ignore differences in case.
int memcasecmp(const wchar_t *s1, const wchar_t *s2, size_t len) noexcept;
// Like memcmp, but ignore differences in case.
int memcasecmp(const char *s1, const char *s2, size_t len) noexcept;
} // namespace strings_internal

} // namespace bela

#endif
