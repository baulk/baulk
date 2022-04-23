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
#ifndef BELA_STR_REPLACE_HPP
#define BELA_STR_REPLACE_HPP
#pragma once
#include <string>
#include <utility>
#include <vector>
#include <string_view>
#include "types.hpp"

namespace bela {
// Implementation details only, past this point.
namespace strings_internal {

template <typename C>
requires bela::character<C>
struct ViableSubstitution {
  using string_view_t = std::basic_string_view<C, std::char_traits<C>>;
  string_view_t old;
  string_view_t replacement;
  size_t offset;

  ViableSubstitution(string_view_t old_str, string_view_t replacement_str, size_t offset_val)
      : old(old_str), replacement(replacement_str), offset(offset_val) {}

  // One substitution occurs "before" another (takes priority) if either
  // it has the lowest offset, or it has the same offset but a larger size.
  bool OccursBefore(const ViableSubstitution &y) const {
    if (offset != y.offset)
      return offset < y.offset;
    return old.size() > y.old.size();
  }
};

// Build a vector of ViableSubstitutions based on the given list of
// replacements. subs can be implemented as a priority_queue. However, it turns
// out that most callers have small enough a list of substitutions that the
// overhead of such a queue isn't worth it.
template <typename StrToStrMapping, typename C>
requires bela::character<C> std::vector<ViableSubstitution<C>>
FindSubstitutions(std::basic_string_view<C, std::char_traits<C>> s, const StrToStrMapping &replacements) {
  std::vector<ViableSubstitution<C>> subs;
  subs.reserve(replacements.size());

  for (const auto &rep : replacements) {
    using std::get;
    std::basic_string_view<C, std::char_traits<C>> old(get<0>(rep));

    size_t pos = s.find(old);
    if (pos == s.npos)
      continue;

    // Ignore attempts to replace "". This condition is almost never true,
    // but above condition is frequently true. That's why we test for this
    // now and not before.
    if (old.empty())
      continue;

    subs.emplace_back(old, get<1>(rep), pos);

    // Insertion sort to ensure the last ViableSubstitution comes before
    // all the others.
    size_t index = subs.size();
    while (--index && subs[index - 1].OccursBefore(subs[index])) {
      std::swap(subs[index], subs[index - 1]);
    }
  }
  return subs;
}

int ApplySubstitutions(std::wstring_view s, std::vector<ViableSubstitution<wchar_t>> *subs_ptr,
                       std::wstring *result_ptr);
int ApplySubstitutions(std::string_view s, std::vector<ViableSubstitution<char>> *subs_ptr, std::string *result_ptr);

} // namespace strings_internal

template <typename StrToStrMapping, typename C>
requires bela::character<C> std::basic_string<C, std::char_traits<C>, std::allocator<C>>
StrReplaceAll(std::basic_string_view<C, std::char_traits<C>> s, const StrToStrMapping &replacements) {
  auto subs = strings_internal::FindSubstitutions<StrToStrMapping, C>(s, replacements);
  std::basic_string<C, std::char_traits<C>, std::allocator<C>> result;
  result.reserve(s.size());
  strings_internal::ApplySubstitutions(s, &subs, &result);
  return result;
}

template <typename StrToStrMapping, typename C>
int StrReplaceAll(const StrToStrMapping &replacements,
                  std::basic_string<C, std::char_traits<C>, std::allocator<C>> *target) {
  auto subs = strings_internal::FindSubstitutions<StrToStrMapping, C>(*target, replacements);
  if (subs.empty())
    return 0;

  std::basic_string<C, std::char_traits<C>, std::allocator<C>> result;
  result.reserve(target->size());
  int substitutions = strings_internal::ApplySubstitutions(*target, &subs, &result);
  target->swap(result);
  return substitutions;
}

[[nodiscard]] std::wstring
StrReplaceAll(std::wstring_view s, std::initializer_list<std::pair<std::wstring_view, std::wstring_view>> replacements);
int StrReplaceAll(std::initializer_list<std::pair<std::wstring_view, std::wstring_view>> replacements,
                  std::wstring *target);

[[nodiscard]] std::string
StrReplaceAll(std::string_view s, std::initializer_list<std::pair<std::string_view, std::string_view>> replacements);
int StrReplaceAll(std::initializer_list<std::pair<std::string_view, std::string_view>> replacements,
                  std::string *target);

} // namespace bela

#endif
