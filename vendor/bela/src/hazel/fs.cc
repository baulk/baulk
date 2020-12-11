//
#include <hazel/fs.hpp>
#include <bela/repasepoint.hpp>

namespace hazel::fs {

struct description_t {
  reparse_point_t t;
  const wchar_t *description;
};
// https://mediatemple.net/community/products/dv/204403964/mime-types
const wchar_t *lookup_reparse_description(reparse_point_t t) {

  constexpr const wchar_t *onedriverdesc =
      L"Used by the Cloud Files filter, for files managed by a sync engine such as "
      L"OneDrive. Server-side interpretation only, not meaningful over the wire.";
  constexpr const wchar_t *wcidesc =
      L"Used by the Windows Container Isolation filter. Server-side interpretation only, not meaningful over the wire.";
  constexpr description_t descriptions[] = {
      {MOUNT_POINT, L"Used for mount point support."},
      {HSM, L"Obsolete. Used by legacy Hierarchical Storage Manager Product."}, //
      {DRIVE_EXTENDER, L"Home server drive extender."},
      {HSM2, L"Obsolete. Used by legacy Hierarchical Storage Manager Product."},
      {SIS, L"Used by single-instance storage (SIS) filter driver. Server-side interpretation only, not meaningful "
            L"over the wire."},
      {WIM, L"Used by the WIM Mount filter. Server-side interpretation only, not meaningful over the wire."},
      {CSV, L"Obsolete. Used by Clustered Shared Volumes (CSV) version 1 in Windows Server 2008 R2 operating system. "
            L"Server-side interpretation only, not meaningful over the wire."},
      {DFS, L"Used by the DFS filter. The DFS is described in the Distributed File System (DFS): Referral Protocol "
            L"Specification [MS-DFSC]. Server-side interpretation only, not meaningful over the wire."},
      {FILTER_MANAGER, L"Used by filter manager test harness."},
      {SYMLINK, L"Used for symbolic link support. "},
      {IIS_CACHE, L"Used by Microsoft Internet Information Services (IIS) caching. Server-side interpretation only, "
                  L"not meaningful over the wire."},
      {DFSR, L"Used by the DFS filter. The DFS is described in [MS-DFSC]. Server-side interpretation only, not "
             L"meaningful over the wire."},
      {DEDUP, L"Used by the Data Deduplication (Dedup) filter. Server-side interpretation only, not meaningful over "
              L"the wire."},
      {APPXSTRM, L"Not used."},
      {NFS, L"Used by the Network File System (NFS) component. Server-side interpretation only, not meaningful over "
            L"the wire."},
      {FILE_PLACEHOLDER, L"Obsolete. Used by Windows Shell for legacy placeholder files in Windows 8.1. Server-side "
                         L"interpretation only, not meaningful over the wire."},
      {DFM, L"Used by the Dynamic File filter. Server-side interpretation only, not meaningful over the wire."},
      {WOF, L"Used by the Windows Overlay filter, for either WIMBoot or single-file compression. Server-side "
            L"interpretation only, not meaningful over the wire."},
      {WCI, wcidesc},
      {WCI_1, wcidesc},
      {GLOBAL_REPARSE, L"Used by NPFS to indicate a named pipe symbolic link from a server silo into the host silo. "
                       L"Server-side interpretation only, not meaningful over the wire."},
      {CLOUD, L"Used by the Cloud Files filter, for files managed by a sync engine such as Microsoft OneDrive. "
              L"Server-side interpretation only, not meaningful over the wire."},
      {CLOUD_1, L"Used by the Cloud Files filter, for files managed by a sync engine such as OneDrive. Server-side "
                L"interpretation only, not meaningful over the wire."},
      {CLOUD_2, onedriverdesc},
      {CLOUD_3, onedriverdesc},
      {CLOUD_4, onedriverdesc},
      {CLOUD_5, onedriverdesc},
      {CLOUD_6, onedriverdesc},
      {CLOUD_7, onedriverdesc}, //
      {CLOUD_8, onedriverdesc},
      {CLOUD_9, onedriverdesc},
      {CLOUD_A, onedriverdesc},
      {CLOUD_B, onedriverdesc},
      {CLOUD_C, onedriverdesc},
      {CLOUD_D, onedriverdesc},
      {CLOUD_E, onedriverdesc},
      {CLOUD_F, onedriverdesc},
      {APPEXECLINK,
       L"Used by Universal Windows Platform (UWP) packages to encode information that allows the application to be "
       L"launched by CreateProcess. Server-side interpretation only, not meaningful over the wire."},
      {PROJFS, L"Used by the Windows Projected File System filter, for files managed by a user mode provider such as "
               L"VFS for Git. Server-side interpretation only, not meaningful over the wire."},
      {LX_SYMLINK, L"Used by the Windows Subsystem for Linux (WSL) to represent a UNIX symbolic link. Server-side "
                   L"interpretation only, not meaningful over the wire."},
      {STORAGE_SYNC,
       L"Used by the Azure File Sync (AFS) filter. Server-side interpretation only, not meaningful over the wire."},
      {WCI_TOMBSTONE, L"Used by the Windows Container Isolation filter. Server-side interpretation only, not "
                      L"meaningful over the wire."},
      {UNHANDLED, L"Used by the Windows Container Isolation filter. Server-side interpretation only, not meaningful "
                  L"over the wire."},
      {ONEDRIVE, L"Not used."},
      {PROJFS_TOMBSTONE, L"Used by the Windows Projected File System filter, for files managed by a user mode provider "
                         L"such as VFS for Git. Server-side interpretation only, not meaningful over the wire."},
      {AF_UNIX, L"Used by the Windows Subsystem for Linux (WSL) to represent a UNIX domain socket. Server-side "
                L"interpretation only, not meaningful over the wire."},
      {LX_FIFO, L"Used by the Windows Subsystem for Linux (WSL) to represent a UNIX FIFO (named pipe). Server-side "
                L"interpretation only, not meaningful over the wire."},
      {LX_CHR, L"Used by the Windows Subsystem for Linux (WSL) to represent a UNIX character special file. Server-side "
               L"interpretation only, not meaningful over the wire."},
      {LX_BLK, L"Used by the Windows Subsystem for Linux (WSL) to represent a UNIX block special file. Server-side "
               L"interpretation only, not meaningful over the wire."},
      {WCI_LINK, wcidesc},
      {WCI_LINK_1, wcidesc},
      // text index end

  };
  //
  for (const auto &m : descriptions) {
    if (m.t == t) {
      return m.description;
    }
  }
  return L"UNKNOWN";
}

// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/b41f1cbf-10df-4a47-98d4-1c52a833d913
inline bool DecodeSymbolicLink(const REPARSE_DATA_BUFFER *buffer, FileReparsePoint &frp, bela::error_code &ec) {
  auto wstr = buffer->SymbolicLinkReparseBuffer.PathBuffer +
              (buffer->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR));
  auto wlen = buffer->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
  if (wlen >= MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {
    ec = bela::make_error_code(1, L"symbolic link: SubstituteNameLength ",
                               buffer->SymbolicLinkReparseBuffer.SubstituteNameLength, L" > ",
                               MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    return false;
  }
  std::wstring target;
  if (wlen >= 4 && wstr[0] == L'\\' && wstr[1] == L'?' && wstr[2] == L'?' && wstr[3] == L'\\') {
    /* Starts with \??\ */
    if (wlen >= 6 && ((wstr[4] >= L'A' && wstr[4] <= L'Z') || (wstr[4] >= L'a' && wstr[4] <= L'z')) &&
        wstr[5] == L':' && (wlen == 6 || wstr[6] == L'\\')) {
      /* \??\<drive>:\ */
      wstr += 4;
      wlen -= 4;

    } else if (wlen >= 8 && (wstr[4] == L'U' || wstr[4] == L'u') && (wstr[5] == L'N' || wstr[5] == L'n') &&
               (wstr[6] == L'C' || wstr[6] == L'c') && wstr[7] == L'\\') {
      /* \??\UNC\<server>\<share>\ - make sure the final path looks like */
      /* \\<server>\<share>\ */
      wstr += 7;
      target.push_back(L'\\');
      wlen -= 7;
    }
  }
  target.append(wstr, wlen);
  frp.attributes.emplace(L"Target", std::move(target));
  return true;
}

// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/ca069dad-ed16-42aa-b057-b6b207f447cc
static bool DecodeMountPoint(const REPARSE_DATA_BUFFER *p, FileReparsePoint &frp, bela::error_code &ec) {
  auto wstr = p->MountPointReparseBuffer.PathBuffer + (p->MountPointReparseBuffer.SubstituteNameOffset / sizeof(WCHAR));
  auto wlen = p->MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
  /* Only treat junctions that look like \??\<drive>:\ as symlink. */
  /* Junctions can also be used as mount points, like \??\Volume{<guid>}, */
  /* but that's confusing for programs since they wouldn't be able to */
  /* actually understand such a path when returned by uv_readlink(). */
  /* UNC paths are never valid for junctions so we don't care about them. */
  if (!(wlen >= 6 && wstr[0] == L'\\' && wstr[1] == L'?' && wstr[2] == L'?' && wstr[3] == L'\\' &&
        ((wstr[4] >= L'A' && wstr[4] <= L'Z') || (wstr[4] >= L'a' && wstr[4] <= L'z')) && wstr[5] == L':' &&
        (wlen == 6 || wstr[6] == L'\\'))) {
    ec = bela::make_error_code(1, L"Unresolved reparse point MountPoint'");
    return false;
  }

  /* Remove leading \??\ */
  wstr += 4;
  wlen -= 4;
  frp.attributes.emplace(L"Target", std::wstring_view(wstr, wlen));
  return true;
}

static bool DecodeAppTargetLink(const REPARSE_DATA_BUFFER *p, FileReparsePoint &frp, bela::error_code &ec) {
  if (p->AppExecLinkReparseBuffer.StringCount < 3) {
    ec = bela::make_error_code(1, L"Unresolved reparse point AppExecLink. StringCount=",
                               p->AppExecLinkReparseBuffer.StringCount);
    return false;
  }
  LPWSTR szString = (LPWSTR)p->AppExecLinkReparseBuffer.StringList;
  /// push_back
  const std::wstring_view attrnames[] = {L"PackageID", L"AppUserID", L"Target"};
  for (ULONG i = 0; i < p->AppExecLinkReparseBuffer.StringCount; i++) {
    auto szlen = wcslen(szString);
    if (i < 3) {
      frp.attributes.emplace(attrnames[i], std::wstring_view(szString, szlen));
    }
    szString += szlen + 1;
  }
  return true;
}

bool LookupReparsePoint(std::wstring_view file, FileReparsePoint &frp, bela::error_code &ec) {
  bela::Buffer b(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
  if (!bela::LookupReparsePoint(file, b, ec)) {
    return false;
  }
  if (b.size() == 0) {
    ec.code = ERROR_NOT_A_REPARSE_POINT;
    ec.message = bela::resolve_system_error_message(ERROR_NOT_A_REPARSE_POINT);
    return false;
  }
  auto p = b.cast<REPARSE_DATA_BUFFER>();
  frp.type = static_cast<reparse_point_t>(p->ReparseTag);
  frp.attributes.emplace(L"TAG", bela::StringCat(L"0x", bela::Hex(frp.type, bela::kZeroPad8)));
  frp.attributes.emplace(L"Description", lookup_reparse_description(frp.type));
  switch (frp.type) {
  case SYMLINK:
    return DecodeSymbolicLink(p, frp, ec);
  case MOUNT_POINT:
    return DecodeMountPoint(p, frp, ec);
  case APPEXECLINK:
    return DecodeAppTargetLink(p, frp, ec);
  case GLOBAL_REPARSE:
    frp.attributes.emplace(L"IsGlobal", L"True");
    return DecodeSymbolicLink(p, frp, ec);
  case LX_SYMLINK:
    break;
  default:
    break;
  }
  return true;
}
} // namespace hazel::fs