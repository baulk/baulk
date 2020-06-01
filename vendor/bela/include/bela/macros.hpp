//////
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
#define BELA_ASSERT(expr)                                                                          \
  (BELA_PREDICT_TRUE((expr)) ? static_cast<void>(0) : [] { assert(false && #expr); }()) // NOLINT
#endif

#endif
