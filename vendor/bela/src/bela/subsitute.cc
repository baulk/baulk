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
#include <bela/subsitute.hpp>

namespace bela::substitute_internal {
void SubstituteAndAppendArray(std::wstring *output, std::wstring_view format, const std::wstring_view *args_array,
                              size_t num_args) {
  size_t size = 0;
  auto fmtsize = format.size();
  for (size_t i = 0; i < fmtsize; i++) {
    if (format[i] != '$') {
      size++;
      continue;
    }
    if (i + 1 > fmtsize) {
      return;
    }
    if (ascii_isdigit(format[i + 1])) {
      int index = format[i + 1] - L'0';
      if (static_cast<size_t>(index) >= num_args) {
        return;
      }
      size += args_array[index].size();
      ++i;
      continue;
    }
    if (format[i + 1] != '$') {
      return;
    }
    ++size;
    ++i;
  }
  if (size == 0) {
    return;
  }

  // Build the std::string.
  size_t original_size = output->size();
  output->resize(original_size + size);
  wchar_t *target = &(*output)[original_size];
  for (size_t i = 0; i < fmtsize; i++) {
    if (format[i] != '$') {
      *target++ = format[i];
      continue;
    }
    if (ascii_isdigit(format[i + 1])) {
      const std::wstring_view src = args_array[format[i + 1] - '0'];
      target = std::copy(src.begin(), src.end(), target);
      ++i; // Skip next char.
      continue;
    }
    if (format[i + 1] == '$') {
      *target++ = '$';
      ++i; // Skip next char.
    }
  }
}
} // namespace bela
