//
#include <cstdio>
#include <io.h>
#include <bela/str_cat.hpp>
#include <bela/match.hpp>
#include <bela/str_split.hpp>
#include <bela/terminal.hpp>

namespace bela::terminal {
// Is console terminal
bool IsTerminal(HANDLE fd) {
  DWORD mode = 0;
  if (GetConsoleMode(fd, &mode) != TRUE) {
    return false;
  }
  return mode != 0;
}

bool TerminalEnabled(HANDLE fd) {
  DWORD dwMode = 0;
  if (GetConsoleMode(fd, &dwMode) != TRUE) {
    return false;
  }
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  return SetConsoleMode(fd, dwMode) == TRUE;
}

bool IsTerminal(FILE *fd) { return IsTerminal(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fd)))); }

// Is cygwin terminal
bool IsCygwinTerminal(HANDLE fd) {
  if (GetFileType(fd) != FILE_TYPE_PIPE) {
    return false;
  }
  constexpr unsigned int pipemaxlen = 512;
  WCHAR buffer[pipemaxlen] = {0};
  if (GetFileInformationByHandleEx(fd, FileNameInfo, buffer, pipemaxlen * 2) != TRUE) {
    return false;
  }
  auto pb = reinterpret_cast<FILE_NAME_INFO *>(buffer);
  std::wstring_view pipename{pb->FileName, pb->FileNameLength / 2};
  std::vector<std::wstring_view> pvv = bela::StrSplit(pipename, bela::ByChar(L'-'), bela::SkipEmpty());
  if (pvv.size() < 5) {
    return false;
  }
  constexpr std::wstring_view cyglikeprefix[] = {L"\\msys", L"\\cygwin", L"\\Device\\NamedPipe\\msys",
                                                 L"\\Device\\NamedPipe\\cygwin"};
  constexpr std::wstring_view ptyprefix = L"pty";
  constexpr std::wstring_view pipeto = L"to";
  constexpr std::wstring_view pipefrom = L"from";
  constexpr std::wstring_view master = L"master";
  if (std::find(std::begin(cyglikeprefix), std::end(cyglikeprefix), pvv[0]) == std::end(cyglikeprefix)) {
    return false;
  }
  if (pvv[1].empty()) {
    return false;
  }
  if (!bela::StartsWith(pvv[2], ptyprefix)) {
    return false;
  }
  if (pvv[3] != pipeto && pvv[3] != pipefrom) {
    return false;
  }
  if (pvv[4] != master) {
    return false;
  }
  return true;
}
bool IsCygwinTerminal(FILE *fd) { return IsCygwinTerminal(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fd)))); }
// Is same terminal maybe console or Cygwin pty
bool IsSameTerminal(HANDLE fd) { return IsTerminal(fd) || IsCygwinTerminal(fd); }
bool IsSameTerminal(FILE *fd) {
  auto FileHandle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fd)));
  return IsTerminal(FileHandle) || IsCygwinTerminal(FileHandle);
}

bool TerminalSize(HANDLE fd, terminal_size &sz) {
  CONSOLE_SCREEN_BUFFER_INFO bi;
  if (GetConsoleScreenBufferInfo(fd, &bi) != TRUE) {
    return false;
  }
  sz.columns = bi.dwSize.X;
  sz.rows = bi.dwSize.Y;
  return true;
}
bool TerminalSize(FILE *fd, terminal_size &sz) {
  return TerminalSize(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fd))), sz);
}

// Write data to windows terminal (windows console)
bela::ssize_t WriteTerminal(HANDLE fd, std::wstring_view data) {
  DWORD dwWrite = 0;
  if (WriteConsoleW(fd, data.data(), (DWORD)data.size(), &dwWrite, nullptr) != TRUE) {
    return -1;
  }
  return static_cast<ssize_t>(dwWrite);
}
// Write data to same file not windows terminal
bela::ssize_t WriteSameFile(HANDLE fd, std::wstring_view data) {
  auto ndata = bela::encode_into<wchar_t, char>(data);
  DWORD dwWrite = 0;
  if (::WriteFile(fd, ndata.data(), (DWORD)ndata.size(), &dwWrite, nullptr) != TRUE) {
    return -1;
  }
  return static_cast<ssize_t>(dwWrite);
}

