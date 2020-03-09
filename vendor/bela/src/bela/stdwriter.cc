///////////
#include <bela/base.hpp>
#include <bela/stdwriter.hpp>
#include <bela/codecvt.hpp>
#include <bela/strcat.hpp>
#include <bela/match.hpp>
#include <bela/str_split.hpp>
#include <io.h>

namespace bela {
ssize_t BelaWriteAnsi(HANDLE hDev, std::wstring_view msg);
enum class ConsoleMode {
  File, //
  Conhost,
  ConPTY,
  PTY
};

/*
Mintty is not a full replacement for Cygwin Console (i.e. cygwin running
in a Windows console window).
Like xterm and rxvt, mintty communicates with the child process through a
pseudo terminal device, which Cygwin emulates using Windows pipes.
This means that native Windows command line programs started in mintty see
a pipe rather than a console device.
As a consequence, such programs often disable interactive input. Also,
direct calls to low-level Win32 console functions will fail.
Programs that access the console as a file should be fine though.
*/
bool IsCygwinPipe(HANDLE hFile) {
  constexpr unsigned int pipemaxlen = 512;
  WCHAR buffer[pipemaxlen] = {0};
  if (GetFileInformationByHandleEx(hFile, FileNameInfo, buffer,
                                   pipemaxlen * 2) != TRUE) {
    return false;
  }
  auto pb = reinterpret_cast<FILE_NAME_INFO *>(buffer);
  std::wstring_view pipename{pb->FileName, pb->FileNameLength / 2};
  std::vector<std::wstring_view> pvv =
      bela::StrSplit(pipename, bela::ByChar(L'-'), bela::SkipEmpty());
  if (pvv.size() < 5) {
    return false;
  }
  constexpr std::wstring_view cyglikeprefix[] = {
      L"\\msys", L"\\cygwin", L"\\Device\\NamedPipe\\msys",
      L"\\Device\\NamedPipe\\cygwin"};
  constexpr std::wstring_view ptyprefix = L"pty";
  constexpr std::wstring_view pipeto = L"to";
  constexpr std::wstring_view pipefrom = L"from";
  constexpr std::wstring_view master = L"master";
  if (std::find(std::begin(cyglikeprefix), std::end(cyglikeprefix), pvv[0]) ==
      std::end(cyglikeprefix)) {
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

inline bool EnableVirtualTerminal(HANDLE hFile) {
  DWORD dwMode = 0;
  if (!GetConsoleMode(hFile, &dwMode)) {
    return false;
  }
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  if (!SetConsoleMode(hFile, dwMode)) {
    return false;
  }
  return true;
}

ConsoleMode GetConsoleModeEx(HANDLE hFile) {
  if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
    return ConsoleMode::File;
  }
  auto ft = GetFileType(hFile);
  if (ft == FILE_TYPE_DISK) {
    return ConsoleMode::File;
  }
  if (ft != FILE_TYPE_CHAR) {
    if (IsCygwinPipe(hFile)) {
      ConsoleMode::PTY; // cygwin is pipe
    }
    return ConsoleMode::File; // cygwin is pipe
  }
  if (EnableVirtualTerminal(hFile)) {
    return ConsoleMode::ConPTY;
  }
  return ConsoleMode::Conhost;
}

static inline ssize_t WriteToTTY(FILE *out, std::wstring_view sv) {
  auto s = bela::ToNarrow(sv);
  return static_cast<ssize_t>(fwrite(s.data(), 1, s.size(), out));
}

static inline ssize_t WriteToConsole(HANDLE hConsole, std::wstring_view sv) {
  DWORD dwWrite = 0;
  if (!WriteConsoleW(hConsole, sv.data(), (DWORD)sv.size(), &dwWrite,
                     nullptr)) {
    return -1;
  }
  return static_cast<ssize_t>(dwWrite);
}

static inline bool IsToConsole(ConsoleMode cm) {
  return (cm == ConsoleMode::Conhost || cm == ConsoleMode::ConPTY);
}

class Adapter {
public:
  Adapter(const Adapter &) = delete;
  Adapter &operator=(const Adapter &) = delete;
  static Adapter &instance() {
    static Adapter a;
    return a;
  }
  std::wstring FileTypeName(FILE *file) const;
  bool IsTerminal(FILE *file) const;
  ssize_t FileWriteConsole(HANDLE hFile, std::wstring_view sv,
                          ConsoleMode cm) const {
    if (cm == ConsoleMode::Conhost) {
      return BelaWriteAnsi(hFile, sv);
    }
    return WriteToConsole(hFile, sv);
  }
  ssize_t FileWrite(FILE *out, std::wstring_view sv) const {
    if (out == stderr && IsToConsole(em)) {
      return FileWriteConsole(hStderr, sv, em);
    }
    if (out == stdout && IsToConsole(om)) {
      return FileWriteConsole(hStdout, sv, om);
    }
    return WriteToTTY(out, sv);
  }
  HANDLE Stdout() const { return hStdout; }
  HANDLE Stderr() const { return hStderr; }

private:
  ConsoleMode om;
  ConsoleMode em;
  HANDLE hStdout;
  HANDLE hStderr;
  void Initialize();
  Adapter() { Initialize(); }
};

uint32_t TerminalWidth() {
  switch (GetConsoleModeEx(Adapter::instance().Stderr())) {
  case ConsoleMode::Conhost:
    [[fallthrough]];
  case ConsoleMode::ConPTY: {
    CONSOLE_SCREEN_BUFFER_INFO bi;
    if (GetConsoleScreenBufferInfo(Adapter::instance().Stderr(), &bi) != TRUE) {
      return 80;
    }
    return bi.dwSize.X;
  }
  case ConsoleMode::File:
    return 80;
  default:
    break;
  }
  // FIXME: cannot detect mintty terminal width (not cygwin process)
  // https://wiki.archlinux.org/index.php/working_with_the_serial_console#Resizing_a_terminal
  // write escape string detect col
  // result like ';120R'
  return 80;
}

bool Adapter::IsTerminal(FILE *file) const {
  if (file == stdout) {
    return om != ConsoleMode::File;
  }
  if (file == stderr) {
    return om != ConsoleMode::File;
  }
  auto hFile = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file)));
  return GetConsoleModeEx(hFile) != ConsoleMode::File;
}

