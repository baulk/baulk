//
#include "extractor.hpp"

namespace baulk::archive {
std::shared_ptr<Extractor> NewExtractor(std::wstring_view filename, std::wstring_view chroot,
                                        const ExtractorOptions &opts, bela::error_code &ec) {
  file_format_t afmt{file_format_t::none};
  int64_t offset{0};
  auto fd = baulk::archive::NewArchiveFile(filename, afmt, offset, ec);
  if (!fd) {
    return nullptr;
  }
  switch (afmt) {
  case file_format_t::zip:
    break;
  case file_format_t::rar:
    break;
  case file_format_t::_7z:
    break;
  case file_format_t::tar:
    break;
  case file_format_t::cab:
    break;
  case file_format_t::eot:
    break;
  case file_format_t::lz:
    break;
  case file_format_t::xz:
    break;
  case file_format_t::zstd:
    break;
  case file_format_t::bz2:
    break;
  default:
    break;
  }
  ec = bela::make_error_code(ErrUnimplemented, L"unsupport archive format ", ArchiveFormatName(afmt));
  return nullptr;
}
} // namespace baulk::archive