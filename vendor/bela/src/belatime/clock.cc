///
#include <bela/time.hpp>
#include <bela/macros.hpp>
#include <mutex>

namespace bela {

// GetCurrentTimeNanos
int64_t GetCurrentTimeNanos() {
  using namespace std::chrono;
  return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
}

Time Now() {
  // TODO(bww): Get a timespec instead so we don't have to divide.
  int64_t n = bela::GetCurrentTimeNanos();
  if (n >= 0) {
    return time_internal::FromUnixDuration(time_internal::MakeDuration(n / 1000000000, n % 1000000000 * 4));
  }
  return time_internal::FromUnixDuration(bela::Nanoseconds(n));
}
// Returns the maximum duration that SleepOnce() can sleep for.
constexpr bela::Duration MaxSleep() {
#ifdef _WIN32
  // Windows Sleep() takes unsigned long argument in milliseconds.
  return bela::Milliseconds((std::numeric_limits<unsigned long>::max)()); // NOLINT(runtime/int)
#else
  return bela::Seconds(std::numeric_limits<time_t>::max());
#endif
}

// Sleeps for the given duration.
// REQUIRES: to_sleep <= MaxSleep().
void SleepOnce(bela::Duration to_sleep) {
#ifdef _WIN32
  Sleep(static_cast<DWORD>(to_sleep / bela::Milliseconds(1)));
#else
  struct timespec sleep_time = bela::ToTimespec(to_sleep);
  while (nanosleep(&sleep_time, &sleep_time) != 0 && errno == EINTR) {
    // Ignore signals and wait for the full interval to elapse.
  }
#endif
}
} // namespace bela

extern "C" {

BELA_ATTRIBUTE_WEAK void BelaInternalSleepFor(bela::Duration duration) {
  while (duration > bela::ZeroDuration()) {
    bela::Duration to_sleep = (std::min)(duration, bela::MaxSleep());
    bela::SleepOnce(to_sleep);
    duration -= to_sleep;
  }
}

} // extern "C"