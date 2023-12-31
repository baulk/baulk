// ---------------------------------------------------------------------------
// Copyright © 2024, Bela contributors
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
#include <bela/match.hpp>
#include <bela/ascii.hpp>

namespace bela {
namespace strings_internal {
constexpr int memcasecmp(const wchar_t *s1, const wchar_t *s2, size_t len) noexcept {
  for (size_t i = 0; i < len; i++) {
    const auto diff = bela::ascii_tolower(s1[i]) - bela::ascii_tolower(s2[i]);
    if (diff != 0) {
      return static_cast<int>(diff);
    }
  }
  return 0;
}

constexpr int memcasecmp(const char *s1, const char *s2, size_t len) noexcept {
  for (size_t i = 0; i < len; i++) {
    const auto diff = bela::ascii_tolower(s1[i]) - bela::ascii_tolower(s2[i]);
    if (diff != 0) {
      return static_cast<int>(diff);
    }
  }
  return 0;
}
} // namespace strings_internal

bool EqualsIgnoreCase(std::wstring_view piece1, std::wstring_view piece2) noexcept {
  return (piece1.size() == piece2.size() &&
          strings_internal::memcasecmp(piece1.data(), piece2.data(), piece1.size()) == 0);
}
bool StartsWithIgnoreCase(std::wstring_view text, std::wstring_view prefix) noexcept {
  return (text.size() >= prefix.size()) && EqualsIgnoreCase(text.substr(0, prefix.size()), prefix);
}
bool EndsWithIgnoreCase(std::wstring_view text, std::wstring_view suffix) noexcept {
  return (text.size() >= suffix.size()) && EqualsIgnoreCase(text.substr(text.size() - suffix.size()), suffix);
}

bool EqualsIgnoreCase(std::string_view piece1, std::string_view piece2) noexcept {
  return (piece1.size() == piece2.size() &&
          strings_internal::memcasecmp(piece1.data(), piece2.data(), piece1.size()) == 0);
}
bool StartsWithIgnoreCase(std::string_view text, std::string_view prefix) noexcept {
  return (text.size() >= prefix.size()) && EqualsIgnoreCase(text.substr(0, prefix.size()), prefix);
}
bool EndsWithIgnoreCase(std::string_view text, std::string_view suffix) noexcept {
  return (text.size() >= suffix.size()) && EqualsIgnoreCase(text.substr(text.size() - suffix.size()), suffix);
}

} // namespace bela
