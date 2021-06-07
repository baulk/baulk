// ---------------------------------------------------------------------------
// Copyright (C) 2021, Bela contributors
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

int memcasecmp(const char *s1, const char *s2, size_t len) noexcept {
  for (size_t i = 0; i < len; i++) {
    const auto diff = std::tolower(s1[i]) - std::tolower(s2[i]);
    if (diff != 0) {
      return static_cast<int>(diff);
    }
  }
  return 0;
}

} // namespace strings_internal
} // namespace bela