void Adapter::Initialize() {
  /*
  GetStdHandle: https://docs.microsoft.com/en-us/windows/console/getstdhandle
  If the function succeeds, the return value is a handle to the specified
device, or a redirected handle set by a previous call to SetStdHandle. The
handle has GENERIC_READ and GENERIC_WRITE access rights, unless the application
has used SetStdHandle to set a standard handle with lesser access.

If the function fails, the return value is INVALID_HANDLE_VALUE. To get extended
error information, call GetLastError.

If an application does not have associated standard handles, such as a service
running on an interactive desktop, and has not redirected them, the return value
is NULL
  */
  hStderr = GetStdHandle(STD_ERROR_HANDLE);
  em = GetConsoleModeEx(hStderr);
  hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
  om = GetConsoleModeEx(hStdout);
}

std::wstring FileTypeModeName(HANDLE hFile) {
  if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE) {
    return L"UNKOWN";
  }
  switch (GetFileType(hFile)) {
  default:
    return L"UNKOWN";
  case FILE_TYPE_DISK:
    return L"Disk File";
  case FILE_TYPE_PIPE:
    if (IsCygwinPipe(hFile)) {
      return L"Cygwin like PTY";
    }
    return L"Pipe";
  case FILE_TYPE_CHAR:
    break;
  }
  DWORD dwMode = 0;
  if (GetConsoleMode(hFile, &dwMode) == TRUE &&
      (dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0) {
    return L"VT Mode Console";
  }
  return L"Legacy Console";
}

std::wstring FileTypeModeName(FILE *file) {
  auto hFile = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file)));
  return FileTypeModeName(hFile);
}

std::wstring Adapter::FileTypeName(FILE *file) const {
  if (file == stderr) {
    return FileTypeModeName(hStderr);
  }
  if (file == stdout) {
    return FileTypeModeName(hStdout);
  }
  return FileTypeModeName(file);
}

ssize_t FileWrite(FILE *out, std::wstring_view msg) {
  return Adapter::instance().FileWrite(out, msg);
}

std::wstring FileTypeName(FILE *file) {
  return Adapter::instance().FileTypeName(file);
}

bool IsTerminal(FILE *file) {
  // fd IsTerminal
  return Adapter::instance().IsTerminal(file);
}

} // namespace bela
