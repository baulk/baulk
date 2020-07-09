///
#include <bela/base.hpp>
#include <bela/escapeargv.hpp>
#include <bela/pe.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/str_split.hpp>
#include <bela/env.hpp>
#include <filesystem>
#include <json.hpp>
#include <cstdio>
#include <cerrno>

namespace fs = std::filesystem;
bool IsDebugMode = false;
// DbgPrint added newline
template <typename... Args> bela::ssize_t DbgPrint(const wchar_t *fmt, Args... args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  std::wstring str;
  str.append(L"\x1b[33m* ");
  bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
  if (str.back() == '\n') {
    str.pop_back();
  }
  str.append(L"\x1b[0m\n");
  return bela::terminal::WriteAuto(stderr, str);
}
inline bela::ssize_t DbgPrint(const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  std::wstring_view msg(fmt);
  if (!msg.empty() && msg.back() == '\n') {
    msg.remove_suffix(1);
  }
  return bela::terminal::WriteAuto(stderr, bela::StringCat(L"\x1b[33m* ", msg, L"\x1b[0m\n"));
}

bool IsSubsytemConsole(std::wstring_view exe) {
  bela::error_code ec;
  auto realexe = bela::RealPathEx(exe, ec);
  if (!realexe) {
    return false;
  }
  auto pe = bela::pe::Expose(*realexe, ec);
  if (!pe) {
    return true;
  }
  return pe->subsystem == bela::pe::Subsystem::CUI;
}

std::wstring_view DirName(std::wstring_view s) {
  auto i = s.size() - 1;
  auto p = s.data();
  for (; bela::IsPathSeparator(p[i]); i--) {
    if (i == 0) {
      return L"";
    }
  }
  for (; !bela::IsPathSeparator(p[i]); i--) {
    if (i == 0) {
      return L"";
    }
  }
  for (; bela::IsPathSeparator(p[i]); i--) {
    if (i == 0) {
      return L"";
    }
  }
  return std::wstring_view{s.data(), i + 1};
}

std::optional<std::wstring> ResolveTarget(std::wstring_view arg0, bela::error_code &ec) {
  // avoid commandline forged
  constexpr std::wstring_view baulklinkmeta = L"\\baulk.linkmeta.json";
  auto exe = bela::Executable(ec); // GetModuleFileName
  if (!exe) {
    return std::nullopt;
  }
  auto launcher = bela::BaseName(*exe);
  auto parent = DirName(*exe);
  auto linkmeta = bela::StringCat(parent, baulklinkmeta);
  try {
    /* code */
    FILE *fd{nullptr};
    if (auto eno = _wfopen_s(&fd, linkmeta.data(), L"rb"); eno != 0) {
      ec = bela::make_stdc_error_code(eno);
      return std::nullopt;
    }
    auto closer = bela::finally([&] { fclose(fd); });
    auto j0 = nlohmann::json::parse(fd);
    auto links = j0.at("links");
    auto metadata = bela::ToWide(links.at(bela::ToNarrow(launcher)).get<std::string>());
    std::vector<std::wstring_view> tv =
        bela::StrSplit(metadata, bela::ByChar('@'), bela::SkipEmpty());
    if (tv.size() < 2) {
      ec = bela::make_error_code(1, L"baulk launcher: '", launcher, L"' invaild metadata: ",
                                 metadata);
      return std::nullopt;
    }
    auto baulkroot = DirName(parent);
    auto target = bela::StringCat(baulkroot, L"\\pkgs\\", tv[0], L"\\", tv[1]);
    return std::make_optional(std::move(target));
  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, L"baulk.links.json exception: ", bela::ToWide(e.what()));
  }
  return std::nullopt;
}

inline bool IsTrue(std::wstring_view b) {
  return bela::EqualsIgnoreCase(b, L"true") || bela::EqualsIgnoreCase(b, L"yes") || b == L"1";
}

int wmain(int argc, wchar_t **argv) {
  IsDebugMode = IsTrue(bela::GetEnv(L"BAULK_DEBUG"));
  bela::error_code ec;
  auto target = ResolveTarget(argv[0], ec);
  if (!target) {
    bela::FPrintF(stderr, L"unable detect launcher target: %s\n", ec.message);
    return 1;
  }
  DbgPrint(L"resolve target: %s\n", *target);
  auto isconsole = IsSubsytemConsole(*target);
  std::wstring newcmd(GetCommandLineW());
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  if (CreateProcessW(target->data(), newcmd.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT,
                     nullptr, nullptr, &si, &pi) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"unable detect launcher target: %s\n", ec.message);
    return -1;
  }
  CloseHandle(pi.hThread);
  SetConsoleCtrlHandler(nullptr, TRUE);
  auto closer = bela::finally([&] {
    SetConsoleCtrlHandler(nullptr, FALSE);
    CloseHandle(pi.hProcess);
  });
  if (!isconsole) {
    return 0;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  return 0;
}
