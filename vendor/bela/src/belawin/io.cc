//
#include <bela/base.hpp>
#include <bela/mapview.hpp>
#include <bela/endian.hpp>
#include <bela/io.hpp>
#include <bela/path.hpp>

namespace bela::io {
bool ReadFile(std::wstring_view file, std::wstring &out, bela::error_code &ec, uint64_t maxsize) {
  bela::MapView mv;
  if (!mv.MappingView(file, ec, 1, maxsize)) {
    return false;
  }
  auto mmv = mv.subview();
  constexpr uint8_t utf8bom[] = {0xEF, 0xBB, 0xBF};
  constexpr uint8_t utf16le[] = {0xFF, 0xFE};
  constexpr uint8_t utf16be[] = {0xFE, 0xFF};
  if (mmv.StartsWith(utf8bom)) {
    out = bela::ToWide(mmv.submv(3).sv());
    return true;
  }
  if constexpr (bela::IsLittleEndianHost) {
    if (mmv.StartsWith(utf16le)) {
      auto sv = mmv.submv(2).sv();
      out.assign(reinterpret_cast<const wchar_t *>(sv.data()), sv.size() / 2);
      return true;
    }
    if (mmv.StartsWith(utf16be)) {
      auto sv = mmv.submv(2).sv();
      std::wstring_view wsv{reinterpret_cast<const wchar_t *>(sv.data()), sv.size() / 2};
      out.resize(wsv.size());
      wchar_t *p = out.data();
      for (size_t i = 0; i < wsv.size(); i++) {
        p[i] = bela::swapbe(wsv[i]);
      }
      return true;
    }
  } else {
    if (mmv.StartsWith(utf16be)) {
      auto sv = mmv.submv(2).sv();
      out.assign(reinterpret_cast<const wchar_t *>(sv.data()), sv.size() / 2);
      return true;
    }
    if (mmv.StartsWith(utf16le)) {
      auto sv = mmv.submv(2).sv();
      std::wstring_view wsv{reinterpret_cast<const wchar_t *>(sv.data()), sv.size() / 2};
      out.resize(wsv.size());
      wchar_t *p = out.data();
      for (size_t i = 0; i < wsv.size(); i++) {
        p[i] = bela::swaple(wsv[i]);
      }
      return true;
    }
  }
  out = bela::ToWide(mmv.sv());
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

bool WriteTextInternal(std::string_view bom, std::string_view text, std::wstring_view file,
                       bela::error_code &ec) {
  auto FileHandle =
      ::CreateFileW(file.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  auto closer = bela::finally([&] { CloseHandle(FileHandle); });
  DWORD written = 0;
  if (!bom.empty()) {
    if (WriteFile(FileHandle, bom.data(), static_cast<DWORD>(bom.size()), &written, nullptr) !=
        TRUE) {
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
  std::string_view text_{reinterpret_cast<const char *>(text.data()),
                         text.size() * sizeof(wchar_t)};
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
