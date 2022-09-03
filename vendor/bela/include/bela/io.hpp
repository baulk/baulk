// Bela IO utils
#ifndef BELA_IO_HPP
#define BELA_IO_HPP
#include "base.hpp"
#include "types.hpp"
#include "buffer.hpp"
#include "os.hpp"
#include "path.hpp"

namespace bela::io {
template <typename CharT = uint8_t>
requires bela::character<CharT>
[[nodiscard]] inline auto as_bytes(const std::basic_string_view<CharT, std::char_traits<CharT>> sv) {
  return std::span<const uint8_t>{reinterpret_cast<const uint8_t *>(sv.data()), sv.size() * sizeof(CharT)};
}

// WriteFull reads buffer.size() bytes from the File starting at byte offset pos.
bool WriteFull(HANDLE fd, std::span<const uint8_t> buffer, bela::error_code &ec);
// ReadAt reads buffer.size() bytes from the File starting at byte offset pos.
bool ReadFull(HANDLE fd, std::span<uint8_t> buffer, bela::error_code &ec);
// Size get file size
inline int64_t Size(HANDLE fd, bela::error_code &ec) {
  FILE_STANDARD_INFO si;
  if (GetFileInformationByHandleEx(fd, FileStandardInfo, &si, sizeof(si)) != TRUE) {
    ec = bela::make_system_error_code(L"GetFileInformationByHandleEx(): ");
    return bela::SizeUnInitialized;
  }
  return si.EndOfFile.QuadPart;
}
// Size get file size
inline int64_t Size(std::wstring_view filename, bela::error_code &ec) {
  WIN32_FILE_ATTRIBUTE_DATA wdata;
  if (GetFileAttributesExW(filename.data(), GetFileExInfoStandard, &wdata) != TRUE) {
    ec = bela::make_system_error_code(L"GetFileAttributesExW(): ");
    return bela::SizeUnInitialized;
  }
  return static_cast<unsigned long long>(wdata.nFileSizeHigh) << 32 | wdata.nFileSizeLow;
}

enum Whence : DWORD {
  SeekStart = FILE_BEGIN,
  SeekCurrent = FILE_CURRENT,
  SeekEnd = FILE_END,
};
// Seek pos
inline bool Seek(HANDLE fd, int64_t pos, bela::error_code &ec, Whence whence = SeekStart) {
  LARGE_INTEGER new_pos{.QuadPart = 0};
  if (SetFilePointerEx(fd, *reinterpret_cast<LARGE_INTEGER const *>(&pos), &new_pos, whence) != TRUE) {
    ec = bela::make_system_error_code(L"SetFilePointerEx(): ");
    return false;
  }
  return true;
}

// Avoid the uint8_t buffer from incorrectly matching the template function
template <class T>
concept vectorizable_derived = std::is_standard_layout_v<T> &&(!std::same_as<T, uint8_t>);

template <class T>
concept exclude_buffer_derived = std::is_standard_layout_v<T> &&(!std::same_as<T, bela::Buffer>);

class FD {
private:
  void Free();
  void MoveFrom(FD &&o);

public:
  FD() = default;
  FD(HANDLE fd_, bool needClosed_ = true) : fd(fd_), needClosed(needClosed_) {}
  FD(const FD &) = delete;
  FD &operator=(const FD &) = delete;
  FD(FD &&o) { MoveFrom(std::move(o)); }
  FD &operator=(FD &&o) {
    MoveFrom(std::move(o));
    return *this;
  }
  ~FD() { Free(); }
  FD &Assgin(FD &&o) {
    MoveFrom(std::move(o));
    return *this;
  }
  FD &Assgin(HANDLE fd_, bool needClosed_ = true) {
    Free();
    fd = fd_;
    needClosed = needClosed_;
    return *this;
  }
  explicit operator bool() const { return fd != INVALID_HANDLE_VALUE; }
  const auto NativeFD() const { return fd; }
  int64_t Size(bela::error_code &ec) const { return bela::io::Size(fd, ec); }
  bool Seek(int64_t pos, bela::error_code &ec, Whence whence = SeekStart) const {
    return bela::io::Seek(fd, pos, ec, whence);
  }
  // ReadAt reads buffer.size() bytes into p starting at offset off in the underlying input source. 0 <= outlen <=
  // buffer.size()
  // Try to read bytes into the buffer
  bool ReadAt(std::span<uint8_t> buffer, int64_t pos, int64_t &outlen, bela::error_code &ec) const {
    if (!bela::io::Seek(fd, pos, ec)) {
      return false;
    }
    DWORD dwSize = 0;
    if (::ReadFile(fd, buffer.data(), static_cast<DWORD>(buffer.size()), &dwSize, nullptr) != TRUE) {
      ec = bela::make_system_error_code(L"ReadFile: ");
      return false;
    }
    outlen = static_cast<int64_t>(dwSize);
    return true;
  }
  // ReadAt reads buffer.size() bytes into p starting at offset off in the underlying input source. 0 <= outlen <=
  // buffer.size()
  // Try to read bytes into the buffer
  template <typename T>
  requires exclude_buffer_derived<T>
  bool ReadAt(T &t, int64_t pos, int64_t &outlen, bela::error_code &ec) const {
    return ReadAt({reinterpret_cast<uint8_t *>(&t), sizeof(T)}, pos, outlen, ec);
  }
  // ReadAt reads buffer.size() bytes into p starting at offset off in the underlying input source. 0 <= outlen <=
  // buffer.size()
  // Try to read bytes into the buffer
  template <typename T>
  requires vectorizable_derived<T>
  bool ReadAt(std::vector<T> &tv, int64_t pos, int64_t &outlen, bela::error_code &ec) const {
    return ReadAt({reinterpret_cast<uint8_t *>(tv.data()), sizeof(T) * tv.size()}, pos, outlen, ec);
  }

