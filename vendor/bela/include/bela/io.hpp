// Bela IO utils
#ifndef BELA_IO_HPP
#define BELA_IO_HPP
#include "base.hpp"
#include "types.hpp"
#include "buffer.hpp"
#include "path.hpp"

namespace bela {
namespace io {
struct Reader {
  virtual ssize_t Read(void *buffer, size_t len, bela::error_code &ec) = 0;
};

struct ReaderAt : public virtual Reader {
  virtual ssize_t ReadAt(void *buffer, size_t len, int64_t pos, bela::error_code &ec) = 0;
};

struct Writer {
  virtual ssize_t Write(const void *buffer, size_t len, bela::error_code &ec) = 0;
};

struct WriterAt : public virtual Writer {
  virtual ssize_t WriteAt(const void *buffer, size_t len, int64_t pos, bela::error_code &ec) = 0;
};

struct ReadeWriter : public virtual Reader, public virtual Writer {};

inline ssize_t ReadAtLeast(Reader &r, void *buffer, size_t len, size_t min, bela::error_code &ec) {
  if (len < min) {
    ec = bela::make_error_code(L"short buffer");
    return -1;
  }
  auto p = reinterpret_cast<uint8_t *>(buffer);
  size_t n = 0;
  for (; n < min;) {
    auto nn = r.Read(p + n, len - n, ec);
    if (nn < 0) {
      return -1;
    }
    if (nn == 0) {
      break;
    }
    n += nn;
  }
  if (n < min) {
    ec = bela::make_error_code(L"unexpected EOF");
    return static_cast<ssize_t>(n);
  }
  return static_cast<ssize_t>(n);
}

inline ssize_t ReadFull(Reader &r, void *buffer, size_t len, bela::error_code &ec) {
  return ReadAtLeast(r, buffer, len, len, ec);
}

class File final : public ReaderAt, public WriterAt {
private:
  void Free();

public:
  File() = default;
  File(const File &) = delete;
  File &operator=(const File &) = delete;
  ~File() { Free(); }
  File(File &&o) {
    Free();
    fd = o.fd;
    o.fd = INVALID_HANDLE_VALUE;
  }
  File &operator=(File &&o) {
    Free();
    fd = o.fd;
    o.fd = INVALID_HANDLE_VALUE;
    return *this;
  }
  ssize_t Read(void *buffer, size_t len, bela::error_code &ec);
  ssize_t ReadAt(void *buffer, size_t len, int64_t pos, bela::error_code &ec);
  ssize_t Write(const void *buffer, size_t len, bela::error_code &ec);
  ssize_t WriteAt(const void *buffer, size_t len, int64_t pos, bela::error_code &ec);
  HANDLE FD() const { return fd; }
  bool Open(std::wstring_view file, bela::error_code &ec);
  bool Open(std::wstring_view file, DWORD dwDesiredAccess, DWORD dwShareMode,
            LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
            HANDLE hTemplateFile, bela::error_code &ec);
  std::wstring FullPath() const {
    bela::error_code ec;
    if (auto fp = bela::RealPathByHandle(fd, ec)) {
      return *fp;
    }
    return L"";
  }

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
};

inline void File::Free() {
  if (fd != INVALID_HANDLE_VALUE) {
    CloseHandle(fd);
    fd = INVALID_HANDLE_VALUE;
  }
}

inline ssize_t File::Read(void *buffer, size_t len, bela::error_code &ec) {
  DWORD drSize = {0};
  if (::ReadFile(fd, buffer, static_cast<DWORD>(len), &drSize, nullptr) != TRUE) {
    ec = bela::make_system_error_code(L"ReadFile: ");
    return -1;
  }
  return static_cast<ssize_t>(drSize);
}

inline ssize_t File::ReadAt(void *buffer, size_t len, int64_t pos, bela::error_code &ec) {
  auto li = *reinterpret_cast<LARGE_INTEGER *>(&pos);
  LARGE_INTEGER oli{0};
  if (SetFilePointerEx(fd, li, &oli, SEEK_SET) != TRUE) {
    ec = bela::make_error_code(L"SetFilePointerEx: ");
    return -1;
  }
  return Read(buffer, len, ec);
}

inline ssize_t File::Write(const void *buffer, size_t len, bela::error_code &ec) {
  DWORD dwSize = {0};
  if (WriteFile(fd, buffer, static_cast<DWORD>(len), &dwSize, nullptr) != TRUE) {
    ec = bela::make_system_error_code(L"WriteFile: ");
    return -1;
  }
  return static_cast<ssize_t>(dwSize);
}

inline ssize_t File::WriteAt(const void *buffer, size_t len, int64_t pos, bela::error_code &ec) {
  auto li = *reinterpret_cast<LARGE_INTEGER *>(&pos);
  LARGE_INTEGER oli{0};
  if (SetFilePointerEx(fd, li, &oli, SEEK_SET) != TRUE) {
    ec = bela::make_error_code(L"SetFilePointerEx: ");
    return -1;
  }
  return Write(buffer, len, ec);
}

inline bool File::Open(std::wstring_view file, bela::error_code &ec) {
  if (fd != INVALID_HANDLE_VALUE) {
    ec = bela::make_error_code(L"The file has been opened, the function cannot be called repeatedly");
    return false;
  }
  fd = CreateFileW(file.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL, nullptr);
  if (fd == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  return true;
}

// Open 
inline bool File::Open(std::wstring_view file, DWORD dwDesiredAccess, DWORD dwShareMode,
                       LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                       DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, bela::error_code &ec) {
  if (fd != INVALID_HANDLE_VALUE) {
    ec = bela::make_error_code(L"The file has been opened, the function cannot be called repeatedly");
    return false;
  }
  fd = CreateFileW(file.data(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition,
                   dwFlagsAndAttributes, hTemplateFile);
  if (fd == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  return true;
}

inline bool ReadAt(HANDLE fd, void *buffer, size_t len, uint64_t pos, size_t &outlen, bela::error_code &ec) {
  auto li = *reinterpret_cast<LARGE_INTEGER *>(&pos);
  LARGE_INTEGER oli{0};
  if (SetFilePointerEx(fd, li, &oli, SEEK_SET) != TRUE) {
    ec = bela::make_error_code(L"SetFilePointerEx: ");
    return false;
  }
  DWORD dwSize = {0};
  if (::ReadFile(fd, buffer, static_cast<DWORD>(len), &dwSize, nullptr) != TRUE) {
    ec = bela::make_system_error_code(L"ReadFile: ");
    return false;
  }
  outlen = static_cast<size_t>(len);
  return true;
}

[[maybe_unused]] constexpr auto MaximumRead = 1024ull * 1024 * 8; // 8MB
[[maybe_unused]] constexpr auto MaximumLineLength = 1024ull * 64; // 64KB
bool ReadFile(std::wstring_view file, std::wstring &out, bela::error_code &ec, uint64_t maxsize = MaximumRead);
bool ReadLine(std::wstring_view file, std::wstring &out, bela::error_code &ec, uint64_t maxline = MaximumLineLength);
inline std::optional<std::wstring> ReadLine(std::wstring_view file, bela::error_code &ec,
                                            uint64_t maxline = MaximumLineLength) {
  std::wstring line;
  if (ReadLine(file, line, ec, maxline)) {
    return std::make_optional(std::move(line));
  }
  return std::nullopt;
}
bool WriteTextU16LE(std::wstring_view text, std::wstring_view file, bela::error_code &ec);
bool WriteText(std::string_view text, std::wstring_view file, bela::error_code &ec);
bool WriteTextAtomic(std::string_view text, std::wstring_view file, bela::error_code &ec);
inline bool WriteText(std::wstring_view text, std::wstring_view file, bela::error_code &ec) {
  return WriteText(bela::ToNarrow(text), file, ec);
}
inline bool WriteText(std::u16string_view text, std::wstring_view file, bela::error_code &ec) {
  return WriteText(bela::ToNarrow(text), file, ec);
}
} // namespace io
using bela::io::File;
} // namespace bela

#endif