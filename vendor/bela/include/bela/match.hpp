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
#ifndef BELA_MATCH_HPP
#define BELA_MATCH_HPP
#pragma once
#include <cstring>
#include <string_view>

namespace bela {

// StartsWith and EndsWith
inline bool StartsWith(std::wstring_view text, std::wstring_view prefix) noexcept { return text.starts_with(prefix); }
inline bool EndsWith(std::wstring_view text, std::wstring_view suffix) noexcept { return text.ends_with(suffix); }
inline bool StartsWith(std::string_view text, std::string_view prefix) noexcept { return text.starts_with(prefix); }
inline bool EndsWith(std::string_view text, std::string_view suffix) noexcept { return text.ends_with(suffix); }

// StrContains()
//
// Returns whether a given string `haystack` contains the substring `needle`.
inline bool StrContains(std::wstring_view haystack, std::wstring_view needle) noexcept {
  return haystack.find(needle, 0) != haystack.npos;
}

inline bool StrContains(std::wstring_view haystack, wchar_t needle) noexcept {
  return haystack.find(needle) != haystack.npos;
}

bool EqualsIgnoreCase(std::wstring_view piece1, std::wstring_view piece2) noexcept;
bool StartsWithIgnoreCase(std::wstring_view text, std::wstring_view prefix) noexcept;
bool EndsWithIgnoreCase(std::wstring_view text, std::wstring_view suffix) noexcept;

/// Narrow

// StrContains()
//
// Returns whether a given string `haystack` contains the substring `needle`.
inline bool StrContains(std::string_view haystack, std::string_view needle) noexcept {
  return haystack.find(needle, 0) != haystack.npos;
}

inline bool StrContains(std::string_view haystack, char needle) noexcept {
  return haystack.find(needle) != haystack.npos;
}

bool EqualsIgnoreCase(std::string_view piece1, std::string_view piece2) noexcept;
bool StartsWithIgnoreCase(std::string_view text, std::string_view prefix) noexcept;
bool EndsWithIgnoreCase(std::string_view text, std::string_view suffix) noexcept;

} // namespace bela

#endif
