//
#include <type_traits>
#include <hazel/hazel.hpp>
#include <bela/mapview.hpp>
#include <bela/path.hpp>
#include "ina/hazelinc.hpp"

namespace hazel {

typedef hazel::internal::status_t (*lookup_handle_t)(bela::MemView mv, hazel_result &hr);

bool LookupFile(bela::File &fd, hazel_result &hr, bela::error_code &ec) {
  LARGE_INTEGER li = {0};
  if (GetFileSizeEx(fd.FD(), &li) != TRUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  hr.size_ = li.QuadPart;
  uint8_t buffer[4096];
  auto outlen = fd.ReadAt(buffer, sizeof(buffer), 0, ec);
  if (outlen == -1) {
    return false;
  }
  if (auto p = memchr(buffer, 0, outlen); p != nullptr) {
    hr.zeroPosition = static_cast<int64_t>(reinterpret_cast<const uint8_t *>(p) - buffer);
  }
  bela::MemView mv(buffer, static_cast<size_t>(outlen));
  using namespace hazel::internal;
  constexpr lookup_handle_t handles[] = {
      LookupExecutableFile, //
      LookupArchives,       // 7z ...
      LookupDocs,
      LookupFonts,     //
      LookupShellLink, // shortcut
      LookupMedia,     // media
      LookupImages,    // images
      LookupText,
  };
  for (auto h : handles) {
    if (h(mv, hr) == Found) {
      return true;
    }
  }
  return false;
}

} // namespace hazel