////
// super reparse pointer resolve
#include <bela/path.hpp>
#include <bela/repasepoint.hpp>
#include <bela/strcat.hpp>
#include <bela/match.hpp>
#include <winioctl.h>
#include "reparsepoint_internal.hpp"

namespace bela {
using facv_t = std::vector<FileAttributePair>;
using ReparseBuffer = REPARSE_DATA_BUFFER;

struct FileReparser {
  FileReparser() = default;
  FileReparser(const FileReparser &) = delete;
  FileReparser &operator=(const FileReparser &) = delete;
  ~FileReparser() {
    if (FileHandle != INVALID_HANDLE_VALUE) {
      CloseHandle(FileHandle);
    }
    if (buffer != nullptr) {
      HeapFree(GetProcessHeap(), 0, buffer);
    }
  }
  REPARSE_DATA_BUFFER *buffer{nullptr};
  HANDLE FileHandle{INVALID_HANDLE_VALUE};
  DWORD len{0};
  bool FileDeviceLookup(std::wstring_view file, bela::error_code &ec);
};

bool FileReparser::FileDeviceLookup(std::wstring_view file, bela::error_code &ec) {
  if (buffer = reinterpret_cast<REPARSE_DATA_BUFFER *>(
          HeapAlloc(GetProcessHeap(), 0, MAXIMUM_REPARSE_DATA_BUFFER_SIZE));
      buffer == nullptr) {
    ec = bela::make_system_error_code();
    return false;
  }
  if (FileHandle = CreateFileW(
          file.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
          OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
      FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  if (DeviceIoControl(FileHandle, FSCTL_GET_REPARSE_POINT, nullptr, 0, buffer,
                      MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &len, nullptr) != TRUE) {
    if (ec.code = GetLastError(); ec.code != ERROR_NOT_A_REPARSE_POINT) {
      ec.message = bela::resolve_system_error_message(ec.code);
    }
    return false;
  }
  return true;
}

inline bool DecodeAppLink(const REPARSE_DATA_BUFFER *buffer, AppExecTarget &target) {
  LPWSTR szString = (LPWSTR)buffer->AppExecLinkReparseBuffer.StringList;
  std::vector<std::wstring_view> strv;
  for (ULONG i = 0; i < buffer->AppExecLinkReparseBuffer.StringCount; i++) {
    auto len = wcslen(szString);
    strv.emplace_back(szString, len);
    szString += len + 1;
  }
  if (strv.empty()) {
    return false;
  }
  target.pkid = strv[0];
  if (strv.size() > 1) {
    target.appuserid = strv[1];
  }
  if (strv.size() > 2) {
    target.target = strv[2];
  }
  return true;
}

inline bool DecodeSymbolicLink(const REPARSE_DATA_BUFFER *buffer, std::wstring &target) {

  auto wstr = buffer->SymbolicLinkReparseBuffer.PathBuffer +
              (buffer->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR));
  auto wlen = buffer->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
  if (wlen >= MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {
    return false;
  }
  if (wlen >= 4 && wstr[0] == L'\\' && wstr[1] == L'?' && wstr[2] == L'?' && wstr[3] == L'\\') {
    /* Starts with \??\ */
    if (wlen >= 6 &&
        ((wstr[4] >= L'A' && wstr[4] <= L'Z') || (wstr[4] >= L'a' && wstr[4] <= L'z')) &&
        wstr[5] == L':' && (wlen == 6 || wstr[6] == L'\\')) {
      /* \??\<drive>:\ */
      wstr += 4;
      wlen -= 4;

    } else if (wlen >= 8 && (wstr[4] == L'U' || wstr[4] == L'u') &&
               (wstr[5] == L'N' || wstr[5] == L'n') && (wstr[6] == L'C' || wstr[6] == L'c') &&
               wstr[7] == L'\\') {
      /* \??\UNC\<server>\<share>\ - make sure the final path looks like */
      /* \\<server>\<share>\ */
      wstr += 7;
      target.push_back(L'\\');
      wlen -= 7;
    }
  }
  target.append(wstr, wlen);
  return true;
}

inline bool DecodeMountPoint(const REPARSE_DATA_BUFFER *buffer, std::wstring &target) {
  auto wstr = buffer->MountPointReparseBuffer.PathBuffer +
              (buffer->MountPointReparseBuffer.SubstituteNameOffset / sizeof(WCHAR));
  auto wlen = buffer->MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
  if (wlen >= MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {
    return false;
  }
  /* Only treat junctions that look like \??\<drive>:\ as symlink. */
  /* Junctions can also be used as mount points, like \??\Volume{<guid>}, */
  /* but that's confusing for programs since they wouldn't be able to */
  /* actually understand such a path when returned by uv_readlink(). */
  /* UNC paths are never valid for junctions so we don't care about them. */
  if (!(wlen >= 6 && wstr[0] == L'\\' && wstr[1] == L'?' && wstr[2] == L'?' && wstr[3] == L'\\' &&
        ((wstr[4] >= L'A' && wstr[4] <= L'Z') || (wstr[4] >= L'a' && wstr[4] <= L'z')) &&
        wstr[5] == L':' && (wlen == 6 || wstr[6] == L'\\'))) {
    return false;
  }

  /* Remove leading \??\ */
  wstr += 4;
  wlen -= 4;
  target.assign(wstr, wlen);
  return true;
}

struct file_reparser_handle {
  DWORD tag;
  bool (*imp)(const ReparseBuffer *, facv_t &, bela::error_code &);
};

static bool nt_symbolic_link(const ReparseBuffer *buf, facv_t &av, bela::error_code &ec) {
  (void)ec;
  std::wstring target;
  DecodeSymbolicLink(buf, target);
  av.emplace_back(L"Description", L"SymbolicLink");
  av.emplace_back(L"Target", target);
  return true;
}

static bool global_symbolic_link(const ReparseBuffer *buf, facv_t &av, bela::error_code &ec) {
  if (!nt_symbolic_link(buf, av, ec)) {
    return true;
  }
  av.emplace_back(L"IsGlobal", L"True");
  return true;
}

static bool app_exec_link(const ReparseBuffer *buf, facv_t &av, bela::error_code &ec) {
  if (buf->AppExecLinkReparseBuffer.StringCount < 3) {
    ec = bela::make_error_code(1, L"Unresolved reparse point AppExecLink. StringCount=",
                               buf->AppExecLinkReparseBuffer.StringCount);
    return false;
  }
  LPWSTR szString = (LPWSTR)buf->AppExecLinkReparseBuffer.StringList;
  av.emplace_back(L"Description", L"AppExecLink");
  /// push_back
  const std::wstring_view attrnames[] = {L"PackageID", L"AppUserID", L"Target"};
  for (ULONG i = 0; i < buf->AppExecLinkReparseBuffer.StringCount; i++) {
    auto szlen = wcslen(szString);
    if (i < 3) {
      av.emplace_back(attrnames[i], std::wstring_view(szString, szlen));
    }
    szString += szlen + 1;
  }
  return true;
}

static bool mount_point(const ReparseBuffer *buf, facv_t &av, bela::error_code &ec) {
  auto wstr = buf->MountPointReparseBuffer.PathBuffer +
              (buf->MountPointReparseBuffer.SubstituteNameOffset / sizeof(WCHAR));
  auto wlen = buf->MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
  /* Only treat junctions that look like \??\<drive>:\ as symlink. */
  /* Junctions can also be used as mount points, like \??\Volume{<guid>}, */
  /* but that's confusing for programs since they wouldn't be able to */
  /* actually understand such a path when returned by uv_readlink(). */
  /* UNC paths are never valid for junctions so we don't care about them. */
  if (!(wlen >= 6 && wstr[0] == L'\\' && wstr[1] == L'?' && wstr[2] == L'?' && wstr[3] == L'\\' &&
        ((wstr[4] >= L'A' && wstr[4] <= L'Z') || (wstr[4] >= L'a' && wstr[4] <= L'z')) &&
        wstr[5] == L':' && (wlen == 6 || wstr[6] == L'\\'))) {
    ec = bela::make_error_code(1, L"Unresolved reparse point MountPoint'");
    return false;
  }

  /* Remove leading \??\ */
  wstr += 4;
  wlen -= 4;
  av.emplace_back(L"Description", L"MountPoint");
  av.emplace_back(L"Target", std::wstring(wstr, wlen));
  return true;
}

static bool af_unix(const ReparseBuffer *buf, facv_t &av, bela::error_code &) {
  //
  (void)buf;
  av.emplace_back(L"Description", L"AF_UNIX");
  return true;
}

static bool wsl_symbolic_link(const ReparseBuffer *buf, facv_t &av, bela::error_code &) {
  (void)buf;
  // Not implemented
  av.emplace_back(L"Description", L"LxSymbolicLink");
  return true;
}

// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/c8e77b37-3909-4fe6-a4ea-2b9d423b1ee4

bool ReparsePoint::Analyze(std::wstring_view file, bela::error_code &ec) {
  static const constexpr file_reparser_handle av[] = {
      {IO_REPARSE_TAG_APPEXECLINK, app_exec_link}, // AppExecLink
      {IO_REPARSE_TAG_SYMLINK, nt_symbolic_link},
      {IO_REPARSE_TAG_GLOBAL_REPARSE, global_symbolic_link},
      {IO_REPARSE_TAG_MOUNT_POINT, mount_point},
      {IO_REPARSE_TAG_AF_UNIX, af_unix},
      {IO_REPARSE_TAG_LX_SYMLINK, wsl_symbolic_link}
      ///  WSL1 symlink is a file. WSL2???
      // end value
  };
  FileReparser reparser;
  if (!reparser.FileDeviceLookup(file, ec)) {
    return false;
  }
  // bind tagvalue
  tagvalue = reparser.buffer->ReparseTag;
  for (auto a : av) {
    if (a.tag == tagvalue) {
      return a.imp(reparser.buffer, values, ec);
    }
  }
  values.emplace_back(L"TAG", bela::StringCat(L"0x", bela::Hex(tagvalue, bela::kZeroPad8)));
  values.emplace_back(L"Description", L"UNKNOWN");
  return true;
}

bool LookupAppExecLinkTarget(std::wstring_view src, AppExecTarget &target) {
  FileReparser reparser;
  bela::error_code ec;
  if (!reparser.FileDeviceLookup(src, ec)) {
    return false;
  }
  if (reparser.buffer->ReparseTag != IO_REPARSE_TAG_APPEXECLINK) {
    return false;
  }
  return DecodeAppLink(reparser.buffer, target);
}

std::optional<std::wstring> RealPathEx(std::wstring_view src, bela::error_code &ec) {
  FileReparser reparser;
  if (!reparser.FileDeviceLookup(src, ec)) {
    if (ec.code == ERROR_NOT_A_REPARSE_POINT) {
      ec.code = 0;
      return std::make_optional(bela::PathAbsolute(src));
    }
    return std::nullopt;
  }
  switch (reparser.buffer->ReparseTag) {
  case IO_REPARSE_TAG_APPEXECLINK:
    if (AppExecTarget target; DecodeAppLink(reparser.buffer, target)) {
      return std::make_optional(std::move(target.target));
    }
    ec = bela::make_error_code(1, L"BAD: unable decode AppLinkExec");
    return std::nullopt;
  case IO_REPARSE_TAG_SYMLINK:
    CloseHandle(reparser.FileHandle);
    reparser.FileHandle = INVALID_HANDLE_VALUE;
    if (auto target = bela::RealPath(src, ec); target) {
      return std::make_optional(std::move(*target));
    }
    return std::nullopt;
  case IO_REPARSE_TAG_GLOBAL_REPARSE:
    if (std::wstring target; DecodeSymbolicLink(reparser.buffer, target)) {
      return std::make_optional(std::move(target));
    }
    ec = bela::make_error_code(1, L"BAD: unable decode Global SymbolicLink");
    return std::nullopt;
  default:
    break;
  }
  return std::make_optional<std::wstring>(src);
}

} // namespace bela
