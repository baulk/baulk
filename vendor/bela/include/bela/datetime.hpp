// DATETIME
#ifndef BELA_DATETIME_HPP
#define BELA_DATETIME_HPP
#include "time.hpp"

namespace bela {

enum Month : std::int_least8_t {
  January = 1, // start as
  February,
  March,
  April,
  May,
  June,
  July,
  August,
  September,
  October,
  November,
  December,
};

// days since Sunday – [0, 6]
enum Weekday : std::int_least32_t {
  Sunday,
  Monday,
  Tuesday,
  Wednesday,
  Thursday,
  Friday,
  Saturday,
};

constexpr int64_t secondsPerMinute = 60;
constexpr int64_t secondsPerHour = 60 * secondsPerMinute;
constexpr int64_t secondsPerDay = 24 * secondsPerHour;
constexpr int64_t secondsPerWeek = 7 * secondsPerDay;
constexpr int64_t daysPer400Years = 365 * 400 + 97;
constexpr int64_t daysPer100Years = 365 * 100 + 24;
constexpr int64_t daysPer4Years = 365 * 4 + 1;
constexpr int64_t unixEpochDays = 1969 * 365 + 1969 / 4 - 1969 / 100 + 1969 / 400;

constexpr bool IsLeapYear(std::int_least64_t y) { return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); }

class DateTime;
namespace time_internal {
bool MakeDateTime(int64_t second, DateTime &dt);
constexpr int LeapMonths[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
constexpr int Months[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
// Unix epoch
constexpr std::int_least64_t daysSinceEpoch(std::int_least64_t Y) {
  Y--;
  return Y * 365 + Y / 4 - Y / 100 + Y / 400 - unixEpochDays;
}

constexpr int64_t DaysSinceEpoch(std::int_least64_t y, std::int_least8_t mon, std::int_least8_t d) {
  auto leapYear = IsLeapYear(y);
  auto mondays = leapYear ? time_internal::LeapMonths[mon - 1] : time_internal::Months[mon - 1];
  if (d < 1 || d > mondays) {
    return 0;
  }
  auto days = time_internal::daysSinceEpoch(y);
  for (auto i = 1; i < mon; i++) {
    days += time_internal::Months[i - 1];
  }
  if (mon > 2 && leapYear) {
    days++;
  }
  days += d - 1;
  return days;
}

std::wstring FormatDateTimeW(const DateTime &dt, bool nano);
std::string FormatDateTimeA(const DateTime &dt, bool nano);

}; // namespace time_internal

constexpr Weekday GetWeekday(std::int_least64_t y, std::int_least8_t mon, std::int_least8_t d) {
  // 1969-12-31 Wednesday -1 day
  // 1970-01-01 Thursday 0 day
  auto days = time_internal::DaysSinceEpoch(y, mon, d);
  auto wday = (days + 3 + 1) % 7;
  if (wday < 0) {
    wday += 7;
  }
  return static_cast<Weekday>(wday);
}

class DateTime {
public:
  constexpr explicit DateTime(std::int_least64_t y, std::int_least8_t mon, std::int_least8_t d, std::int_least8_t h,
                              std::int_least8_t m, std::int_least8_t s, std::int_least32_t tz = 0) noexcept
      : year(y), month(static_cast<bela::Month>(mon)), day(d), hour(h), minute(m), second(s), tzoffset(tz),
        wday(GetWeekday(y, mon, d)) {}
  DateTime() = default;
  DateTime(bela::Time t, std::int_least32_t tz = 0) noexcept;
  constexpr auto Year() const noexcept { return year; }
  constexpr auto Month() const noexcept { return month; }
  constexpr int Day() const noexcept { return static_cast<int>(day); }
  constexpr int Hour() const noexcept { return static_cast<int>(hour); }
  constexpr int Minute() const noexcept { return static_cast<int>(minute); }
  constexpr int Second() const noexcept { return static_cast<int>(second); }
  constexpr auto TimeZoneOffset() const noexcept { return tzoffset; }
  constexpr auto Weekday() const noexcept { return wday; }
  auto &TimeZoneOffset() noexcept { return tzoffset; }
  // RFC3339

  template <typename CharT = wchar_t, typename Allocator = std::allocator<CharT>>
    requires bela::character<CharT>
  [[nodiscard]] std::basic_string<CharT, std::char_traits<CharT>, Allocator> Format(bool nano = false) const;

  bela::Time Time() const noexcept;

private:
  friend bool time_internal::MakeDateTime(int64_t second, DateTime &dt);
  friend std::wstring time_internal::FormatDateTimeW(const DateTime &dt, bool nano);
  friend std::string time_internal::FormatDateTimeA(const DateTime &dt, bool nano);
  std::int_fast64_t year;
  bela::Month month;
  std::int_least8_t day;
  std::int_least8_t hour;
  std::int_least8_t minute;
  std::int_least8_t second;
  std::int_least32_t nsec{0};
  std::int_least32_t tzoffset{0}; // timezone diff
  bela::Weekday wday;
};

template <> inline std::wstring DateTime::Format<wchar_t>(bool nano) const {
  return time_internal::FormatDateTimeW(*this, nano);
}
template <> inline std::string DateTime::Format<char>(bool nano) const {
  return time_internal::FormatDateTimeA(*this, nano);
}

std::int_least32_t TimeZoneOffset();

inline bela::DateTime LocalDateTime(bela::Time t) {
  auto tzoffset = TimeZoneOffset();
  return bela::DateTime(t - bela::Seconds(tzoffset), tzoffset);
}

template <typename CharT = wchar_t, typename Allocator = std::allocator<CharT>>
  requires bela::character<CharT>
[[nodiscard]] std::basic_string<CharT, std::char_traits<CharT>, Allocator> FormatTime(bela::Time t, bool nano = false) {
  auto dt = LocalDateTime(t);
  return dt.Format<CharT>(nano);
}

template <typename CharT = wchar_t, typename Allocator = std::allocator<CharT>>
  requires bela::character<CharT>
[[nodiscard]] std::basic_string<CharT, std::char_traits<CharT>, Allocator> FormatUniversalTime(bela::Time t,
                                                                                               bool nano = false) {
  return DateTime(t).Format<CharT>(nano);
}

std::wstring_view WeekdayName(Weekday wd, bool shortname = true) noexcept;
std::wstring_view MonthName(Month mon, bool shortname = true) noexcept;
} // namespace bela

#endif