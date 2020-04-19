/// decompress zip file
#include <bela/codecvt.hpp> //UTF8 to UTF16
#include "zip.hpp"
#include "bzip2/bzlib.h" // bzip2 support
// zip support
#if defined(_M_X64) || defined(_M_X86)
#include "zlib-intel/zlib.h"
#else
#include "zlib/zlib.h"
#endif

namespace baulk::archive::zip {

[[maybe_unused]] constexpr int ValidZipDate_YearMin = 1980;
[[maybe_unused]] constexpr int ValidZipDate_YearMax = 2107;

enum ZipVersionNeededValues : uint16_t {
  Default = 10,
  ExplicitDirectory = 20,
  Deflate = 20,
  Deflate64 = 21,
  Zip64 = 45
};

enum ZipVersionMadeByPlatform : uint8_t { Windows = 0, Unix = 3 };

// https://github.com/dotnet/runtime/blob/cf66f084ca8b563c42cd814e8ce8b03519af51bd/src/libraries/System.IO.Compression/src/System/IO/Compression/ZipHelper.cs#L57
// BOOL SetFileTime(
//   HANDLE         hFile,
//   const FILETIME *lpCreationTime,
//   const FILETIME *lpLastAccessTime,
//   const FILETIME *lpLastWriteTime
// );
// BOOL SystemTimeToFileTime(
//   const SYSTEMTIME *lpSystemTime,
//   LPFILETIME       lpFileTime
// );
std::time_t DosTimeToUnixTime(uint32_t dateTime) {
  // DosTime format 32 bits
  // Year: 7 bits, 0 is ValidZipDate_YearMin, unsigned (ValidZipDate_YearMin =
  // 1980) Month: 4 bits Day: 5 bits Hour: 5 Minute: 6 bits Second: 5 bits

  // do the bit shift as unsigned because the fields are unsigned, but
  // we can safely convert to int, because they won't be too big
  int year = (int)(ValidZipDate_YearMin + (dateTime >> 25));
  int month = (int)((dateTime >> 21) & 0xF);
  int day = (int)((dateTime >> 16) & 0x1F);
  int hour = (int)((dateTime >> 11) & 0x1F);
  int minute = (int)((dateTime >> 5) & 0x3F);
  int second =
      (int)((dateTime & 0x001F) * 2); // only 5 bits for second, so we only have
                                      // a granularity of 2 sec.
  return 0;
}

/*
BOOL
WINAPI
DosDateTimeToFileTime(IN WORD wFatDate,
                      IN WORD wFatTime,
                      OUT LPFILETIME lpFileTime)
{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER SystemTime;

    TimeFields.Year = (wFatDate >> 9) + 1980;
    TimeFields.Month = (wFatDate >> 5) & 0xF;
    TimeFields.Day = (wFatDate & 0x1F);
    TimeFields.Hour = (wFatTime >> 11);
    TimeFields.Minute = (wFatTime >> 5) & 0x3F;
    TimeFields.Second = (wFatTime & 0x1F) << 1;
    TimeFields.Milliseconds = 0;

    if (RtlTimeFieldsToTime(&TimeFields, &SystemTime))
    {
        lpFileTime->dwLowDateTime = SystemTime.LowPart;
        lpFileTime->dwHighDateTime = SystemTime.HighPart;
        return TRUE;
    }

    BaseSetLastNTError(STATUS_INVALID_PARAMETER);
    return FALSE;
}
*/

class ZipArchiveEntry {
public:
  ZipArchiveEntry() = default;
  ~ZipArchiveEntry() {
    if (FileHandle != INVALID_HANDLE_VALUE) {
      CloseHandle(FileHandle);
      FileHandle = INVALID_HANDLE_VALUE;
    }
  }
  bool UpdateTime(uint32_t dostime) {
    FILETIME ftm, ftLocal, ftCreate, ftLastAcc, ftLastWrite;
    GetFileTime(FileHandle, &ftCreate, &ftLastAcc, &ftLastWrite);
    DosDateTimeToFileTime((WORD)(dostime >> 16), (WORD)dostime, &ftLocal);
    LocalFileTimeToFileTime(&ftLocal, &ftm);
    SetFileTime(FileHandle, &ftm, &ftLastAcc, &ftm);
    return true;
  }

private:
  HANDLE FileHandle{INVALID_HANDLE_VALUE};
};

ZipReader::~ZipReader() {
  if (FileHandle != INVALID_HANDLE_VALUE) {
    CloseHandle(FileHandle);
    FileHandle = INVALID_HANDLE_VALUE;
  }
}
bool ZipReader::Initialize(std::wstring_view zipfile, std::wstring_view dest,
                           bela::error_code &ec) {
  if (FileHandle =
          CreateFileW(zipfile.data(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
      FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  LARGE_INTEGER li;
  if (GetFileSizeEx(FileHandle, &li) != TRUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  filesize = li.QuadPart;
  return true;
}

// http://www.pkware.com/documents/casestudies/APPNOTE.TXT

} // namespace baulk::archive::zip