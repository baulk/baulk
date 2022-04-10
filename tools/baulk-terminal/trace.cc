//
#include <bela/terminal.hpp>
#include <bela/str_cat.hpp>
#include <bela/path.hpp>
#include <bela/datetime.hpp>
#include "baulk-terminal.hpp"

namespace baulk {
class TraceHelper {
public:
  TraceHelper(const TraceHelper &) = delete;
  TraceHelper &operator=(const TraceHelper &) = delete;
  ~TraceHelper() {
    if (fd != INVALID_HANDLE_VALUE) {
      CloseHandle(fd);
    }
  }
  static TraceHelper &Instance() {
    static TraceHelper th;
    return th;
  }
  int WriteMessage(std::wstring_view msg) {
    if (fd == INVALID_HANDLE_VALUE) {
      return -1;
    }
    auto um = bela::StringNarrowCat("[DEBUG] ", bela::FormatTime<char>(bela::Now()), " [", GetCurrentProcessId(),
                                      "] ", bela::encode_into<wchar_t, char>(msg), "\n");
    DWORD written = 0;
    if (WriteFile(fd, um.data(), static_cast<DWORD>(um.size()), &written, nullptr) != TRUE) {
      return -1;
    }
    return written;
  }

private:
  TraceHelper() { Initialize(); }
  bool Initialize() {
    bela::error_code ec;
    auto p = bela::ExecutableParent(ec);
    if (!p) {
      return false;
    }
    auto file = bela::StringCat(*p, L"\\baulk-terminal.trace.log");
    if (fd = CreateFileW(file.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
        fd == INVALID_HANDLE_VALUE) {
      return false;
    }
    LARGE_INTEGER li{0};
    ::SetFilePointerEx(fd, li, nullptr, FILE_END);
    return true;
  }
  HANDLE fd{INVALID_HANDLE_VALUE};
};

int WriteTraceInternal(std::wstring_view msg) {
  //
  return TraceHelper::Instance().WriteMessage(msg);
}

int WriteTrace(std::wstring_view msg) {
  if (!IsDebugMode) {
    return 0;
  }
  return WriteTraceInternal(msg);
}

} // namespace baulk