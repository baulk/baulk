////
// super reparse pointer resolve
#include <bela/path.hpp>
#include <bela/repasepoint.hpp>
#include <bela/strcat.hpp>
#include <winioctl.h>
#include "reparsepoint_internal.hpp"

namespace bela {
using facv_t = std::vector<FileAttributePair>;
using ReparseBuffer = REPARSE_DATA_BUFFER;

struct Distributor {
  DWORD tag;
  bool (*imp)(const ReparseBuffer *, facv_t &, bela::error_code &);
};

static bool NtSymbolicLink(const ReparseBuffer *buf, facv_t &av,
                           bela::error_code &ec) {
  std::wstring target;
  auto wstr =
      buf->SymbolicLinkReparseBuffer.PathBuffer +
      (buf->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR));
  auto wlen =
      buf->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
  if (wlen >= 4 && wstr[0] == L'\\' && wstr[1] == L'?' && wstr[2] == L'?' &&
      wstr[3] == L'\\') {
    /* Starts with \??\ */
    if (wlen >= 6 &&
        ((wstr[4] >= L'A' && wstr[4] <= L'Z') ||
         (wstr[4] >= L'a' && wstr[4] <= L'z')) &&
        wstr[5] == L':' && (wlen == 6 || wstr[6] == L'\\')) {
      /* \??\<drive>:\ */
      wstr += 4;
      wlen -= 4;

    } else if (wlen >= 8 && (wstr[4] == L'U' || wstr[4] == L'u') &&
               (wstr[5] == L'N' || wstr[5] == L'n') &&
               (wstr[6] == L'C' || wstr[6] == L'c') && wstr[7] == L'\\') {
      /* \??\UNC\<server>\<share>\ - make sure the final path looks like */
      /* \\<server>\<share>\ */
      wstr += 7;
      target.push_back(L'\\');
      wlen -= 7;
    }
  }
  target.append(wstr, wlen);
  av.emplace_back(L"Description", L"SymbolicLink");
  av.emplace_back(L"Target", target);
  return true;
}

static bool GlobalSymbolicLink(const ReparseBuffer *buf, facv_t &av,
                               bela::error_code &ec) {
  if (!NtSymbolicLink(buf, av, ec)) {
    return true;
  }
  av.emplace_back(L"IsGlobal", L"True");
  return true;
}

static bool AppExecLink(const ReparseBuffer *buf, facv_t &av,
                        bela::error_code &ec) {
  if (buf->AppExecLinkReparseBuffer.StringCount < 3) {
    ec = bela::make_error_code(
        1, L"Unresolved reparse point AppExecLink. StringCount=",
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

static bool MountPoint(const ReparseBuffer *buf, facv_t &av,
                       bela::error_code &ec) {
  auto wstr =
      buf->MountPointReparseBuffer.PathBuffer +
      (buf->MountPointReparseBuffer.SubstituteNameOffset / sizeof(WCHAR));
  auto wlen = buf->MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
  /* Only treat junctions that look like \??\<drive>:\ as symlink. */
  /* Junctions can also be used as mount points, like \??\Volume{<guid>}, */
  /* but that's confusing for programs since they wouldn't be able to */
  /* actually understand such a path when returned by uv_readlink(). */
  /* UNC paths are never valid for junctions so we don't care about them. */
  if (!(wlen >= 6 && wstr[0] == L'\\' && wstr[1] == L'?' && wstr[2] == L'?' &&
        wstr[3] == L'\\' &&
        ((wstr[4] >= L'A' && wstr[4] <= L'Z') ||
         (wstr[4] >= L'a' && wstr[4] <= L'z')) &&
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

static bool AFUnix(const ReparseBuffer *buf, facv_t &av, bela::error_code &ec) {
  //
  av.emplace_back(L"Description", L"AF_UNIX");
  return true;
}

static bool LxSymbolicLink(const ReparseBuffer *buf, facv_t &av,
                           bela::error_code &ec) {
  // Not implemented
  av.emplace_back(L"Description", L"LxSymbolicLink");
  return true;
}

bool ReparsePoint::Analyze(std::wstring_view file, bela::error_code &ec) {
  static const constexpr Distributor av[] = {
      {IO_REPARSE_TAG_APPEXECLINK, AppExecLink}, // AppExecLink
      {IO_REPARSE_TAG_SYMLINK, NtSymbolicLink},
      {IO_REPARSE_TAG_GLOBAL_REPARSE, GlobalSymbolicLink},
      {IO_REPARSE_TAG_MOUNT_POINT, MountPoint},
      {IO_REPARSE_TAG_AF_UNIX, AFUnix},
      {IO_REPARSE_TAG_LX_SYMLINK, LxSymbolicLink}
      ///  WSL1 symlink is a file. WSL2???
      // end value
  };
  HANDLE hFile = INVALID_HANDLE_VALUE;
  REPARSE_DATA_BUFFER *buf = nullptr;
  auto closer = bela::finally([&] {
    if (hFile != nullptr) {
      CloseHandle(hFile);
    }
    if (buf != nullptr) {
      HeapFree(GetProcessHeap(), 0, buf);
    }
  });
  buf = reinterpret_cast<REPARSE_DATA_BUFFER *>(
      HeapAlloc(GetProcessHeap(), 0, MAXIMUM_REPARSE_DATA_BUFFER_SIZE));
  if (buf == nullptr) {
    ec = bela::make_system_error_code();
    return false;
  }
  hFile = CreateFileW(
      file.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr, OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  DWORD dwBytes = 0;
  // FIXME some reparsepoint type (AF_UNX LX_SYMLINK... ) cannot resolve as
  // reparsepoint
  if (DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, nullptr, 0, buf,
                      MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwBytes,
                      nullptr) != TRUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  tagvalue = buf->ReparseTag;
  for (auto a : av) {
    if (a.tag == tagvalue) {
      return a.imp(buf, values, ec);
    }
  }
  values.emplace_back(
      L"TAG", bela::StringCat(L"0x", bela::Hex(tagvalue, bela::kZeroPad8)));
  values.emplace_back(L"Description", L"UNKNOWN");
  return true;
}

} // namespace bela
