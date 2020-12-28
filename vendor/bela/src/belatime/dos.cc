///
#include <bela/time.hpp>
#include <bela/datetime.hpp>

namespace bela {

constexpr int WindowsEpochDays = (1601 - 1) * 365 + (1601 - 1) / 4 - (1601 - 1) / 100 + (1601 - 1) / 400;
constexpr int64_t UnixEpochStart = 11644473600ll;

constexpr inline bool IsLeapYear(int Y) { return Y % 4 == 0 && (Y % 100 != 0 || Y % 400 == 0); }
constexpr int sinceWindowsEpochDays(int Y) {
  Y--;
  return Y * 365 + Y / 4 - Y / 100 + Y / 400 - WindowsEpochDays;
}
// FromDosDateTime dos time to bela::Time
// https://msdn.microsoft.com/en-us/library/ms724247(v=VS.85).aspx
// Windows Epoch start 1601
bela::Time FromDosDateTime(uint16_t dosDate, uint16_t dosTime) {
  auto year = (static_cast<int>(dosDate) >> 9) + 1980;
  auto mon = static_cast<int>((dosDate >> 5) & 0xf);
  auto day = static_cast<int>(dosDate & 0x1f);
  auto hour = static_cast<int>(dosTime) >> 11;
  auto minute = static_cast<int>((dosTime >> 5) & 0x3f);
  auto sec = static_cast<int>(dosTime & 0x1f) << 1;
  if (sec < 0 || sec > 59 || minute > 59 || minute < 0 || hour < 0 || hour > 23 || mon < 1 || mon > 12 || year < 1601) {
    return bela::UnixEpoch();
  }
  auto leapYear = IsLeapYear(year);
  auto mondays = leapYear ? time_internal::LeapMonths[mon - 1] : time_internal::Months[mon - 1];
  if (day < 1 || day > mondays) {
    return bela::UnixEpoch();
  }
  auto days = sinceWindowsEpochDays(year);
  for (auto i = 1; i < mon; i++) {
    days += time_internal::Months[i - 1];
  }
  if (mon > 2 && leapYear) {
    days++;
  }
  days += day - 1;
  return bela::FromUnixSeconds(days * secondsPerDay + hour * 3600 + minute * 60 + sec - UnixEpochStart);
}
} // namespace bela