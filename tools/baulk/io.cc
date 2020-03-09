///
#include "io.hpp"
#include <bela/finaly.hpp>
#include <bela/mapview.hpp>
#include <bela/endian.hpp>

namespace baulk::io {
constexpr size_t MB = 1024ull * 1024;
bool ReadAll(std::wstring_view file, std::wstring &out, bela::error_code &ec,
             uint64_t maxsize) {
  HANDLE FileHandle =
      CreateFileW(file.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  auto deleter = bela::finally([&] { CloseHandle(FileHandle); });
  LARGE_INTEGER li;
  if (GetFileSizeEx(FileHandle, &li) != TRUE) {
    ec = bela::make_system_error_code();
    return false;
  }
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
      std::wstring_view wsv{reinterpret_cast<const wchar_t *>(sv.data()),
                            sv.size() / 2};
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
      std::wstring_view wsv{reinterpret_cast<const wchar_t *>(sv.data()),
                            sv.size() / 2};
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

std::optional<std::wstring> ReadLine(std::wstring_view file,
                                     bela::error_code &ec, uint64_t maxline) {
  std::wstring line;
  if (!ReadAll(file, line, ec, maxline)) {
    return std::nullopt;
  }
  if (auto pos = line.find_first_of(L"\r\n"); pos != std::wstring::npos) {
    line.resize(pos);
  }
  return std::make_optional(std::move(line));
}
} // namespace baulk::io