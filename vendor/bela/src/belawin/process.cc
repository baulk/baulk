//
#include <bela/process.hpp>
#include <bela/terminal.hpp>

namespace bela::process {
// run a new process and wait
int Process::ExecuteInternal(std::wstring_view file, wchar_t *cmdline) {
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  std::wstring env;
  std::wstring path;
  if (simulator != nullptr) {
    env = const_cast<bela::env::Simulator *>(simulator)->MakeEnv();
    if (file.find_first_of(L":\\/") == std::wstring_view::npos) {
      simulator->LookPath(file, path, true);
    }
  }
  if (CreateProcessW(string_nullable(path), cmdline, nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT,
                     reinterpret_cast<LPVOID>(string_nullable(env)), string_nullable(cwd), &si, &pi) != TRUE) {
    ec = bela::make_system_error_code(bela::StringCat(L"run command '", cmdline, L"': "));
    return -1;
  }
  CloseHandle(pi.hThread);
  SetConsoleCtrlHandler(nullptr, TRUE);
  auto closer = bela::finally([&] {
    SetConsoleCtrlHandler(nullptr, FALSE);
    CloseHandle(pi.hProcess);
  });
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  return exitCode;
}

void Free(HANDLE fd) {
  if (fd != nullptr) {
    CloseHandle(fd);
  }
}

// thanks:
// https://github.com/microsoft/vcpkg-tool/blob/main/src/vcpkg/base/system.process.cpp
// https://devblogs.microsoft.com/oldnewthing/20111216-00/?p=8873
// https://github.com/rust-lang/rust/issues/73281
struct process_capture_helper {
  process_capture_helper() : pi{} {}
  PROCESS_INFORMATION pi;
  HANDLE fdout{nullptr};
  bool create_process_redirect(std::wstring &path, wchar_t *cmdline, std::wstring &env, std::wstring &cwd, DWORD flags,
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
    // Create a pipe for the child process's STDOUT.
    if (CreatePipe(&fdout, &si.hStdOutput, &saAttr, 0) != TRUE) {
      ec = bela::make_system_error_code(bela::StringCat(L"run command '", cmdline, L"' CreatePipe: "));
      return false;
    }
    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (SetHandleInformation(fdout, HANDLE_FLAG_INHERIT, 0) != TRUE) {
      ec = bela::make_system_error_code();
      Free(fdout);
      Free(si.hStdOutput);
      return false;
    }
    if (FlagIsTrue(flags, CAPTURE_ERR)) {
      si.hStdError = si.hStdOutput;
    }
    if (FlagIsTrue(flags, CAPTURE_USEERR)) {
      si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }
    if (FlagIsTrue(flags, CAPTURE_USEIN)) {
      si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    }
    if (CreateProcessW(string_nullable(path), cmdline, nullptr, nullptr, TRUE, CREATE_UNICODE_ENVIRONMENT,
                       string_nullable(env), string_nullable(cwd), &si, &pi) != TRUE) {
      ec = bela::make_system_error_code(bela::StringCat(L"run command '", cmdline, L"': "));
      Free(fdout);
      Free(si.hStdOutput);
      return false;
    }
    Free(si.hStdOutput);
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
    while (ReadFile(fdout, (void *)buf.get(), buffer_size, &bytes_read, nullptr) == TRUE && bytes_read > 0) {
      out.append(buf.get(), static_cast<size_t>(bytes_read));
    }
    CloseHandle(fdout);
    return wait_and_close_handles();
  }
};

int Process::CaptureInternal(std::wstring_view file, wchar_t *cmdline, DWORD flags) {
  process_capture_helper helper;
  std::wstring env;
  std::wstring path;
  if (simulator != nullptr) {
    env = const_cast<bela::env::Simulator *>(simulator)->MakeEnv();
    if (file.find_first_of(L":\\/") == std::wstring_view::npos) {
      simulator->LookPath(file, path, true);
    }
  }
  if (!helper.create_process_redirect(path, cmdline, env, cwd, flags, ec)) {
    return 1;
  }
  return helper.wait_and_stream_output(out);
}

} // namespace bela::process
