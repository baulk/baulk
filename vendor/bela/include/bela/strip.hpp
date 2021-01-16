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
#ifndef BELA_STRIP_HPP
#define BELA_STRIP_HPP
#pragma once
#include "match.hpp"

namespace bela {
inline bool ConsumePrefix(std::wstring_view *str, std::wstring_view expected) {
  if (!bela::StartsWith(*str, expected)) {
    return false;
  }
  str->remove_prefix(expected.size());
  return true;
}
inline bool ConsumeSuffix(std::wstring_view *str, std::wstring_view expected) {
  if (!bela::EndsWith(*str, expected)) {
    return false;
  }
  str->remove_suffix(expected.size());
  return true;
}

inline bool ConsumePrefix(std::string_view *str, std::string_view expected) {
  if (!bela::StartsWith(*str, expected)) {
    return false;
  }
  str->remove_prefix(expected.size());
  return true;
}
inline bool ConsumeSuffix(std::string_view *str, std::string_view expected) {
  if (!bela::EndsWith(*str, expected)) {
    return false;
  }
  str->remove_suffix(expected.size());
  return true;
}

[[nodiscard]] inline std::wstring_view StripPrefix(std::wstring_view str, std::wstring_view prefix) {
  if (bela::StartsWith(str, prefix)) {
    str.remove_prefix(prefix.size());
  }
  return str;
}

inline std::wstring_view StripSuffix(std::wstring_view str, std::wstring_view suffix) {
  if (bela::EndsWith(str, suffix)) {
    str.remove_suffix(suffix.size());
  }
  return str;
}
[[nodiscard]] inline std::string_view StripPrefix(std::string_view str, std::string_view prefix) {
  if (bela::StartsWith(str, prefix)) {
    str.remove_prefix(prefix.size());
  }
  return str;
}

inline std::string_view StripSuffix(std::string_view str, std::string_view suffix) {
  if (bela::EndsWith(str, suffix)) {
    str.remove_suffix(suffix.size());
  }
  return str;
}

} // namespace bela

#endif
