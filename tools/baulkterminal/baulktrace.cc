//
#include <bela/terminal.hpp>
#include <bela/narrow/strcat.hpp>
#include <bela/path.hpp>
#include <time.hpp>
#include "baulkterminal.hpp"

namespace baulkterminal {
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
    auto um =
        bela::narrow::StringCat(baulk::time::TimeNow(), " [", GetCurrentProcessId(), "] ", bela::ToNarrow(msg), "\n");
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
    auto file = bela::StringCat(*p, L"\\baulkterminal.trace.log");
    if (fd = CreateFileW(file.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
        fd == INVALID_HANDLE_VALUE) {
      return false;
    }
    return true;
  }
  HANDLE fd{INVALID_HANDLE_VALUE};
};

int WriteTraceInternal(std::wstring_view msg) {
  //
  return TraceHelper::Instance().WriteMessage(msg);
}

int WriteTrace(std::wstring_view msg) {
  if (!baulkterminal::IsDebugMode) {
    return 0;
  }
  return WriteTraceInternal(msg);
}

} // namespace baulkterminal