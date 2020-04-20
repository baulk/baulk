/// decompress zip file
#include <filesystem>
#include <bela/codecvt.hpp> //UTF8 to UTF16
#include <bela/path.hpp>
#include "zip.hpp"
// Do not modify the include order of header files
#include "minizip/mz.h"
#include "minizip/mz_os.h"
#include "minizip/mz_strm_buf.h"
#include "minizip/mz_strm.h"
#include "minizip/mz_strm_split.h"
#include "minizip/mz_strm_os.h"
#include "minizip/mz_zip.h"
#include "minizip/mz_zip_rw.h"

extern "C" {
typedef struct mz_zip_reader_s {
  void *zip_handle;
  void *file_stream;
  void *buffered_stream;
  void *split_stream;
  void *mem_stream;
  void *hash;
  uint16_t hash_algorithm;
  uint16_t hash_digest_size;
  mz_zip_file *file_info;
  const char *pattern;
  uint8_t pattern_ignore_case;
  const char *password;
  void *overwrite_userdata;
  mz_zip_reader_overwrite_cb overwrite_cb;
  void *password_userdata;
  mz_zip_reader_password_cb password_cb;
  void *progress_userdata;
  mz_zip_reader_progress_cb progress_cb;
  uint32_t progress_cb_interval_ms;
  void *entry_userdata;
  mz_zip_reader_entry_cb entry_cb;
  uint8_t raw;
  uint8_t buffer[UINT16_MAX];
  int32_t encoding;
  uint8_t sign_required;
  uint8_t cd_verified;
  uint8_t cd_zipped;
  uint8_t entry_verified;
} mz_zip_reader;
typedef struct mz_stream_win32_s {
  mz_stream stream;
  HANDLE handle;
  int32_t error;
} mz_stream_win32;
}

namespace baulk::archive::zip {

bool PathResolve(std::wstring_view path, std::wstring &out) {
  auto sv = bela::SplitPath(path);
  out.reserve(path.size());
  if (sv.empty()) {
    return false;
  }
  out.assign(sv[0]);
  for (size_t i = 1; i < sv.size(); i++) {
    out.append(L"\\").append(sv[i]);
  }
  return true;
}

inline std::wstring baulk_unicode_string_create(std::string_view sv,
                                                int encoding) {
  if (encoding == CP_UTF8) {
    return bela::ToWide(sv);
  }
  auto sz =
      MultiByteToWideChar(encoding, 0, sv.data(), (int)sv.size(), nullptr, 0);
  std::wstring output;
  output.resize(sz);
  // C++17 must output.data()
  MultiByteToWideChar(encoding, 0, sv.data(), (int)sv.size(), output.data(),
                      sz);
  return output;
}

int32_t baulk_stream_os_open(void *stream, const wchar_t *path, int32_t mode) {
  mz_stream_win32 *win32 = (mz_stream_win32 *)stream;
  uint32_t desired_access = 0;
  uint32_t creation_disposition = 0;
  uint32_t share_mode = FILE_SHARE_READ;
  uint32_t flags_attribs = FILE_ATTRIBUTE_NORMAL;

  /* Some use cases require write sharing as well */
  share_mode |= FILE_SHARE_WRITE;

  if ((mode & MZ_OPEN_MODE_READWRITE) == MZ_OPEN_MODE_READ) {
    desired_access = GENERIC_READ;
    creation_disposition = OPEN_EXISTING;
  } else if (mode & MZ_OPEN_MODE_APPEND) {
    desired_access = GENERIC_WRITE | GENERIC_READ;
    creation_disposition = OPEN_EXISTING;
  } else if (mode & MZ_OPEN_MODE_CREATE) {
    desired_access = GENERIC_WRITE | GENERIC_READ;
    creation_disposition = CREATE_ALWAYS;
  } else {
    return MZ_PARAM_ERROR;
  }

  win32->handle = CreateFileW(path, desired_access, share_mode, NULL,
                              creation_disposition, flags_attribs, NULL);

  if (mz_stream_os_is_open(stream) != MZ_OK) {
    win32->error = GetLastError();
    return MZ_OPEN_ERROR;
  }

  if (mode & MZ_OPEN_MODE_APPEND) {
    return mz_stream_os_seek(stream, 0, MZ_SEEK_END);
  }

  return MZ_OK;
}

inline void mz_os_unix_to_file_time(time_t unix_time, FILETIME *file_time) {
  uint64_t quad_file_time = 0;
  quad_file_time = ((uint64_t)unix_time * 10000000) + 116444736000000000LL;
  file_time->dwHighDateTime = (quad_file_time >> 32);
  file_time->dwLowDateTime = (uint32_t)(quad_file_time);
}

int32_t baulk_os_set_file_date(const wchar_t *path, time_t modified_date,
                               time_t accessed_date, time_t creation_date) {
  HANDLE handle = NULL;
  FILETIME ftm_creation, ftm_accessed, ftm_modified;
  int32_t err = MZ_OK;
  handle = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, 0, NULL);
  if (handle != INVALID_HANDLE_VALUE) {
    GetFileTime(handle, &ftm_creation, &ftm_accessed, &ftm_modified);
    if (modified_date != 0) {
      mz_os_unix_to_file_time(modified_date, &ftm_modified);
    }
    if (accessed_date != 0) {
      mz_os_unix_to_file_time(accessed_date, &ftm_accessed);
    }
    if (creation_date != 0) {
      mz_os_unix_to_file_time(creation_date, &ftm_creation);
    }

    if (SetFileTime(handle, &ftm_creation, &ftm_accessed, &ftm_modified) == 0) {
      err = MZ_INTERNAL_ERROR;
    }
    CloseHandle(handle);
  }
  return err;
}

