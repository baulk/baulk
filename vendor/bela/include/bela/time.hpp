// ---------------------------------------------------------------------------
// Copyright © 2025, Bela contributors
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
// Copyright 2017 The Abseil Authors.
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
#ifndef BELA_TIME_HPP
#define BELA_TIME_HPP
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <string>
#include <type_traits>
#include <utility>
#include <bit>
#include "base.hpp"

#ifndef _WINSOCKAPI_
struct timeval {
  long tv_sec{0};  /* seconds */
  long tv_usec{0}; /* and microseconds */
};
#endif

namespace bela {

class Duration; // Defined below
class Time;     // Defined below

namespace time_internal {
int64_t IDivDuration(bool satq, Duration num, Duration den, Duration *rem);
constexpr Time FromUnixDuration(Duration d);
constexpr Duration ToUnixDuration(Time t);
constexpr int64_t GetRepHi(Duration d);
constexpr uint32_t GetRepLo(Duration d);
constexpr Duration MakeDuration(int64_t hi, uint32_t lo);
constexpr Duration MakeDuration(int64_t hi, int64_t lo);
inline Duration MakePosDoubleDuration(double n);
constexpr int64_t kTicksPerNanosecond = 4;
constexpr int64_t kTicksPerSecond = 1000 * 1000 * 1000 * kTicksPerNanosecond;
template <std::intmax_t N> constexpr Duration FromInt64(int64_t v, std::ratio<1, N>);
constexpr Duration FromInt64(int64_t v, std::ratio<60>);
constexpr Duration FromInt64(int64_t v, std::ratio<3600>);
} // namespace time_internal

class Duration {
public:
  // Value semantics.
  constexpr Duration() : rep_hi_(0), rep_lo_(0) {} // zero-length duration

  // Copyable.
#if !defined(__clang__) && defined(_MSC_VER) && _MSC_VER < 1910
  // Explicitly defining the constexpr copy constructor avoids an MSVC bug.
  constexpr Duration(const Duration &d) : rep_hi_(d.rep_hi_), rep_lo_(d.rep_lo_) {}
#else
  constexpr Duration(const Duration &d) = default;
#endif
  Duration &operator=(const Duration &d) = default;

  // Compound assignment operators.
  Duration &operator+=(Duration rhs);
  Duration &operator-=(Duration rhs);
  Duration &operator*=(int64_t r);
  Duration &operator*=(double r);
  Duration &operator/=(int64_t r);
  Duration &operator/=(double r);
  Duration &operator%=(Duration rhs);

  // Overloads that forward to either the int64_t or double overloads above.
  // Integer operands must be representable as int64_t.
  template <typename T>
    requires bela::integral_superset<T>
  Duration &operator*=(T r) {
    int64_t x = r;
    return *this *= x;
  }

  template <typename T>
    requires bela::integral_superset<T>
  Duration &operator/=(T r) {
    int64_t x = r;
    return *this /= x;
  }

  template <typename T>
    requires std::floating_point<T>
  Duration &operator*=(T r) {
    double x = r;
    return *this *= x;
  }

  template <typename T>
    requires std::floating_point<T>
  Duration &operator/=(T r) {
    double x = r;
    return *this /= x;
  }

