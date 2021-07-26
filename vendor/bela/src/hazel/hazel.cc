//
#include <type_traits>
#include <hazel/hazel.hpp>
#include <bela/path.hpp>
#include <bela/os.hpp>
#include "ina/hazelinc.hpp"

namespace hazel {

typedef hazel::internal::status_t (*lookup_handle_t)(bela::bytes_view bv, hazel_result &hr);

bool LookupFile(bela::io::FD &fd, hazel_result &hr, bela::error_code &ec) {
  if ((hr.size_ = bela::io::Size(fd.NativeFD(), ec)) == bela::SizeUnInitialized) {
    return false;
  }
  uint8_t buffer[4096];
  auto minSize = (std::min)(hr.size_, 4096ll);
  if (!fd.ReadAt({buffer, static_cast<size_t>(minSize)}, 0, ec)) {
    return false;
  }
  if (auto p = memchr(buffer, 0, static_cast<size_t>(minSize)); p != nullptr) {
    hr.zeroPosition = static_cast<int64_t>(reinterpret_cast<const uint8_t *>(p) - buffer);
  }
  bela::bytes_view bv(buffer, static_cast<size_t>(minSize));
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
    if (h(bv, hr) == Found) {
      return true;
    }
  }
  return false;
}

} // namespace hazel