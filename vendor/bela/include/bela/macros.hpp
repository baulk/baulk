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

#if defined(__has_attribute)
// BELA_HAVE_ATTRIBUTE
//
// A function-like feature checking macro that is a wrapper around
#define BELA_HAVE_ATTRIBUTE(x) __has_attribute(x)
#else
#define BELA_HAVE_ATTRIBUTE(x) 0
#endif

// BELA_ATTRIBUTE_WEAK
//
// Tags a function as weak for the purposes of compilation and linking.
// Weak attributes currently do not work properly in LLVM's Windows backend,
// so disable them there. See https://bugs.llvm.org/show_bug.cgi?id=37598
// for further information.
// The MinGW compiler doesn't complain about the weak attribute until the link
// step, presumably because Windows doesn't use ELF binaries.
#if (BELA_HAVE_ATTRIBUTE(weak) || (defined(__GNUC__) && !defined(__clang__))) &&                                       \
    !(defined(__llvm__) && defined(_WIN32)) && !defined(__MINGW32__)
#undef BELA_ATTRIBUTE_WEAK
#define BELA_ATTRIBUTE_WEAK __attribute__((weak))
#define BELA_HAVE_ATTRIBUTE_WEAK 1
#else
#define BELA_ATTRIBUTE_WEAK
#define BELA_HAVE_ATTRIBUTE_WEAK 0
#endif

// BELA_HAVE_CPP_ATTRIBUTE
//
// A function-like feature checking macro that accepts C++11 style attributes.
// It's a wrapper around `__has_cpp_attribute`, defined by ISO C++ SD-6
// (https://en.cppreference.com/w/cpp/experimental/feature_test). If we don't
// find `__has_cpp_attribute`, will evaluate to 0.
#if defined(__cplusplus) && defined(__has_cpp_attribute)
// NOTE: requiring __cplusplus above should not be necessary, but
// works around https://bugs.llvm.org/show_bug.cgi?id=23435.
#define BELA_HAVE_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#define BELA_HAVE_CPP_ATTRIBUTE(x) 0
#endif

#if BELA_HAVE_CPP_ATTRIBUTE(clang::require_constant_initialization)
#define BELA_CONST_INIT [[clang::require_constant_initialization]]
#else
#define BELA_CONST_INIT
#endif // BELA_HAVE_CPP_ATTRIBUTE(clang::require_constant_initialization)

#if defined(__clang__)
#define THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x) // no-op
#endif

#define GUARDED_BY(x) THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#if defined(NDEBUG)
#define BELA_ASSERT(expr) (false ? static_cast<void>(expr) : static_cast<void>(0))
#else
#define BELA_ASSERT(expr)                                                                                              \
  (BELA_PREDICT_TRUE((expr)) ? static_cast<void>(0) : [] { assert(false && #expr); }()) // NOLINT
#endif

// BELA_EXCLUSIVE_LOCKS_REQUIRED() / BELA_SHARED_LOCKS_REQUIRED()
//
// Documents a function that expects a mutex to be held prior to entry.
// The mutex is expected to be held both on entry to, and exit from, the
// function.
//
// An exclusive lock allows read-write access to the guarded data member(s), and
// only one thread can acquire a lock exclusively at any one time. A shared lock
// allows read-only access, and any number of threads can acquire a shared lock
// concurrently.
//
// Generally, non-const methods should be annotated with
// BELA_EXCLUSIVE_LOCKS_REQUIRED, while const methods should be annotated with
// BELA_SHARED_LOCKS_REQUIRED.
//
// Example:
//
//   Mutex mu1, mu2;
//   int a BELA_GUARDED_BY(mu1);
//   int b BELA_GUARDED_BY(mu2);
//
//   void foo() BELA_EXCLUSIVE_LOCKS_REQUIRED(mu1, mu2) { ... }
//   void bar() const BELA_SHARED_LOCKS_REQUIRED(mu1, mu2) { ... }
#if BELA_HAVE_ATTRIBUTE(exclusive_locks_required)
#define BELA_EXCLUSIVE_LOCKS_REQUIRED(...) __attribute__((exclusive_locks_required(__VA_ARGS__)))
#else
#define BELA_EXCLUSIVE_LOCKS_REQUIRED(...)
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define BELA_FORCE_INLINE __forceinline
#define BELA_ATTRIBUTE_ALWAYS_INLINE
#else
#define BELA_ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))
#define BELA_FORCE_INLINE inline BELA_ATTRIBUTE_ALWAYS_INLINE
#endif

#if (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define BELA_IS_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BELA_IS_BIG_ENDIAN 1
#elif defined(_WIN32)
#define BELA_IS_LITTLE_ENDIAN 1
#else
#error "bela endian detection needs to be set up for your compiler"
#endif

#endif