  template <typename H> friend H AbslHashValue(H h, Duration d) {
    return H::combine(std::move(h), d.rep_hi_, d.rep_lo_);
  }

private:
  friend constexpr int64_t time_internal::GetRepHi(Duration d);
  friend constexpr uint32_t time_internal::GetRepLo(Duration d);
  friend constexpr Duration time_internal::MakeDuration(int64_t hi, uint32_t lo);
  constexpr Duration(int64_t hi, uint32_t lo) : rep_hi_(hi), rep_lo_(lo) {}
  int64_t rep_hi_;
  uint32_t rep_lo_;
};

// Relational Operators
constexpr bool operator<(Duration lhs, Duration rhs);
constexpr bool operator>(Duration lhs, Duration rhs) { return rhs < lhs; }
constexpr bool operator>=(Duration lhs, Duration rhs) { return !(lhs < rhs); }
constexpr bool operator<=(Duration lhs, Duration rhs) { return !(rhs < lhs); }
constexpr bool operator==(Duration lhs, Duration rhs);
constexpr bool operator!=(Duration lhs, Duration rhs) { return !(lhs == rhs); }

// Additive Operators
constexpr Duration operator-(Duration d);
inline Duration operator+(Duration lhs, Duration rhs) { return lhs += rhs; }
inline Duration operator-(Duration lhs, Duration rhs) { return lhs -= rhs; }

// Multiplicative Operators
// Integer operands must be representable as int64_t.
template <typename T> Duration operator*(Duration lhs, T rhs) { return lhs *= rhs; }
template <typename T> Duration operator*(T lhs, Duration rhs) { return rhs *= lhs; }
template <typename T> Duration operator/(Duration lhs, T rhs) { return lhs /= rhs; }
inline int64_t operator/(Duration lhs, Duration rhs) {
  return time_internal::IDivDuration(true, lhs, rhs,
                                     &lhs); // trunc towards zero
}
inline Duration operator%(Duration lhs, Duration rhs) { return lhs %= rhs; }

// IDivDuration()
//
// Divides a numerator `Duration` by a denominator `Duration`, returning the
// quotient and remainder. The remainder always has the same sign as the
// numerator. The returned quotient and remainder respect the identity:
//
//   numerator = denominator * quotient + remainder
//
// Returned quotients are capped to the range of `int64_t`, with the difference
// spilling into the remainder to uphold the above identity. This means that the
// remainder returned could differ from the remainder returned by
// `Duration::operator%` for huge quotients.
//
// See also the notes on `InfiniteDuration()` below regarding the behavior of
// division involving zero and infinite durations.
//
// Example:
//
//   constexpr bela::Duration a =
//       bela::Seconds(std::numeric_limits<int64_t>::max());  // big
//   constexpr bela::Duration b = bela::Nanoseconds(1);       // small
//
//   bela::Duration rem = a % b;
//   // rem == bela::ZeroDuration()
//
//   // Here, q would overflow int64_t, so rem accounts for the difference.
//   int64_t q = bela::IDivDuration(a, b, &rem);
//   // q == std::numeric_limits<int64_t>::max(), rem == a - b * q
inline int64_t IDivDuration(Duration num, Duration den, Duration *rem) {
  return time_internal::IDivDuration(true, num, den,
                                     rem); // trunc towards zero
}

// FDivDuration()
//
// Divides a `Duration` numerator into a fractional number of units of a
// `Duration` denominator.
//
// See also the notes on `InfiniteDuration()` below regarding the behavior of
// division involving zero and infinite durations.
//
// Example:
//
//   double d = bela::FDivDuration(bela::Milliseconds(1500), bela::Seconds(1));
//   // d == 1.5
double FDivDuration(Duration num, Duration den);

// ZeroDuration()
//
// Returns a zero-length duration. This function behaves just like the default
// constructor, but the name helps make the semantics clear at call sites.
constexpr Duration ZeroDuration() { return Duration(); }

// AbsDuration()
//
// Returns the absolute value of a duration.
inline Duration AbsDuration(Duration d) { return (d < ZeroDuration()) ? -d : d; }

// Trunc()
//
// Truncates a duration (toward zero) to a multiple of a non-zero unit.
//
// Example:
//
//   bela::Duration d = bela::Nanoseconds(123456789);
//   bela::Duration a = bela::Trunc(d, bela::Microseconds(1));  // 123456us
Duration Trunc(Duration d, Duration unit);

// Floor()
//
// Floors a duration using the passed duration unit to its largest value not
// greater than the duration.
//
// Example:
//
//   bela::Duration d = bela::Nanoseconds(123456789);
//   bela::Duration b = bela::Floor(d, bela::Microseconds(1));  // 123456us
Duration Floor(Duration d, Duration unit);

// Ceil()
//
// Returns the ceiling of a duration using the passed duration unit to its
// smallest value not less than the duration.
//
// Example:
//
//   bela::Duration d = bela::Nanoseconds(123456789);
//   bela::Duration c = bela::Ceil(d, bela::Microseconds(1));   // 123457us
Duration Ceil(Duration d, Duration unit);

// InfiniteDuration()
//
// Returns an infinite `Duration`.  To get a `Duration` representing negative
// infinity, use `-InfiniteDuration()`.
//
// Duration arithmetic overflows to +/- infinity and saturates. In general,
// arithmetic with `Duration` infinities is similar to IEEE 754 infinities
// except where IEEE 754 NaN would be involved, in which case +/-
// `InfiniteDuration()` is used in place of a "nan" Duration.
//
// Examples:
//
//   constexpr bela::Duration inf = bela::InfiniteDuration();
//   const bela::Duration d = ... any finite duration ...
//
//   inf == inf + inf
//   inf == inf + d
//   inf == inf - inf
//   -inf == d - inf
//
//   inf == d * 1e100
//   inf == inf / 2
//   0 == d / inf
//   INT64_MAX == inf / d
//
//   d < inf
//   -inf < d
//
//   // Division by zero returns infinity, or INT64_MIN/MAX where appropriate.
//   inf == d / 0
//   INT64_MAX == d / bela::ZeroDuration()
//
// The examples involving the `/` operator above also apply to `IDivDuration()`
// and `FDivDuration()`.
constexpr Duration InfiniteDuration();

// Nanoseconds()
// Microseconds()
// Milliseconds()
// Seconds()
// Minutes()
// Hours()
//
// Factory functions for constructing `Duration` values from an integral number
// of the unit indicated by the factory function's name. The number must be
// representable as int64_t.
//
// NOTE: no "Days()" factory function exists because "a day" is ambiguous.
// Civil days are not always 24 hours long, and a 24-hour duration often does
// not correspond with a civil day. If a 24-hour duration is needed, use
// `bela::Hours(24)`. If you actually want a civil day, use bela::CivilDay
// from civil_time.h.
//
// Example:
//
//   bela::Duration a = bela::Seconds(60);
//   bela::Duration b = bela::Minutes(1);  // b == a
template <typename T>
  requires bela::integral_superset<T>
constexpr Duration Nanoseconds(T n) {
  return time_internal::FromInt64(n, std::nano{});
}
template <typename T>
  requires bela::integral_superset<T>
constexpr Duration Microseconds(T n) {
  return time_internal::FromInt64(n, std::micro{});
}
template <typename T>
  requires bela::integral_superset<T>
constexpr Duration Milliseconds(T n) {
  return time_internal::FromInt64(n, std::milli{});
}
template <typename T>
  requires bela::integral_superset<T>
constexpr Duration Seconds(T n) {
  return time_internal::FromInt64(n, std::ratio<1>{});
}
template <typename T>
  requires bela::integral_superset<T>
constexpr Duration Minutes(T n) {
  return time_internal::FromInt64(n, std::ratio<60>{});
}
template <typename T>
  requires bela::integral_superset<T>
constexpr Duration Hours(T n) {
  return time_internal::FromInt64(n, std::ratio<3600>{});
}

// Factory overloads for constructing `Duration` values from a floating-point
// number of the unit indicated by the factory function's name. These functions
// exist for convenience, but they are not as efficient as the integral
// factories, which should be preferred.
//
// Example:
//
//   auto a = bela::Seconds(1.5);        // OK
//   auto b = bela::Milliseconds(1500);  // BETTER
template <typename T>
  requires std::floating_point<T>
Duration Nanoseconds(T n) {
  return n * Nanoseconds(1);
}
template <typename T>
  requires std::floating_point<T>
Duration Microseconds(T n) {
  return n * Microseconds(1);
}
template <typename T>
  requires std::floating_point<T>
Duration Milliseconds(T n) {
  return n * Milliseconds(1);
}
template <typename T>
  requires std::floating_point<T>
Duration Seconds(T n) {
  if (n >= 0) { // Note: `NaN >= 0` is false.
    if (n >= static_cast<T>((std::numeric_limits<int64_t>::max)())) {
      return InfiniteDuration();
    }
    return time_internal::MakePosDoubleDuration(n);
  } else {
    if (std::isnan(n))
      return std::signbit(n) ? -InfiniteDuration() : InfiniteDuration();
    if (n <= (std::numeric_limits<int64_t>::min)())
      return -InfiniteDuration();
    return -time_internal::MakePosDoubleDuration(-n);
  }
}
template <typename T>
  requires std::floating_point<T>
Duration Minutes(T n) {
  return n * Minutes(1);
}
template <typename T>
  requires std::floating_point<T>
Duration Hours(T n) {
  return n * Hours(1);
}

// ToInt64Nanoseconds()
// ToInt64Microseconds()
// ToInt64Milliseconds()
// ToInt64Seconds()
// ToInt64Minutes()
// ToInt64Hours()
//
// Helper functions that convert a Duration to an integral count of the
// indicated unit. These functions are shorthand for the `IDivDuration()`
// function above; see its documentation for details about overflow, etc.
//
// Example:
//
//   bela::Duration d = bela::Milliseconds(1500);
//   int64_t isec = bela::ToInt64Seconds(d);  // isec == 1
int64_t ToInt64Nanoseconds(Duration d);
int64_t ToInt64Microseconds(Duration d);
int64_t ToInt64Milliseconds(Duration d);
int64_t ToInt64Seconds(Duration d);
int64_t ToInt64Minutes(Duration d);
int64_t ToInt64Hours(Duration d);

// ToDoubleNanoSeconds()
// ToDoubleMicroseconds()
// ToDoubleMilliseconds()
// ToDoubleSeconds()
// ToDoubleMinutes()
// ToDoubleHours()
//
// Helper functions that convert a Duration to a floating point count of the
// indicated unit. These functions are shorthand for the `FDivDuration()`
// function above; see its documentation for details about overflow, etc.
//
// Example:
//
//   bela::Duration d = bela::Milliseconds(1500);
//   double dsec = bela::ToDoubleSeconds(d);  // dsec == 1.5
double ToDoubleNanoseconds(Duration d);
double ToDoubleMicroseconds(Duration d);
double ToDoubleMilliseconds(Duration d);
double ToDoubleSeconds(Duration d);
double ToDoubleMinutes(Duration d);
double ToDoubleHours(Duration d);

// FromChrono()
//
// Converts any of the pre-defined std::chrono durations to an bela::Duration.
//
// Example:
//
//   std::chrono::milliseconds ms(123);
//   bela::Duration d = bela::FromChrono(ms);
constexpr Duration FromChrono(const std::chrono::nanoseconds &d);
constexpr Duration FromChrono(const std::chrono::microseconds &d);
constexpr Duration FromChrono(const std::chrono::milliseconds &d);
constexpr Duration FromChrono(const std::chrono::seconds &d);
constexpr Duration FromChrono(const std::chrono::minutes &d);
constexpr Duration FromChrono(const std::chrono::hours &d);

// ToChronoNanoseconds()
// ToChronoMicroseconds()
// ToChronoMilliseconds()
// ToChronoSeconds()
// ToChronoMinutes()
// ToChronoHours()
//
// Converts an bela::Duration to any of the pre-defined std::chrono durations.
// If overflow would occur, the returned value will saturate at the min/max
// chrono duration value instead.
//
// Example:
//
//   bela::Duration d = bela::Microseconds(123);
//   auto x = bela::ToChronoMicroseconds(d);
//   auto y = bela::ToChronoNanoseconds(d);  // x == y
//   auto z = bela::ToChronoSeconds(bela::InfiniteDuration());
//   // z == std::chrono::seconds::max()
std::chrono::nanoseconds ToChronoNanoseconds(Duration d);
std::chrono::microseconds ToChronoMicroseconds(Duration d);
std::chrono::milliseconds ToChronoMilliseconds(Duration d);
std::chrono::seconds ToChronoSeconds(Duration d);
std::chrono::minutes ToChronoMinutes(Duration d);
std::chrono::hours ToChronoHours(Duration d);

std::wstring FormatDuration(Duration d);
bool ParseDuration(std::wstring_view dur_sv, Duration *d);

class Time {
public:
  // Value semantics.

