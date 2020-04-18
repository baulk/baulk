/// decompress zip file
#include <bela/codecvt.hpp> //UTF8 to UTF16
#include "zip.hpp"
#include "bzip2/bzlib.h" //zip bzip2 support

namespace zip {
bool ZipReader::Initialize(std::wstring_view zipfile, std::wstring_view dest,
                           bela::error_code &ec) {
  return false;
}
} // namespace zip