inline int32_t baulk_os_get_file_attribs(const wchar_t *path,
                                         uint32_t *attributes) {
  *attributes = GetFileAttributesW(path);
  if (*attributes == INVALID_FILE_ATTRIBUTES) {
    return MZ_INTERNAL_ERROR;
  }
  return MZ_OK;
}

inline int32_t baulk_os_set_file_attribs(const wchar_t *path,
                                         uint32_t attributes) {
  if (SetFileAttributesW(path, attributes) != TRUE) {
    return MZ_INTERNAL_ERROR;
  }
  return MZ_OK;
}

// todo
int32_t baulk_zip_reader_open_stream(void *handle, HANDLE FileHandle) {
  mz_zip_reader *reader = (mz_zip_reader *)handle;
  int32_t err = MZ_OK;

  mz_zip_reader_close(handle);

  mz_stream_os_create(&reader->file_stream);
  mz_stream_buffered_create(&reader->buffered_stream);
  mz_stream_split_create(&reader->split_stream);

  mz_stream_set_base(reader->buffered_stream, reader->file_stream);
  mz_stream_set_base(reader->split_stream, reader->buffered_stream);
  mz_stream_win32 *win32 = (mz_stream_win32 *)reader->split_stream;
  win32->handle = FileHandle;
  return mz_zip_reader_open(handle, reader->split_stream);
}

