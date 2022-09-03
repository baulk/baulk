//
#include <bela/base.hpp>
#include <bela/endian.hpp>
#include <bela/io.hpp>
#include <bela/path.hpp>

namespace bela::io {
constexpr auto ulmax = (std::numeric_limits<DWORD>::max)();

bool WriteFull(HANDLE fd, std::span<const uint8_t> buffer, bela::error_code &ec) {
  auto p = buffer.data();
  size_t size = buffer.size();
  for (;;) {
    if (size == 0) {
      break;
    }
    DWORD bytes{0};
    if (WriteFile(fd, p, static_cast<DWORD>((std::min)(static_cast<size_t>(ulmax), size)), &bytes, nullptr) != TRUE) {
      ec = bela::make_system_error_code(L"WriteFile: ");
      return false;
    }
    p += bytes;
    size -= bytes;
  }
  return true;
}

bool ReadFull(HANDLE fd, std::span<uint8_t> buffer, bela::error_code &ec) {
  auto p = reinterpret_cast<uint8_t *>(buffer.data());
  auto size = buffer.size();
  for (;;) {
    if (size == 0) {
      break;
    }
    DWORD bytes{0};
    if (::ReadFile(fd, p, static_cast<DWORD>((std::min)(static_cast<size_t>(ulmax), size)), &bytes, nullptr) != TRUE) {
      ec = bela::make_system_error_code(L"ReadFile: ");
      return false;
    }
    if (bytes == 0) {
      ec = bela::make_error_code(ErrEOF, L"Reached the end of the file");
      return false;
    }
    p += bytes;
    size -= bytes;
  }
  return true;
}

void FD::Free() {
  if (fd != INVALID_HANDLE_VALUE && needClosed) {
    CloseHandle(fd);
  }
  fd = INVALID_HANDLE_VALUE;
}
void FD::MoveFrom(FD &&o) {
  Free();
  fd = o.fd;
  needClosed = o.needClosed;
  o.fd = INVALID_HANDLE_VALUE;
  o.needClosed = false;
}

std::optional<FD> NewFile(std::wstring_view file, bela::error_code &ec) {
  auto fd = CreateFileW(file.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, nullptr);
  if (fd == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code(L"CreateFileW() ");
    return std::nullopt;
  }
  return std::make_optional<FD>(fd, true);
}

std::optional<FD> NewFile(std::wstring_view file, DWORD dwDesiredAccess, DWORD dwShareMode,
                          LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                          DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, bela::error_code &ec) {
  auto fd = CreateFileW(file.data(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition,
                        dwFlagsAndAttributes, hTemplateFile);
  if (fd == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code(L"CreateFileW() ");
    return std::nullopt;
  }
  return std::make_optional<FD>(fd, true);
}

inline void bytes_switch(std::wstring &out, std::wstring_view text) {
  out.resize(text.size());
  auto dest = reinterpret_cast<std::uint16_t *>(out.data());
  auto src = reinterpret_cast<const std::uint16_t *>(text.data());
  for (size_t i = 0; i < text.size(); i++) {
    dest[i] = bela::swap16(src[i]);
  }
}

bool ReadFile(std::wstring_view file, std::wstring &out, bela::error_code &ec, uint64_t maxsize) {
  auto fd = bela::io::NewFile(file, ec);
  if (!fd) {
    return false;
  }
  auto size = fd->Size(ec);
  if (size == bela::SizeUnInitialized) {
    return false;
  }
  auto maxSize = (std::min)(static_cast<size_t>(maxsize), static_cast<size_t>(size));
  bela::Buffer buffer(maxSize);
  if (!fd->ReadFull(buffer, maxSize, ec)) {
    return false;
  }
  auto bv = buffer.as_bytes_view();
  if (bv.starts_bytes_with(utf8_bytes)) {
    out = bela::encode_into<char, wchar_t>(bv.make_string_view<char>(3));
    return true;
  }
  if constexpr (bela::IsLittleEndian()) {
    if (bv.starts_bytes_with(utf16le_bytes)) {
      out = bv.make_string_view<wchar_t>(2);
      return true;
    }
    if (bv.starts_bytes_with(utf16be_bytes)) {
      bytes_switch(out, bv.make_string_view<wchar_t>(2));
      return true;
    }
  } else {
    if (bv.starts_bytes_with(utf16be_bytes)) {
      out = bv.make_string_view<wchar_t>(2);
      return true;
    }
    if (bv.starts_bytes_with(utf16le_bytes)) {
      bytes_switch(out, bv.make_string_view<wchar_t>(2));
      return true;
    }
  }
  out = bela::encode_into<char, wchar_t>(bv.make_string_view<char>());
  return true;
}

bool ReadFile(std::wstring_view file, std::string &out, bela::error_code &ec, uint64_t maxsize) {
  auto fd = bela::io::NewFile(file, ec);
  if (!fd) {
    return false;
  }
  auto size = fd->Size(ec);
  if (size == bela::SizeUnInitialized) {
    return false;
  }
  auto maxSize = (std::min)(static_cast<size_t>(maxsize), static_cast<size_t>(size));
  bela::Buffer buffer(maxSize);
  if (!fd->ReadFull(buffer, maxSize, ec)) {
    return false;
  }
  auto bv = buffer.as_bytes_view();
  if (bv.starts_bytes_with(utf8_bytes)) {
    out = bv.make_string_view<char>(3);
    return true;
  }
  if constexpr (bela::IsLittleEndian()) {
    if (bv.starts_bytes_with(utf16le_bytes)) {
      out = bela::encode_into<wchar_t, char>(bv.make_string_view<wchar_t>(2));
      return true;
    }
    if (bv.starts_bytes_with(utf16be_bytes)) {
      std::wstring u16out;
      bytes_switch(u16out, bv.make_string_view<wchar_t>(2));
      out = bela::encode_into<wchar_t, char>(u16out);
      return true;
    }
  } else {
    if (bv.starts_bytes_with(utf16be_bytes)) {
      out = bela::encode_into<wchar_t, char>(bv.make_string_view<wchar_t>(2));
      return true;
    }
    if (bv.starts_bytes_with(utf16le_bytes)) {
      std::wstring u16out;
      bytes_switch(u16out, bv.make_string_view<wchar_t>(2));
      out = bela::encode_into<wchar_t, char>(u16out);
      return true;
    }
  }
  out = bv.make_string_view<char>();
  return true;
}

bool ReadLine(std::wstring_view file, std::wstring &out, bela::error_code &ec, uint64_t maxline) {
  if (!ReadFile(file, out, ec, maxline)) {
    return false;
  }
  if (auto pos = out.find_first_of(L"\r\n"); pos != std::wstring::npos) {
    out.resize(pos);
  }
  return true;
}

bool WriteText(const std::wstring_view file, const std::span<const uint8_t> bom, const std::span<const uint8_t> text,
               bela::error_code &ec) {
  auto fd = ::CreateFileW(file.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, nullptr);
  if (fd == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  auto closer = bela::finally([&] { CloseHandle(fd); });
  if (!WriteFull(fd, bom, ec)) {
    return false;
  }
  return WriteFull(fd, text, ec);
}

bool WriteTextU16LE(std::wstring_view file, const std::span<const wchar_t> text, bela::error_code &ec) {
  if constexpr (bela::IsBigEndian()) {
    std::wstring out;
    bytes_switch(out, {text.data(), text.size()});
    return WriteText(file, byte_order_mark_utf16le,
                     {reinterpret_cast<const uint8_t *>(out.data()), out.size() * sizeof(wchar_t)}, ec);
  }
  return WriteText(file, byte_order_mark_utf16le,
                   {reinterpret_cast<const uint8_t *>(text.data()), text.size() * sizeof(char16_t)}, ec);
}

bool AtomicWriteText(std::wstring_view file, const std::span<const uint8_t> text, bela::error_code &ec) {
  auto newfile = bela::StringCat(file, L".", GetCurrentProcessId(), L".new");
  if (!WriteText(newfile, byte_order_mark_non, text, ec)) {
    return false;
  }
  if (MoveFileExW(newfile.data(), file.data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) != TRUE) {
    ec = bela::make_system_error_code();
    DeleteFileW(newfile.data());
    return false;
  }
  return true;
}

} // namespace bela::io
