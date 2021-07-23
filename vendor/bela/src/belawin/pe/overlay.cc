// read pe overlay data
#include "internal.hpp"

namespace bela::pe {
int64_t File::ReadOverlay(std::span<uint8_t> overlayData, bela::error_code &ec) const {
  if (size <= overlayOffset) {
    ec = bela::make_error_code(ErrNoOverlay, L"no overlay data");
    return -1;
  }
  auto readSize = (std::min)(static_cast<int64_t>(overlayData.size()), size - overlayOffset);
  if (!fd.ReadAt(overlayData.subspan(0, readSize), overlayOffset, ec)) {
    return -1;
  }
  return readSize;
}

} // namespace bela::pe