// callback
int baulk_zip_reader_entry_save_file(void *handle, std::wstring_view path) {
  std::filesystem::path path_(path);
  auto *reader = reinterpret_cast<mz_zip_reader *>(handle);
  std::error_code e;
  /* If it is a directory entry then create a directory instead of writing file
   */

  // if (reader->entry_cb != NULL)
  //     reader->entry_cb(handle, reader->entry_userdata, reader->file_info,
  //     pathwfs);
  if ((mz_zip_entry_is_dir(reader->zip_handle) == MZ_OK) &&
      (mz_zip_entry_is_symlink(reader->zip_handle) != MZ_OK)) {
    // err = mz_dir_make(directory);
    if (!std::filesystem::create_directories(path_, e)) {
      return MZ_INTERNAL_ERROR;
    }
    return MZ_OK;
  }
  if (std::filesystem::exists(path_, e)) {
    std::filesystem::remove_all(path_, e);
  }
  auto parent = path_.parent_path();
  if (!std::filesystem::is_directory(parent, e) &&
      !std::filesystem::create_directories(parent, e)) {
    return MZ_INTERNAL_ERROR;
  }
  // /* Check if file exists and ask if we want to overwrite */
  // if ((mz_os_file_exists(pathwfs) == MZ_OK) && (reader->overwrite_cb !=
  // NULL))
  // {
  //     err_cb = reader->overwrite_cb(handle, reader->overwrite_userdata,
  //     reader->file_info, pathwfs); if (err_cb != MZ_OK)
  //         return err;
  //     /* We want to overwrite the file so we delete the existing one */
  //     mz_os_unlink(pathwfs);
  // }

  /* If it is a symbolic link then create symbolic link instead of writing file
   */
  if (mz_zip_entry_is_symlink(reader->zip_handle) == MZ_OK) {
    auto linkname = bela::ToWide(reader->file_info->linkname);
    std::filesystem::path src(linkname);
    if (src.is_relative()) {
      src = parent / src;
    }
    std::filesystem::create_symlink(src, path_, e);
    return MZ_OK;
  }
  void *stream = nullptr;
  int err = MZ_OK;
  uint32_t target_attrib = 0;
  int32_t err_attrib = 0;
  mz_stream_os_create(&stream);
  if (baulk_stream_os_open(stream, path.data(), MZ_OPEN_MODE_CREATE) == MZ_OK) {
    err = mz_zip_reader_entry_save(handle, stream, mz_stream_write);
  }

  mz_stream_close(stream);
  mz_stream_delete(&stream);
  if (err == MZ_OK) {
    /* Set the time of the file that has been created */
    baulk_os_set_file_date(path.data(), reader->file_info->modified_date,
                           reader->file_info->accessed_date,
                           reader->file_info->creation_date);
    /* Set file attributes for the correct system */
    if (mz_zip_attrib_convert(MZ_HOST_SYSTEM(reader->file_info->version_madeby),
                              reader->file_info->external_fa,
                              MZ_VERSION_MADEBY_HOST_SYSTEM,
                              &target_attrib) == MZ_OK) {
      baulk_os_set_file_attribs(path.data(), target_attrib);
    }
  }
  return err;
}

int baulk_zip_reader_save_all(void *handle, std::wstring_view dest) {
  auto err = mz_zip_reader_goto_first_entry(handle);
  if (err != MZ_END_OF_LIST) {
    return err;
  }
  auto path = bela::PathAbsolute(dest);
  auto pathlen = path.size();
  std::wstring subfile;
  auto PathAppend = [&](std::wstring_view sub) {
    path.resize(pathlen);
    if (PathResolve(sub, subfile)) {
      return false;
    }
    bela::StrAppend(&path, L"\\", subfile);
    return true;
  };
  auto *reader = reinterpret_cast<mz_zip_reader *>(handle);
  path.reserve(4096);
  while (err == MZ_OK) {
    auto filename = std::string_view(reader->file_info->filename,
                                     reader->file_info->filename_size);
    if ((reader->encoding > 0) &&
        (reader->file_info->flag & MZ_ZIP_FLAG_UTF8) == 0) {
      if (!PathAppend(
              baulk_unicode_string_create(filename, reader->encoding))) {
        err = MZ_INTERNAL_ERROR;
        break;
      }
    } else {
      if (!PathAppend(bela::ToWide(filename))) {
        err = MZ_INTERNAL_ERROR;
        break;
      }
    }
    /* Save file to disk */
    err = baulk_zip_reader_entry_save_file(handle, path);
    if (err == MZ_OK) {
      err = mz_zip_reader_goto_next_entry(handle);
    }
  }
  if (err == MZ_END_OF_LIST) {
    return MZ_OK;
  }
  return 0;
}

bool DecompressFile(HANDLE &FileHandle, std::wstring_view dest,
                    bela::error_code &ec) {
  if (FileHandle == INVALID_HANDLE_VALUE) {
    return false;
  }
  LARGE_INTEGER pos{0};
  LARGE_INTEGER newPos;
  if (SetFilePointerEx(FileHandle, pos, &newPos, FILE_BEGIN) != TRUE) {
    ec = bela::make_system_error_code();
    CloseHandle(FileHandle);
    return false;
  }
  void *reader = nullptr;
  mz_zip_create(&reader);
  mz_zip_reader_create(&reader);
  auto closer = bela::finally([&] {
    if (mz_zip_reader_close(reader) != MZ_OK) {
      /// trace
    }
    mz_zip_delete(&reader);
    FileHandle = INVALID_HANDLE_VALUE;
  });
  auto err = baulk_zip_reader_open_stream(reader, FileHandle);
  if (err != MZ_OK) {
    CloseHandle(FileHandle);
    return false;
  }
  return baulk_zip_reader_save_all(reader, dest) == MZ_OK;
}

} // namespace baulk::archive::zip