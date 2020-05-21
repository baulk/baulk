#include <bela/base.hpp>
#include <bela/stdwriter.hpp>
#include <bela/str_split.hpp>
#include <bela/ascii.hpp>
#include <bela/escapeargv.hpp>
#include "indicators.hpp"

namespace baulk {

constexpr const wchar_t *string_nullable(std::wstring_view str) {
  return str.empty() ? nullptr : str.data();
}
constexpr wchar_t *string_nullable(std::wstring &str) {
  return str.empty() ? nullptr : str.data();
}

struct process_capture_helper {
  process_capture_helper() : pi{} {}
  PROCESS_INFORMATION pi;
  HANDLE child_stdout = nullptr;
  bool create_process_redirect(wchar_t *cmdline, std::wstring &env,
                               std::wstring &cwd,
                               bela::error_code &ec) noexcept {
    STARTUPINFOW si;
    memset(&si, 0, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);
    si.dwFlags |= STARTF_USESTDHANDLES;

    SECURITY_ATTRIBUTES saAttr;
    memset(&saAttr, 0, sizeof(SECURITY_ATTRIBUTES));
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);// must bind
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    // Create a pipe for the child process's STDOUT.
    if (CreatePipe(&child_stdout, &si.hStdOutput, &saAttr, 0) != TRUE) {
      ec = bela::make_system_error_code();
      return false;
    }
    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (SetHandleInformation(child_stdout, HANDLE_FLAG_INHERIT, 0) != TRUE) {
      ec = bela::make_system_error_code();
      CloseHandle(child_stdout);
      CloseHandle(si.hStdOutput);
      return false;
    }
    si.hStdError = si.hStdOutput;
    if (CreateProcessW(
            nullptr, cmdline, nullptr, nullptr, TRUE,
            IDLE_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT | CREATE_NO_WINDOW,
            string_nullable(env), string_nullable(cwd), &si, &pi) != TRUE) {
      ec = bela::make_system_error_code();
      CloseHandle(child_stdout);
      CloseHandle(si.hStdOutput);
      return false;
    }
    CloseHandle(si.hStdOutput);
    return true;
  }
  void close_handles() {
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }
  unsigned int wait_and_close_handles() {
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    return exit_code;
  }

  int wait_and_stream_output(std::string &out) {
    unsigned long bytes_read = 0;
    static constexpr int buffer_size = 1024 * 32;
    auto buf = std::make_unique<char[]>(buffer_size);
    while (ReadFile(child_stdout, (void *)buf.get(), buffer_size, &bytes_read,
                    nullptr) == TRUE &&
           bytes_read > 0) {
      out.append(buf.get(), static_cast<size_t>(bytes_read));
    }
    CloseHandle(child_stdout);
    return wait_and_close_handles();
  }
};
struct ProcessHelper {
  int CaptureInternal(wchar_t *cmdline) {
    process_capture_helper helper;
    if (!helper.create_process_redirect(cmdline, env, cwd, ec)) {
      return 1;
    }
    return helper.wait_and_stream_output(out);
  }

  template <typename... Args> int Capture(std::wstring_view cmd, Args... args) {
    bela::EscapeArgv ea(cmd, args...);
    return CaptureInternal(ea.data());
  }
  std::wstring env;
  std::wstring cwd;
  std::string out;
  bela::error_code ec;
};

bool ProgressBar::MakeColumns() {
  ProcessHelper ps;
  if (auto exitcode = ps.Capture(L"stty", L"size"); exitcode != 0) {
    bela::FPrintF(stderr, L"stty %d: %s\n", exitcode, ps.ec.message);
    return false;
  }
  auto out = bela::ToWide(ps.out);
  std::vector<std::wstring_view> ss =
      bela::StrSplit(bela::StripTrailingAsciiWhitespace(out), bela::ByChar(' '),
                     bela::SkipEmpty());
  if (ss.size() != 2) {
    return false;
  }
  if (!bela::SimpleAtoi(ss[1], &columns)) {
    return false;
  }
  if (columns < 80) {
    columns = 80;
  }
  return true;
}

} // namespace baulk