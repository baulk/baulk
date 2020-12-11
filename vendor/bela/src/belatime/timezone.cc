///
#include <atomic>
#include <mutex>
#include <bela/datetime.hpp>

namespace bela {
// SOFTWARE\Microsoft\Windows NT\CurrentVersion\Time Zones
// https://docs.microsoft.com/en-us/windows/win32/api/timezoneapi/nf-timezoneapi-gettimezoneinformation

static TIME_ZONE_INFORMATION tz_info;
static std::once_flag tz_flag;

std::int_least32_t TimeZoneOffset() {
  std::call_once(tz_flag, [] { GetTimeZoneInformation(&tz_info); });
  return tz_info.Bias * 60;
}

const wchar_t *TimeZoneStandardName() {
  std::call_once(tz_flag, [] { GetTimeZoneInformation(&tz_info); });
  return tz_info.StandardName;
}

} // namespace bela