// Write data to same file not windows terminal
bela::ssize_t WriteSameFile(HANDLE fd, std::string_view data) {
  DWORD dwWrite = 0;
  if (::WriteFile(fd, data.data(), (DWORD)data.size(), &dwWrite, nullptr) != TRUE) {
    return -1;
  }
  return static_cast<ssize_t>(dwWrite);
}

enum class TerminalMode {
  File, //
  ConPTY,
  Cygwin
};

inline bela::ssize_t WriteAutoInternal(HANDLE fd, TerminalMode mode, std::wstring_view data) {
  if (fd == nullptr) {
    return -1;
  }
  if (mode == TerminalMode::ConPTY) {
    return WriteTerminal(fd, data);
  }
  return WriteSameFile(fd, data);
}

inline bela::ssize_t WriteAutoInternal(HANDLE fd, TerminalMode mode, std::string_view data) {
  if (fd == nullptr) {
    return -1;
  }
  if (mode == TerminalMode::ConPTY) {
    return WriteTerminal(fd, bela::encode_into<char, wchar_t>(data));
  }
  return WriteSameFile(fd, data);
}

class Filter {
public:
  Filter(const Filter &) = delete;
  Filter &operator=(const Filter &) = delete;
  static Filter &Instance() {
    static Filter filter;
    return filter;
  }
  bela::ssize_t WriteAuto(FILE *fd, std::wstring_view data) {
    if (fd == stderr) {
      return WriteAutoInternal(fderr, errmode, data);
    }
    if (fd == stdout) {
      return WriteAutoInternal(fdout, outmode, data);
    }
    auto FileHandle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fd)));
    return WriteSameFile(FileHandle, data);
  }
  bela::ssize_t WriteAuto(FILE *fd, std::string_view data) {
    if (fd == stderr) {
      return WriteAutoInternal(fderr, errmode, data);
    }
    if (fd == stdout) {
      return WriteAutoInternal(fdout, outmode, data);
    }
    auto FileHandle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fd)));
    return WriteSameFile(FileHandle, data);
  }

private:
  Filter() {
    fdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (fdout != nullptr) {
      if (TerminalEnabled(fdout)) {
        outmode = TerminalMode::ConPTY;
      } else if (IsCygwinTerminal(fdout)) {
        outmode = TerminalMode::Cygwin;
      }
    }
    fderr = GetStdHandle(STD_ERROR_HANDLE);
    if (fderr != nullptr) {
      if (TerminalEnabled(fderr)) {
        errmode = TerminalMode::ConPTY;
      } else if (IsCygwinTerminal(fderr)) {
        errmode = TerminalMode::Cygwin;
      }
    }
  }
  HANDLE fdout{nullptr};
  HANDLE fderr{nullptr};
  TerminalMode outmode{TerminalMode::File};
  TerminalMode errmode{TerminalMode::File};
};

bela::ssize_t WriteAuto(FILE *fd, std::wstring_view data) { return Filter::Instance().WriteAuto(fd, data); }
bela::ssize_t WriteAuto(FILE *fd, std::string_view data) { return Filter::Instance().WriteAuto(fd, data); }

bela::ssize_t WriteAutoFallback(FILE *fd, std::wstring_view data) {
  auto FileHandle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fd)));
  if (IsTerminal(FileHandle)) {
    return WriteTerminal(FileHandle, data);
  }
  return WriteSameFile(FileHandle, data);
}

bela::ssize_t WriteAutoFallback(FILE *fd, std::string_view data) {
  auto FileHandle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fd)));
  if (IsTerminal(FileHandle)) {
    return WriteTerminal(FileHandle, bela::encode_into<char, wchar_t>(data));
  }
  return WriteSameFile(FileHandle, data);
}

} // namespace bela::terminal