  // ReadFull reads buffer.size() bytes from the File starting at byte offset pos.
  bool ReadFull(std::span<uint8_t> buffer, bela::error_code &ec) const { return bela::io::ReadFull(fd, buffer, ec); }
  bool ReadFull(bela::Buffer &buffer, size_t nbytes, bela::error_code &ec) const {
    if (auto p = buffer.make_span(nbytes); bela::io::ReadFull(fd, p, ec)) {
      buffer.size() = p.size();
      return true;
    }
    return false;
  }
  // ReadAt reads buffer.size() bytes into p starting at offset off in the underlying input source
  // Force a full buffer
  bool ReadAt(std::span<uint8_t> buffer, int64_t pos, bela::error_code &ec) const {
    if (!bela::io::Seek(fd, pos, ec)) {
      return false;
    }
    return bela::io::ReadFull(fd, buffer, ec);
  }
  // ReadAt reads nbytes bytes into p starting at offset off in the underlying input source
  // Force a full buffer
  bool ReadAt(bela::Buffer &buffer, size_t nbytes, int64_t pos, bela::error_code &ec) const {
    if (auto p = buffer.make_span(nbytes); ReadAt(p, pos, ec)) {
      buffer.size() = p.size();
      return true;
    }
    return false;
  }
  template <typename T>
  requires exclude_buffer_derived<T>
  bool ReadFull(T &t, bela::error_code &ec) const {
    return bela::io::ReadFull(fd, {reinterpret_cast<uint8_t *>(&t), sizeof(T)}, ec);
  }
  template <typename T>
  requires vectorizable_derived<T>
  bool ReadFull(std::vector<T> &tv, bela::error_code &ec) const {
    return bela::io::ReadFull(fd, {reinterpret_cast<uint8_t *>(tv.data()), sizeof(T) * tv.size()}, ec);
  }
  // ReadAt reads sizeof T bytes into p starting at offset off in the underlying input source
  // Force a full buffer
  template <typename T>
  requires exclude_buffer_derived<T>
  bool ReadAt(T &t, int64_t pos, bela::error_code &ec) const {
    return ReadAt({reinterpret_cast<uint8_t *>(&t), sizeof(T)}, pos, ec);
  }
  // ReadAt reads vector bytes into p starting at offset off in the underlying input source
  // Force a full buffer
  template <typename T>
  requires vectorizable_derived<T>
  bool ReadAt(std::vector<T> &tv, int64_t pos, bela::error_code &ec) const {
    return ReadAt({reinterpret_cast<uint8_t *>(tv.data()), sizeof(T) * tv.size()}, pos, ec);
  }

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
  bool needClosed{true};
};

std::optional<FD> NewFile(std::wstring_view file, bela::error_code &ec);
std::optional<FD> NewFile(std::wstring_view file, DWORD dwDesiredAccess, DWORD dwShareMode,
                          LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                          DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, bela::error_code &ec);

inline bool ReadAt(HANDLE fd, void *buffer, size_t len, int64_t pos, size_t &outlen, bela::error_code &ec) {
  if (!bela::io::Seek(fd, pos, ec)) {
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
bool ReadFile(std::wstring_view file, std::string &out, bela::error_code &ec, uint64_t maxsize = MaximumRead);
bool ReadLine(std::wstring_view file, std::wstring &out, bela::error_code &ec, uint64_t maxline = MaximumLineLength);
inline std::optional<std::wstring> ReadLine(std::wstring_view file, bela::error_code &ec,
                                            uint64_t maxline = MaximumLineLength) {
  std::wstring line;
  if (ReadLine(file, line, ec, maxline)) {
    return std::make_optional(std::move(line));
  }
  return std::nullopt;
}

// A byte order mark (BOM) consists of the character code U+FEFF at the beginning of a data stream, where it can be used
// as a signature defining the byte order and encoding form, primarily of unmarked plaintext files. Under some higher
// level protocols, use of a BOM may be mandatory (or prohibited) in the Unicode data stream defined in that protocol.
constexpr uint8_t utf8_bytes[] = {0xEF, 0xBB, 0xBF};
constexpr uint8_t utf16le_bytes[] = {0xFF, 0xFE};
constexpr uint8_t utf16be_bytes[] = {0xFE, 0xFF};
constexpr std::span<const uint8_t> byte_order_mark_non = {};
constexpr std::span<const uint8_t> byte_order_mark_utf8 = {utf8_bytes};
constexpr std::span<const uint8_t> byte_order_mark_utf16le = {utf16le_bytes};
constexpr std::span<const uint8_t> byte_order_mark_utf16be = {utf16be_bytes};

// WriteText: Low-level interface for writing text to a file
bool WriteText(std::wstring_view file, std::span<const uint8_t> bom, const std::span<const uint8_t> text,
               bela::error_code &ec);
// WriteText: Write UTF-8 text to a file.
inline bool WriteText(std::wstring_view file, const std::span<const uint8_t> text, bela::error_code &ec) {
  return WriteText(file, byte_order_mark_non, text, ec);
}

// WriteText: Convert UTF-16 text to UTF-8 and write to file
inline bool WriteText(std::wstring_view file, const std::span<const wchar_t> text, bela::error_code &ec) {
  auto u8text = bela::encode_into<wchar_t, char>({text.data(), text.size()});
  return WriteText(file, byte_order_mark_non, as_bytes<char>(u8text), ec);
}

// WriteText: Convert UTF-16 text to UTF-8 and write to file
inline bool WriteText(std::wstring_view file, const std::span<const char16_t> text, bela::error_code &ec) {
  auto u8text = bela::encode_into<char16_t, char>({text.data(), text.size()});
  return WriteText(file, byte_order_mark_non, as_bytes<char>(u8text), ec);
}

// AtomicWriteText: Atomically writes UTF-8 text to file
bool AtomicWriteText(std::wstring_view file, const std::span<const uint8_t> text, bela::error_code &ec);

// WriteTextU16LE:  Write UTF-16 LE text to a file.
bool WriteTextU16LE(std::wstring_view file, const std::span<const wchar_t> text, bela::error_code &ec);
inline bool WriteTextU16LE(std::wstring_view file, const std::span<const char16_t> text, bela::error_code &ec) {
  return WriteTextU16LE(file, {reinterpret_cast<const wchar_t *>(text.data()), text.size()}, ec);
}

} // namespace bela::io

#endif