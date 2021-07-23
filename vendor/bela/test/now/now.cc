//
#include <bela/time.hpp>
#include <bela/terminal.hpp>
#include <bela/base.hpp>
#include <bela/datetime.hpp>
#include <ctime>

inline std::string TimeString(time_t t) {
  if (t < 0) {
    t = 0;
  }
  std::tm tm_;
  localtime_s(&tm_, &t);

  std::string buffer;
  buffer.resize(64);
  auto n = std::strftime(buffer.data(), 64, "%Y-%m-%dT%H:%M:%S%Ez", &tm_);
  buffer.resize(n);
  return buffer;
}

inline std::wstring_view trimSuffixZero(std::wstring_view sv) {
  if (auto pos = sv.find_last_not_of('0'); pos != std::wstring_view::npos) {
    return sv.substr(0, pos + 1);
  }
  return sv;
}

int wmain() {

  constexpr const wchar_t *sv[] = {L"11110000", L"", L"0001110"};
  for (const auto s : sv) {
    bela::FPrintF(stderr, L"[%s]--[%s]\n", s, trimSuffixZero(s));
  }
  auto t0 = time(nullptr);
  bela::FPrintF(stderr, L"time:  %d\n", t0);
  auto now = bela::Now();
  FILETIME ft = {0};
  GetSystemTimePreciseAsFileTime(&ft);
  bela::FPrintF(stderr, L"now:  %d\n", bela::ToUnixSeconds(now));
  auto ftn = bela::ToFileTime(now);
  FILETIME lft;
  FileTimeToLocalFileTime(&ftn, &lft);
  SYSTEMTIME st;
  FileTimeToSystemTime(&lft, &st);
  bela::FPrintF(stderr, L"Now ToFileTime: %04d-%02d-%02d %02d:%02d:%02d\n", st.wYear, st.wMonth, st.wDay, st.wHour,
                st.wMinute, st.wSecond);
  auto tickNow = bela::FromWindowsPreciseTime(*reinterpret_cast<int64_t *>(&ft));
  bela::FPrintF(stderr, L"tick: %d\n", bela::ToUnixSeconds(tickNow));
  auto tickNow2 = bela::FromFileTime(ft);
  bela::FPrintF(stderr, L"FromFileTime time:  %s\n", bela::FormatTime(tickNow2));
  bela::FPrintF(stderr, L"FromFileTime time(Narrow):  %s\n", bela::FormatTimeNarrow(tickNow2));
  TIME_ZONE_INFORMATION tz_info;
  GetTimeZoneInformation(&tz_info);

  DYNAMIC_TIME_ZONE_INFORMATION dzt;
  GetDynamicTimeZoneInformation(&dzt);
  bela::FPrintF(stderr, L"TZ: +%d TimeZone: %s %s\n", -tz_info.Bias / 60, tz_info.StandardName, dzt.TimeZoneKeyName);
  auto dt = bela::LocalDateTime(now);
  bela::FPrintF(stderr, L"%04d-%02d-%02d %02d:%02d:%02d %s %s\n", dt.Year(), static_cast<int>(dt.Month()), dt.Day(),
                dt.Hour(), dt.Minute(), dt.Second(), bela::WeekdayName(dt.Weekday(), false),
                bela::MonthName(dt.Month()), false);
  bela::FPrintF(stderr, L"now: %d\n", bela::ToUnixSeconds(dt.Time()));
  bela::DateTime dut(now);
  bela::FPrintF(stderr, L"T: %s\nT: %s\n", dt.Format(), dt.Format(true));
  bela::FPrintF(stderr, L"T: %s\nT: %s\n", dut.Format(), dut.Format(true));
  bela::FPrintF(stderr, L"Weekday: %s\n", bela::WeekdayName(bela::DateTime(2020, 12, 6, 19, 57, 00).Weekday(), false));
  bela::FPrintF(stderr, L"Weekday 1970-01-01: %s\n",
                bela::WeekdayName(bela::DateTime(1970, 1, 1, 19, 57, 00).Weekday(), false));
  bela::FPrintF(stderr, L"Weekday 1969-12-31: %s\n",
                bela::WeekdayName(bela::DateTime(1969, 12, 31, 19, 57, 00).Weekday(), false));
  bela::FPrintF(stderr, L"Weekday 2000-03-01: %s\n",
                bela::WeekdayName(bela::DateTime(2000, 03, 01, 19, 57, 00).Weekday(), false));
  bela::FPrintF(stderr, L"Weekday 1601-01-01: %s\n",
                bela::WeekdayName(bela::DateTime(1601, 01, 01, 19, 57, 00).Weekday(), false));
  constexpr auto imax = (std::numeric_limits<int>::max)();
  auto t = bela::FromUnixSeconds(imax);
  bela::FPrintF(stderr, L"int32 time: %d %s\n", imax, bela::FormatUniversalTime(t));
  return 0;
}