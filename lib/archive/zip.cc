/// decompress zip file
#include <filesystem>
#include <bela/codecvt.hpp> //UTF8 to UTF16
#include <bela/path.hpp>
#include "minizip/mz.h"
#include "minizip/mz_os.h"
#include "minizip/mz_strm_buf.h"
#include "minizip/mz_strm.h"
#include "minizip/mz_strm_split.h"
#include "minizip/mz_strm_os.h"
#include "minizip/mz_zip.h"
#include "minizip/mz_zip_rw.h"
// defined
#include "zip.hpp"

namespace baulk::archive::zip {

bool ZipExtract(std::wstring_view file, std::wstring_view dest, bela::error_code &ec,
                const zip_closure *closure, int encoding) {
  void *reader = NULL;
  int32_t err = MZ_OK;
  int32_t err_close = MZ_OK;
  auto path = bela::ToNarrow(file);
  auto destination = bela::ToNarrow(dest);
  mz_zip_reader_create(&reader);
  auto closer = bela::finally([&] {
    if (err_close = mz_zip_reader_close(reader); err_close != MZ_OK) {
      bela::StrAppend(&ec.message, L" mz_zip_reader_close return: ", err_close);
    }
    mz_zip_reader_delete(&reader);
  });
  if (encoding > 0) {
    mz_zip_reader_set_encoding(reader, encoding);
  }
  if (closure != nullptr) {
    if (closure->progress != nullptr) {
      mz_zip_reader_set_progress_cb(reader, closure->userdata, closure->progress);
    }
    if (closure->entry != nullptr) {
      mz_zip_reader_set_entry_cb(reader, closure->userdata, closure->entry);
    }
  }
  if (err = mz_zip_reader_open_file(reader, path.data()); err != MZ_OK) {
    ec = bela::make_error_code(1, L"unable open file, minizip return: ", err);
    return false;
  }
  if (err = mz_zip_reader_save_all(reader, destination.data());
      err != MZ_OK && err != MZ_END_OF_LIST) {
    ec = bela::make_error_code(1, L"unable save all, minizip return: ", err);
    return false;
  }
  return err_close == MZ_OK;
}

} // namespace baulk::archive::zip