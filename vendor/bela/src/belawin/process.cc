//
#include <bela/process.hpp>
#include <bela/finaly.hpp>

namespace bela::process {
int Process::ExecuteInternal(wchar_t *cmdline) {
  // run a new process and wait
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  if (env.empty() && de.Size() != 0) {
    env = de.MakeEnv();
  }
  if (CreateProcessW(nullptr, cmdline, nullptr, nullptr, FALSE,
                     CREATE_UNICODE_ENVIRONMENT,
                     reinterpret_cast<LPVOID>(string_nullable(env)),
                     string_nullable(cwd), &si, &pi) != TRUE) {
    ec = bela::make_system_error_code();
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
// thanks:
// https://github.com/microsoft/vcpkg/blob/master/toolsrc/src/vcpkg/base/system.process.cpp
struct process_capture_helper {
  process_capture_helper() : pi{} {}
  PROCESS_INFORMATION pi;
  HANDLE child_stdin = nullptr;
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
    // Create a pipe for the child process's STDIN.
    if (CreatePipe(&si.hStdInput, &child_stdin, &saAttr, 0) != TRUE) {
      ec = bela::make_system_error_code();
      CloseHandle(child_stdout);
      CloseHandle(si.hStdOutput);
      return false;
    }
    // Ensure the write handle to the pipe for STDIN is not inherited.
    if (SetHandleInformation(child_stdin, HANDLE_FLAG_INHERIT, 0) != TRUE) {
      ec = bela::make_system_error_code();
      CloseHandle(child_stdout);
      CloseHandle(si.hStdOutput);
      CloseHandle(child_stdin);
      CloseHandle(si.hStdInput);
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
      CloseHandle(child_stdin);
      CloseHandle(si.hStdInput);
      return false;
    }
    CloseHandle(si.hStdInput);
    CloseHandle(si.hStdOutput);
    return true;
  }
  void close_handles() {
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }
  unsigned int wait_and_close_handles() {
    CloseHandle(pi.hThread);
    const DWORD result = WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    return exit_code;
  }

  int wait_and_stream_output(std::string &out) {
    CloseHandle(child_stdin);
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

int Process::CaptureInternal(wchar_t *cmdline) {
  process_capture_helper helper;
  if (env.empty() && de.Size() != 0) {
    env = de.MakeEnv();
  }
  if (!helper.create_process_redirect(cmdline, env, cwd, ec)) {
    return 1;
  }
  return helper.wait_and_stream_output(out);
}

} // namespace bela::process