//
#include <bela/base.hpp>
#include <bela/endian.hpp>
#include <bela/io.hpp>
#include <bela/path.hpp>

namespace bela::io {
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

bool FD::ReadFull(std::span<uint8_t> buffer, bela::error_code &ec) const {
  if (buffer.size() == 0) {
    return true;
  }
  auto p = reinterpret_cast<uint8_t *>(buffer.data());
  auto len = buffer.size();
  size_t total = 0;
  while (total < len) {
    DWORD dwSize = 0;
    if (::ReadFile(fd, p + total, static_cast<DWORD>(len - total), &dwSize, nullptr) != TRUE) {
      ec = bela::make_system_error_code(L"ReadFile: ");
      return false;
    }
    if (dwSize == 0) {
      ec = bela::make_error_code(ErrEOF, L"Reached the end of the file");
      return false;
    }
    total += dwSize;
  }
  return true;
}

bool FD::ReadAt(std::span<uint8_t> buffer, int64_t pos, bela::error_code &ec) const {
  if (!bela::io::Seek(fd, pos, ec)) {
    return false;
  }
  return ReadFull(buffer, ec);
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
  constexpr uint8_t utf8bom[] = {0xEF, 0xBB, 0xBF};
  constexpr uint8_t utf16le[] = {0xFF, 0xFE};
  constexpr uint8_t utf16be[] = {0xFE, 0xFF};
  if (bv.starts_bytes_with(utf8bom)) {
    out = bela::ToWide(bv.make_string_view<char>(3));
    return true;
  }
  if constexpr (bela::IsLittleEndian()) {
    if (bv.starts_bytes_with(utf16le)) {
      out = bv.make_string_view<wchar_t>(2);
      return true;
    }
    if (bv.starts_bytes_with(utf16be)) {
      auto wsv = bv.make_string_view<wchar_t>(2);
      out.resize(wsv.size());
      wchar_t *p = out.data();
      for (size_t i = 0; i < wsv.size(); i++) {
        p[i] = static_cast<wchar_t>(bela::frombe(static_cast<uint16_t>(wsv[i])));
      }
      return true;
    }
  } else {
    if (bv.starts_bytes_with(utf16be)) {
      out = bv.make_string_view<wchar_t>(2);
      return true;
    }
    if (bv.starts_bytes_with(utf16le)) {
      auto wsv = bv.make_string_view<wchar_t>(2);
      out.resize(wsv.size());
      wchar_t *p = out.data();
      for (size_t i = 0; i < wsv.size(); i++) {
        p[i] = bela::fromle(wsv[i]);
      }
      return true;
    }
  }
  out = bela::ToWide(bv.make_string_view<char>());
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

bool WriteTextInternal(std::string_view bom, std::string_view text, std::wstring_view file, bela::error_code &ec) {
  auto FileHandle = ::CreateFileW(file.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  auto closer = bela::finally([&] { CloseHandle(FileHandle); });
  DWORD written = 0;
  if (!bom.empty()) {
    if (WriteFile(FileHandle, bom.data(), static_cast<DWORD>(bom.size()), &written, nullptr) != TRUE) {
      ec = bela::make_system_error_code();
      return false;
    }
  }
  written = 0;
  auto p = text.data();
  auto size = text.size();
  while (size > 0) {
    auto len = (std::min)(size, static_cast<size_t>(4096));
    if (WriteFile(FileHandle, p, static_cast<DWORD>(len), &written, nullptr) != TRUE) {
      ec = bela::make_system_error_code();
      break;
    }
    size -= written;
    p += written;
  }
  return size == 0;
}

bool WriteTextU16LE(std::wstring_view text, std::wstring_view file, bela::error_code &ec) {
  if constexpr (bela::IsBigEndian()) {
    constexpr uint8_t u16bebom[] = {0xFE, 0xFF};
    std::string_view bom{reinterpret_cast<const char *>(u16bebom), 2};
    std::wstring s(text);
    for (auto &ch : s) {
      ch = static_cast<uint16_t>(bela::swap16(static_cast<uint16_t>(ch)));
    }
    std::string_view text_{reinterpret_cast<const char *>(s.data()), s.size() * sizeof(wchar_t)};
    return WriteTextInternal(bom, text_, file, ec);
  }
  constexpr uint8_t u16lebom[] = {0xFF, 0xFE};
  std::string_view bom{reinterpret_cast<const char *>(u16lebom), 2};
  std::string_view text_{reinterpret_cast<const char *>(text.data()), text.size() * sizeof(wchar_t)};
  return WriteTextInternal(bom, text_, file, ec);
}

bool WriteText(std::string_view text, std::wstring_view file, bela::error_code &ec) {
  return WriteTextInternal("", text, file, ec);
}

bool WriteTextAtomic(std::string_view text, std::wstring_view file, bela::error_code &ec) {
  if (!bela::PathExists(file)) {
    return WriteTextInternal("", text, file, ec);
  }
  auto lock = bela::StringCat(file, L".lock");
  if (!WriteText(text, lock, ec)) {
    DeleteFileW(lock.data());
    return false;
  }
  auto old = bela::StringCat(file, L".old");
  if (MoveFileW(file.data(), old.data()) != TRUE) {
    ec = bela::make_system_error_code();
    DeleteFileW(lock.data());
    return false;
  }
  if (MoveFileW(lock.data(), file.data()) != TRUE) {
    ec = bela::make_system_error_code();
    MoveFileW(old.data(), file.data());
    return false;
  }
  DeleteFileW(old.data());
  return true;
}

} // namespace bela::io
