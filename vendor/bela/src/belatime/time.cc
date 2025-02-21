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
#include <bela/time.hpp>

namespace bela {
// Floors d to the next unit boundary closer to negative infinity.
inline int64_t FloorToUnit(bela::Duration d, bela::Duration unit) {
  bela::Duration rem;
  int64_t q = bela::IDivDuration(d, unit, &rem);
  return (q > 0 || rem >= ZeroDuration() || q == (std::numeric_limits<int64_t>::min)()) ? q : q - 1;
}

bela::Time FromUDate(double udate) { return time_internal::FromUnixDuration(bela::Milliseconds(udate)); }

bela::Time FromUniversal(int64_t universal) { return bela::UniversalEpoch() + 100 * bela::Nanoseconds(universal); }

int64_t ToUnixNanos(Time t) {
  if (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >= 0 &&
      time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >> 33 == 0) {
    return (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) * 1000 * 1000 * 1000) +
           (time_internal::GetRepLo(time_internal::ToUnixDuration(t)) / 4);
  }
  return FloorToUnit(time_internal::ToUnixDuration(t), bela::Nanoseconds(1));
}

int64_t ToUnixMicros(Time t) {
  if (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >= 0 &&
      time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >> 43 == 0) {
    return (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) * 1000 * 1000) +
           (time_internal::GetRepLo(time_internal::ToUnixDuration(t)) / 4000);
  }
  return FloorToUnit(time_internal::ToUnixDuration(t), bela::Microseconds(1));
}

int64_t ToUnixMillis(Time t) {
  if (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >= 0 &&
      time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >> 53 == 0) {
    return (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) * 1000) +
           (time_internal::GetRepLo(time_internal::ToUnixDuration(t)) / (4000 * 1000));
  }
  return FloorToUnit(time_internal::ToUnixDuration(t), bela::Milliseconds(1));
}

int64_t ToUnixSeconds(Time t) { return time_internal::GetRepHi(time_internal::ToUnixDuration(t)); }

time_t ToTimeT(Time t) { return bela::ToTimespec(t).tv_sec; }

double ToUDate(Time t) { return bela::FDivDuration(time_internal::ToUnixDuration(t), bela::Milliseconds(1)); }

int64_t ToUniversal(bela::Time t) { return bela::FloorToUnit(t - bela::UniversalEpoch(), bela::Nanoseconds(100)); }

bela::Time TimeFromTimespec(timespec ts) { return time_internal::FromUnixDuration(bela::DurationFromTimespec(ts)); }

bela::Time TimeFromTimeval(timeval tv) { return time_internal::FromUnixDuration(bela::DurationFromTimeval(tv)); }

timespec ToTimespec(Time t) {
  timespec ts;
  bela::Duration d = time_internal::ToUnixDuration(t);
  if (!time_internal::IsInfiniteDuration(d)) {
    ts.tv_sec = time_internal::GetRepHi(d);
    if (ts.tv_sec == time_internal::GetRepHi(d)) { // no time_t narrowing
      ts.tv_nsec = time_internal::GetRepLo(d) / 4; // floor
      return ts;
    }
  }
  if (d >= bela::ZeroDuration()) {
    ts.tv_sec = (std::numeric_limits<time_t>::max)();
    ts.tv_nsec = 1000 * 1000 * 1000 - 1;
  } else {
    ts.tv_sec = (std::numeric_limits<time_t>::min)();
    ts.tv_nsec = 0;
  }
  return ts;
}

timeval ToTimeval(Time t) {
  timeval tv;
  timespec ts = bela::ToTimespec(t);
  tv.tv_sec = static_cast<long>(ts.tv_sec);
  if (tv.tv_sec != ts.tv_sec) { // narrowing
    if (ts.tv_sec < 0) {
      tv.tv_sec = (std::numeric_limits<decltype(tv.tv_sec)>::min)();
      tv.tv_usec = 0;
    } else {
      tv.tv_sec = (std::numeric_limits<decltype(tv.tv_sec)>::max)();
      tv.tv_usec = 1000 * 1000 - 1;
    }
    return tv;
  }
  tv.tv_usec = static_cast<int>(ts.tv_nsec / 1000); // suseconds_t
  return tv;
}

Time FromChrono(const std::chrono::system_clock::time_point &tp) {
  return time_internal::FromUnixDuration(time_internal::FromChrono(tp - std::chrono::system_clock::from_time_t(0)));
}

std::chrono::system_clock::time_point ToChronoTime(bela::Time t) {
  using D = std::chrono::system_clock::duration;
  auto d = time_internal::ToUnixDuration(t);
  if (d < ZeroDuration()) {
    d = Floor(d, FromChrono(D{1}));
  }
  return std::chrono::system_clock::from_time_t(0) + time_internal::ToChronoDuration<D>(d);
}

} // namespace bela