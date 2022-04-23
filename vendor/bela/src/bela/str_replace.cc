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
#include <bela/str_replace.hpp>
#include <bela/str_cat.hpp>

namespace bela {
namespace strings_internal {

// Applies the ViableSubstitutions in subs_ptr to the std::wstring_view s, and
// stores the result in *result_ptr. Returns the number of substitutions that
// occurred.
int ApplySubstitutions(std::wstring_view s, std::vector<strings_internal::ViableSubstitution<wchar_t>> *subs_ptr,
                       std::wstring *result_ptr) {
  auto &subs = *subs_ptr;
  int substitutions = 0;
  size_t pos = 0;
  while (!subs.empty()) {
    auto &sub = subs.back();
    if (sub.offset >= pos) {
      if (pos <= s.size()) {
        StrAppend(result_ptr, s.substr(pos, sub.offset - pos), sub.replacement);
      }
      pos = sub.offset + sub.old.size();
      substitutions += 1;
    }
    sub.offset = s.find(sub.old, pos);
    if (sub.offset == std::wstring_view::npos) {
      subs.pop_back();
    } else {
      // Insertion sort to ensure the last ViableSubstitution continues to be
      // before all the others.
      size_t index = subs.size();
      while ((--index != 0U) && subs[index - 1].OccursBefore(subs[index])) {
        std::swap(subs[index], subs[index - 1]);
      }
    }
  }
  result_ptr->append(s.data() + pos, s.size() - pos);
  return substitutions;
}

int ApplySubstitutions(std::string_view s, std::vector<strings_internal::ViableSubstitution<char>> *subs_ptr,
                       std::string *result_ptr) {
  auto &subs = *subs_ptr;
  int substitutions = 0;
  size_t pos = 0;
  while (!subs.empty()) {
    auto &sub = subs.back();
    if (sub.offset >= pos) {
      if (pos <= s.size()) {
        StrAppend(result_ptr, s.substr(pos, sub.offset - pos), sub.replacement);
      }
      pos = sub.offset + sub.old.size();
      substitutions += 1;
    }
    sub.offset = s.find(sub.old, pos);
    if (sub.offset == std::wstring_view::npos) {
      subs.pop_back();
    } else {
      // Insertion sort to ensure the last ViableSubstitution continues to be
      // before all the others.
      size_t index = subs.size();
      while ((--index != 0U) && subs[index - 1].OccursBefore(subs[index])) {
        std::swap(subs[index], subs[index - 1]);
      }
    }
  }
  result_ptr->append(s.data() + pos, s.size() - pos);
  return substitutions;
}

} // namespace strings_internal

// We can implement this in terms of the generic StrReplaceAll, but
// we must specify the template overload because C++ cannot deduce the type
// of an initializer_list parameter to a function, and also if we don't specify
// the type, we just call ourselves.
//
// Note that we implement them here, rather than in the header, so that they
// aren't inlined.

std::wstring StrReplaceAll(std::wstring_view s,
                           std::initializer_list<std::pair<std::wstring_view, std::wstring_view>> replacements) {
  return StrReplaceAll<std::initializer_list<std::pair<std::wstring_view, std::wstring_view>>, wchar_t>(s,
                                                                                                        replacements);
}

int StrReplaceAll(std::initializer_list<std::pair<std::wstring_view, std::wstring_view>> replacements,
                  std::wstring *target) {
  return StrReplaceAll<std::initializer_list<std::pair<std::wstring_view, std::wstring_view>>, wchar_t>(replacements,
                                                                                                        target);
}

std::string StrReplaceAll(std::string_view s,
                          std::initializer_list<std::pair<std::string_view, std::string_view>> replacements) {
  return StrReplaceAll<std::initializer_list<std::pair<std::string_view, std::string_view>>, char>(s, replacements);
}

int StrReplaceAll(std::initializer_list<std::pair<std::string_view, std::string_view>> replacements,
                  std::string *target) {
  return StrReplaceAll<std::initializer_list<std::pair<std::string_view, std::string_view>>, char>(replacements,
                                                                                                   target);
}

} // namespace bela
