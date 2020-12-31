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
// Copyright 2018 The Abseil Authors.
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
#ifndef BELA_MACROS_HPP
#define BELA_MACROS_HPP
#include <cassert>

#ifdef __has_builtin
#define BELA_HAVE_BUILTIN(x) __has_builtin(x)
#else
#define BELA_HAVE_BUILTIN(x) 0
#endif

#if BELA_HAVE_BUILTIN(__builtin_expect) || (defined(__GNUC__) && !defined(__clang__))
#define BELA_PREDICT_FALSE(x) (__builtin_expect(x, 0))
#define BELA_PREDICT_TRUE(x) (__builtin_expect(!!(x), 1))
#else
#define BELA_PREDICT_FALSE(x) (x)
#define BELA_PREDICT_TRUE(x) (x)
#endif

#if defined(NDEBUG)
#define BELA_ASSERT(expr) (false ? static_cast<void>(expr) : static_cast<void>(0))
#else
#define BELA_ASSERT(expr)                                                                                              \
  (BELA_PREDICT_TRUE((expr)) ? static_cast<void>(0) : [] { assert(false && #expr); }()) // NOLINT
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define BELA_FORCE_INLINE __forceinline
#define BELA_ATTRIBUTE_ALWAYS_INLINE
#else
#define BELA_ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))
#define BELA_FORCE_INLINE inline BELA_ATTRIBUTE_ALWAYS_INLINE
#endif

#endif