  // Returns the Unix epoch.  However, those reading your code may not know
  // or expect the Unix epoch as the default value, so make your code more
  // readable by explicitly initializing all instances before use.
  //
  // Example:
  //   bela::Time t = bela::UnixEpoch();
  //   bela::Time t = bela::Now();
  //   bela::Time t = bela::TimeFromTimeval(tv);
  //   bela::Time t = bela::InfinitePast();
  constexpr Time() = default;

  // Copyable.
  constexpr Time(const Time &t) = default;
  Time &operator=(const Time &t) = default;

  // Assignment operators.
  Time &operator+=(Duration d) {
    rep_ += d;
    return *this;
  }
  Time &operator-=(Duration d) {
    rep_ -= d;
    return *this;
  }

private:
  friend constexpr Time time_internal::FromUnixDuration(Duration d);
  friend constexpr Duration time_internal::ToUnixDuration(Time t);
  friend constexpr bool operator<(Time lhs, Time rhs);
  friend constexpr bool operator==(Time lhs, Time rhs);
  friend Duration operator-(Time lhs, Time rhs);
  friend constexpr Time UniversalEpoch();
  friend constexpr Time InfiniteFuture();
  friend constexpr Time InfinitePast();
  constexpr explicit Time(Duration rep) : rep_(rep) {}
  Duration rep_;
};

// Relational Operators
constexpr bool operator<(Time lhs, Time rhs) { return lhs.rep_ < rhs.rep_; }
constexpr bool operator>(Time lhs, Time rhs) { return rhs < lhs; }
constexpr bool operator>=(Time lhs, Time rhs) { return !(lhs < rhs); }
constexpr bool operator<=(Time lhs, Time rhs) { return !(rhs < lhs); }
constexpr bool operator==(Time lhs, Time rhs) { return lhs.rep_ == rhs.rep_; }
constexpr bool operator!=(Time lhs, Time rhs) { return !(lhs == rhs); }

// Additive Operators
inline Time operator+(Time lhs, Duration rhs) { return lhs += rhs; }
inline Time operator+(Duration lhs, Time rhs) { return rhs += lhs; }
inline Time operator-(Time lhs, Duration rhs) { return lhs -= rhs; }
inline Duration operator-(Time lhs, Time rhs) { return lhs.rep_ - rhs.rep_; }

// UnixEpoch()
//
// Returns the `bela::Time` representing "1970-01-01 00:00:00.0 +0000".
constexpr Time UnixEpoch() { return Time(); }

// UniversalEpoch()
//
// Returns the `bela::Time` representing "0001-01-01 00:00:00.0 +0000", the
// epoch of the ICU Universal Time Scale.
constexpr Time UniversalEpoch() {
  // 719162 is the number of days from 0001-01-01 to 1970-01-01,
  // assuming the Gregorian calendar.
  return Time(time_internal::MakeDuration(-24 * 719162 * int64_t{3600}, 0U));
}

// InfiniteFuture()
//
// Returns an `bela::Time` that is infinitely far in the future.
constexpr Time InfiniteFuture() {
  return Time(time_internal::MakeDuration((std::numeric_limits<int64_t>::max)(), ~0U));
}

// InfinitePast()
//
// Returns an `bela::Time` that is infinitely far in the past.
constexpr Time InfinitePast() { return Time(time_internal::MakeDuration((std::numeric_limits<int64_t>::min)(), ~0U)); }

// FromUnixNanos()
// FromUnixMicros()
// FromUnixMillis()
// FromUnixSeconds()
// FromTimeT()
// FromUDate()
// FromUniversal()
//
// Creates an `bela::Time` from a variety of other representations.
constexpr Time FromUnixNanos(int64_t ns);
constexpr Time FromUnixMicros(int64_t us);
constexpr Time FromUnixMillis(int64_t ms);
constexpr Time FromUnixSeconds(int64_t s);
constexpr Time FromTimeT(time_t t);
Time FromUDate(double udate);
Time FromUniversal(int64_t universal);

// ToUnixNanos()
// ToUnixMicros()
// ToUnixMillis()
// ToUnixSeconds()
// ToTimeT()
// ToUDate()
// ToUniversal()
//
// Converts an `bela::Time` to a variety of other representations.  Note that
// these operations round down toward negative infinity where necessary to
// adjust to the resolution of the result type.  Beware of possible time_t
// over/underflow in ToTime{T,val,spec}() on 32-bit platforms.
int64_t ToUnixNanos(Time t);
int64_t ToUnixMicros(Time t);
int64_t ToUnixMillis(Time t);
int64_t ToUnixSeconds(Time t);

time_t ToTimeT(Time t);
double ToUDate(Time t);
int64_t ToUniversal(Time t);

// DurationFromTimespec()
// DurationFromTimeval()
// ToTimespec()
// ToTimeval()
// TimeFromTimespec()
// TimeFromTimeval()
// ToTimespec()
// ToTimeval()
//
// Some APIs use a timespec or a timeval as a Duration (e.g., nanosleep(2)
// and select(2)), while others use them as a Time (e.g. clock_gettime(2)
// and gettimeofday(2)), so conversion functions are provided for both cases.
// The "to timespec/val" direction is easily handled via overloading, but
// for "from timespec/val" the desired type is part of the function name.
Duration DurationFromTimespec(timespec ts);
Duration DurationFromTimeval(timeval tv);
timespec ToTimespec(Duration d);
timeval ToTimeval(Duration d);
Time TimeFromTimespec(timespec ts);
Time TimeFromTimeval(timeval tv);
timespec ToTimespec(Time t);
timeval ToTimeval(Time t);

// FromChrono()
//
// Converts a std::chrono::system_clock::time_point to an bela::Time.
//
// Example:
//
//   auto tp = std::chrono::system_clock::from_time_t(123);
//   bela::Time t = bela::FromChrono(tp);
//   // t == bela::FromTimeT(123)
Time FromChrono(const std::chrono::system_clock::time_point &tp);

// ToChronoTime()
//
// Converts an bela::Time to a std::chrono::system_clock::time_point. If
// overflow would occur, the returned value will saturate at the min/max time
// point value instead.
//
// Example:
//
//   bela::Time t = bela::FromTimeT(123);
//   auto tp = bela::ToChronoTime(t);
//   // tp == std::chrono::system_clock::from_time_t(123);
std::chrono::system_clock::time_point ToChronoTime(Time);
// ============================================================================
// Implementation Details Follow
// ============================================================================

namespace time_internal {

// Creates a Duration with a given representation.
// REQUIRES: hi,lo is a valid representation of a Duration as specified
// in time/duration.cc.
constexpr Duration MakeDuration(int64_t hi, uint32_t lo = 0) { return Duration(hi, lo); }

constexpr Duration MakeDuration(int64_t hi, int64_t lo) { return MakeDuration(hi, static_cast<uint32_t>(lo)); }

// Make a Duration value from a floating-point number, as long as that number
// is in the range [ 0 .. numeric_limits<int64_t>::max ), that is, as long as
// it's positive and can be converted to int64_t without risk of UB.
inline Duration MakePosDoubleDuration(double n) {
  const int64_t int_secs = static_cast<int64_t>(n);
  const uint32_t ticks = static_cast<uint32_t>((n - static_cast<double>(int_secs)) * kTicksPerSecond + 0.5);
  return ticks < kTicksPerSecond ? MakeDuration(int_secs, ticks) : MakeDuration(int_secs + 1, ticks - kTicksPerSecond);
}

// Creates a normalized Duration from an almost-normalized (sec,ticks)
// pair. sec may be positive or negative.  ticks must be in the range
// -kTicksPerSecond < *ticks < kTicksPerSecond.  If ticks is negative it
// will be normalized to a positive value in the resulting Duration.
constexpr Duration MakeNormalizedDuration(int64_t sec, int64_t ticks) {
  return (ticks < 0) ? MakeDuration(sec - 1, ticks + kTicksPerSecond) : MakeDuration(sec, ticks);
}

// Provide access to the Duration representation.
constexpr int64_t GetRepHi(Duration d) { return d.rep_hi_; }
constexpr uint32_t GetRepLo(Duration d) { return d.rep_lo_; }

// Returns true iff d is positive or negative infinity.
constexpr bool IsInfiniteDuration(Duration d) { return GetRepLo(d) == ~0U; }

// Returns an infinite Duration with the opposite sign.
// REQUIRES: IsInfiniteDuration(d)
constexpr Duration OppositeInfinity(Duration d) {
  return GetRepHi(d) < 0 ? MakeDuration((std::numeric_limits<int64_t>::max)(), ~0U)
                         : MakeDuration((std::numeric_limits<int64_t>::min)(), ~0U);
}

// Returns (-n)-1 (equivalently -(n+1)) without avoidable overflow.
constexpr int64_t NegateAndSubtractOne(int64_t n) {
  // Note: Good compilers will optimize this expression to ~n when using
  // a two's-complement representation (which is required for int64_t).
  return (n < 0) ? -(n + 1) : (-n) - 1;
}

// Map between a Time and a Duration since the Unix epoch.  Note that these
// functions depend on the above mentioned choice of the Unix epoch for the
// Time representation (and both need to be Time friends).  Without this
// knowledge, we would need to add-in/subtract-out UnixEpoch() respectively.
constexpr Time FromUnixDuration(Duration d) { return Time(d); }
constexpr Duration ToUnixDuration(Time t) { return t.rep_; }

template <std::intmax_t N> constexpr Duration FromInt64(int64_t v, std::ratio<1, N>) {
  static_assert(0 < N && N <= 1000 * 1000 * 1000, "Unsupported ratio");
  // Subsecond ratios cannot overflow.
  return MakeNormalizedDuration(v / N, v % N * kTicksPerNanosecond * 1000 * 1000 * 1000 / N);
}
constexpr Duration FromInt64(int64_t v, std::ratio<60>) {
  return (v <= (std::numeric_limits<int64_t>::max)() / 60 && v >= (std::numeric_limits<int64_t>::min)() / 60)
             ? MakeDuration(v * 60)
         : v > 0 ? InfiniteDuration()
                 : -InfiniteDuration();
}
constexpr Duration FromInt64(int64_t v, std::ratio<3600>) {
  return (v <= (std::numeric_limits<int64_t>::max)() / 3600 && v >= (std::numeric_limits<int64_t>::min)() / 3600)
             ? MakeDuration(v * 3600)
         : v > 0 ? InfiniteDuration()
                 : -InfiniteDuration();
}

// IsValidRep64<T>(0) is true if the expression `int64_t{std::declval<T>()}` is
// valid. That is, if a T can be assigned to an int64_t without narrowing.
template <typename T> constexpr auto IsValidRep64(int) -> decltype(int64_t{std::declval<T>()} == 0) { return true; }
template <typename T> constexpr auto IsValidRep64(char) -> bool { return false; }

// Converts a std::chrono::duration to an bela::Duration.
template <typename Rep, typename Period> constexpr Duration FromChrono(const std::chrono::duration<Rep, Period> &d) {
  static_assert(IsValidRep64<Rep>(0), "duration::rep is invalid");
  return FromInt64(int64_t{d.count()}, Period{});
}

template <typename Ratio> int64_t ToInt64(Duration d, Ratio) {
  // Note: This may be used on MSVC, which may have a system_clock period of
  // std::ratio<1, 10 * 1000 * 1000>
  return ToInt64Seconds(d * Ratio::den / Ratio::num);
}
// Fastpath implementations for the 6 common duration units.
inline int64_t ToInt64(Duration d, std::nano) { return ToInt64Nanoseconds(d); }
inline int64_t ToInt64(Duration d, std::micro) { return ToInt64Microseconds(d); }
inline int64_t ToInt64(Duration d, std::milli) { return ToInt64Milliseconds(d); }
inline int64_t ToInt64(Duration d, std::ratio<1>) { return ToInt64Seconds(d); }
inline int64_t ToInt64(Duration d, std::ratio<60>) { return ToInt64Minutes(d); }
inline int64_t ToInt64(Duration d, std::ratio<3600>) { return ToInt64Hours(d); }

// Converts an bela::Duration to a chrono duration of type T.
template <typename T> T ToChronoDuration(Duration d) {
  using Rep = typename T::rep;
  using Period = typename T::period;
  static_assert(IsValidRep64<Rep>(0), "duration::rep is invalid");
  if (time_internal::IsInfiniteDuration(d)) {
    return d < ZeroDuration() ? (T::min)() : (T::max)();
  }
  const auto v = ToInt64(d, Period{});
  if (v > (std::numeric_limits<Rep>::max)()) {
    return (T::max)();
  }
  if (v < (std::numeric_limits<Rep>::min)()) {
    return (T::min)();
  }
  return T{v};
}

} // namespace time_internal

constexpr bool operator<(Duration lhs, Duration rhs) {
  return time_internal::GetRepHi(lhs) != time_internal::GetRepHi(rhs)
             ? time_internal::GetRepHi(lhs) < time_internal::GetRepHi(rhs)
         : time_internal::GetRepHi(lhs) == (std::numeric_limits<int64_t>::min)()
             ? time_internal::GetRepLo(lhs) + 1 < time_internal::GetRepLo(rhs) + 1
             : time_internal::GetRepLo(lhs) < time_internal::GetRepLo(rhs);
}

constexpr bool operator==(Duration lhs, Duration rhs) {
  return time_internal::GetRepHi(lhs) == time_internal::GetRepHi(rhs) &&
         time_internal::GetRepLo(lhs) == time_internal::GetRepLo(rhs);
}

constexpr Duration operator-(Duration d) {
  // This is a little interesting because of the special cases.
  //
  // If rep_lo_ is zero, we have it easy; it's safe to negate rep_hi_, we're
  // dealing with an integral number of seconds, and the only special case is
  // the maximum negative finite duration, which can't be negated.
  //
  // Infinities stay infinite, and just change direction.
  //
  // Finally we're in the case where rep_lo_ is non-zero, and we can borrow
  // a second's worth of ticks and avoid overflow (as negating int64_t-min + 1
  // is safe).
  return time_internal::GetRepLo(d) == 0 ? time_internal::GetRepHi(d) == (std::numeric_limits<int64_t>::min)()
                                               ? InfiniteDuration()
                                               : time_internal::MakeDuration(-time_internal::GetRepHi(d))
         : time_internal::IsInfiniteDuration(d)
             ? time_internal::OppositeInfinity(d)
             : time_internal::MakeDuration(time_internal::NegateAndSubtractOne(time_internal::GetRepHi(d)),
                                           time_internal::kTicksPerSecond - time_internal::GetRepLo(d));
}

constexpr Duration InfiniteDuration() {
  return time_internal::MakeDuration((std::numeric_limits<int64_t>::max)(), ~0U);
}

constexpr Duration FromChrono(const std::chrono::nanoseconds &d) { return time_internal::FromChrono(d); }
constexpr Duration FromChrono(const std::chrono::microseconds &d) { return time_internal::FromChrono(d); }
constexpr Duration FromChrono(const std::chrono::milliseconds &d) { return time_internal::FromChrono(d); }
constexpr Duration FromChrono(const std::chrono::seconds &d) { return time_internal::FromChrono(d); }
constexpr Duration FromChrono(const std::chrono::minutes &d) { return time_internal::FromChrono(d); }
constexpr Duration FromChrono(const std::chrono::hours &d) { return time_internal::FromChrono(d); }

constexpr Time FromUnixNanos(int64_t ns) { return time_internal::FromUnixDuration(Nanoseconds(ns)); }

constexpr Time FromUnixMicros(int64_t us) { return time_internal::FromUnixDuration(Microseconds(us)); }

constexpr Time FromUnixMillis(int64_t ms) { return time_internal::FromUnixDuration(Milliseconds(ms)); }

constexpr Time FromUnixSeconds(int64_t s) { return time_internal::FromUnixDuration(Seconds(s)); }

constexpr Time FromUnix(int64_t s, int64_t ns) {
  const uint32_t rep_lo = static_cast<uint32_t>(ns) * time_internal::kTicksPerNanosecond;
  const auto d = time_internal::MakeDuration(s, rep_lo);
  return time_internal::FromUnixDuration(d);
}

constexpr Time FromTimeT(time_t t) { return time_internal::FromUnixDuration(Seconds(t)); }

// Now()
//
// Returns the current time, expressed as an `absl::Time` absolute time value.
Time Now();

// GetCurrentTimeNanos()
//
// Returns the current time, expressed as a count of nanoseconds since the Unix
// Epoch (https://en.wikipedia.org/wiki/Unix_time). Prefer `absl::Now()` instead
// for all but the most performance-sensitive cases (i.e. when you are calling
// this function hundreds of thousands of times per second).
int64_t GetCurrentTimeNanos();

// SleepFor()
//
// Sleeps for the specified duration, expressed as an `absl::Duration`.
//
// Notes:
// * Signal interruptions will not reduce the sleep duration.
// * Returns immediately when passed a nonpositive duration.
void SleepFor(bela::Duration duration);

// FromDosDateTime() convert dos time to bela::Time
Time FromDosDateTime(uint16_t dosDate, uint16_t dosTime);

// GetSystemTimePreciseAsFileTime  FILETIME
constexpr Time FromWindowsPreciseTime(uint64_t tick) {
  constexpr auto unixTimeStart = 116444736000000000ui64;
  return FromUnixMicros(static_cast<int64_t>((tick - unixTimeStart) / 10));
}

constexpr Time FromFileTime(FILETIME ft) {
  // Need to bit_cast to fix alignment, then divide by 10 to convert
  // 100-nanoseconds to microseconds. This only works on little-endian
  // machines.
  constexpr auto unixTimeStart = 116444736000000000ui64;
  auto tick = std::bit_cast<int64_t, FILETIME>(ft);
  return FromUnixMicros((tick - unixTimeStart) / 10);
}

struct time_parts {
  int64_t sec{0};
  uint32_t nsec{0};
};

constexpr time_parts Split(Time t) {
  const auto d = time_internal::ToUnixDuration(t);
  const int64_t rep_hi = time_internal::GetRepHi(d);
  const uint32_t rep_lo = time_internal::GetRepLo(d);
  return {rep_hi, static_cast<uint32_t>(rep_lo / time_internal::kTicksPerNanosecond)};
}

constexpr FILETIME ToFileTime(Time t) {
  auto parts = bela::Split(t);
  auto tick = (parts.sec + 11644473600ll) * 10000000 + parts.nsec / 100;
  return {static_cast<DWORD>(tick), static_cast<DWORD>(tick >> 32)};
}

} // namespace bela

extern "C" {
void BelaInternalSleepFor(bela::Duration duration);
}
inline void bela::SleepFor(bela::Duration duration) { BelaInternalSleepFor(duration); }

#endif