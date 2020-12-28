///
#include <bela/datetime.hpp>
#include <bela/fmt.hpp>

namespace bela {

// The unsigned zero year for internal calculations.
// Must be 1 mod 400, and times before it will not compute correctly,
// but otherwise can be changed at will.
constexpr int64_t absoluteZeroYear = -292277022399;
// The year of the zero Time.
// Assumed by the unixToInternal computation below.
constexpr int64_t internalYear = 1;

constexpr int64_t unixToInternal = unixEpochDays * secondsPerDay;
constexpr int64_t internalToUnix = -unixToInternal;
constexpr int64_t wallToInternal = (1884 * 365 + 1884 / 4 - 1884 / 100 + 1884 / 400) * secondsPerDay;
constexpr int64_t internalToWall = -wallToInternal;

constexpr const wchar_t *longDayNames[] = {
    L"Sunday", L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday",
};

constexpr const wchar_t *shortDayNames[] = {
    L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat",
};

constexpr const wchar_t *shortMonthNames[] = {
    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec",
};

constexpr const wchar_t *longMonthNames[] = {
    L"January", L"February", L"March",     L"April",   L"May",      L"June",
    L"July",    L"August",   L"September", L"October", L"November", L"December",
};

std::wstring_view WeekdayName(Weekday wd, bool shortname) noexcept {
  if (wd < Sunday || wd > Saturday) {
    return L"";
  }
  return shortname ? shortDayNames[static_cast<int>(wd)] : longDayNames[static_cast<int>(wd)];
}

std::wstring_view MonthName(Month mon, bool shortname) noexcept {
  if (mon < January || mon > December) {
    return L"";
  }
  return shortname ? shortMonthNames[static_cast<int>(mon) - 1] : longMonthNames[static_cast<int>(mon) - 1];
}

namespace time_internal {

/* 2000-03-01 (mod 400 year, immediately after feb29 */
constexpr auto LeapEpoch = (946684800LL + 86400 * (31 + 29));
// https://en.cppreference.com/w/c/chrono/tm
bool MakeDateTime(int64_t second, DateTime &dt) {
  static constexpr const uint8_t days_in_month[] = {31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29};
  second -= LeapEpoch;
  auto days = second / secondsPerDay;
  auto remsecs = second % secondsPerDay;
  if (remsecs < 0) {
    remsecs += 86400;
    days--;
  }

  // 2000-03-01 Wednesday
  auto wday = (3 + days) % 7;
  if (wday < 0) {
    wday += 7;
  }
  // days since Sunday – [0, 6]
  dt.wday = static_cast<Weekday>(wday);

  auto qccycles = days / daysPer400Years;
  auto remdays = days % daysPer400Years;
  if (remdays < 0) {
    remdays += daysPer400Years;
    qccycles--;
  }

  auto ccycles = remdays / daysPer100Years;
  if (ccycles == 4) {
    ccycles--;
  }
  remdays -= ccycles * daysPer100Years;

  auto qcycles = remdays / daysPer4Years;
  if (qcycles == 25) {
    qcycles--;
  }
  remdays -= qcycles * daysPer4Years;

  auto remyears = remdays / 365;
  if (remyears == 4) {
    remyears--;
  }
  remdays -= remyears * 365;

  auto leap = (remyears == 0) && (qcycles != 0 || ccycles == 0);
  auto yday = remdays + 31 + 28 + (leap ? 1 : 0);
  if (yday >= 365 + (leap ? 1 : 0)) {
    yday -= 365 + (leap ? 1 : 0);
  }
  auto years = remyears + 4 * qcycles + 100 * ccycles + 400LL * qccycles;
  int months = 0;
  for (; days_in_month[months] <= remdays; months++) {
    remdays -= days_in_month[months];
  }

  if (months >= 10) {
    months -= 12;
    years++;
  }

  if (years + 100 > INT_MAX || years + 100 < INT_MIN) {
    return false;
  }

  // Port from musl.
  // years since 1900
  dt.year = years + 100 + 1900;
  // months since January – [0, 11]
  dt.month = static_cast<Month>(months + 2 + 1);
  dt.day = static_cast<std::int_least8_t>(remdays + 1);
  dt.hour = static_cast<std::int_least8_t>(remsecs / secondsPerHour);                        // 28937
  dt.minute = static_cast<std::int_least8_t>((remsecs % secondsPerHour) / secondsPerMinute); // 137
  dt.second = static_cast<std::int_least8_t>(remsecs % secondsPerMinute);
  return true;
}
}; // namespace time_internal

// From bela::Time
DateTime::DateTime(bela::Time t, std::int_least32_t tz) noexcept {
  tzoffset = tz;
  auto parts = bela::Split(t);
  nsec = parts.nsec;
  time_internal::MakeDateTime(parts.sec, *this);
}

bela::Time DateTime::Time() const noexcept {
  if (second < 0 || second > 59 || minute > 59 || minute < 0 || hour < 0 || hour > 23 || month < 1 || month > 12 ||
      year < 1970) {
    return bela::UnixEpoch();
  }

  auto rep_hi = time_internal::DaysSinceEpoch(year, month, day) * secondsPerDay + hour * 3600 + minute * 60 + second;
  rep_hi += tzoffset;

  const auto d = time_internal::MakeDuration(rep_hi, nsec * time_internal::kTicksPerNanosecond);
  return time_internal::FromUnixDuration(d);
}

} // namespace bela