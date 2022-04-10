//
#include <hazel/fs.hpp>
#include <bela/repasepoint.hpp>
#include <bela/str_cat.hpp>

namespace hazel::fs {

struct resparse_point_tagname_t {
  reparse_point_t t;
  const wchar_t *tagName;
};

#define DEFINED_NAME_RESP(X)                                                                                           \
  { static_cast<reparse_point_t>(X), L#X }

// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/c8e77b37-3909-4fe6-a4ea-2b9d423b1ee4

const wchar_t *lookup_reparse_tagname(reparse_point_t t) {

  constexpr resparse_point_tagname_t tagnames[] = {
      DEFINED_NAME_RESP(IO_REPARSE_TAG_MOUNT_POINT),   DEFINED_NAME_RESP(IO_REPARSE_TAG_HSM),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_HSM2),          DEFINED_NAME_RESP(IO_REPARSE_TAG_SIS),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_WIM),           DEFINED_NAME_RESP(IO_REPARSE_TAG_CSV),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_DFS),           DEFINED_NAME_RESP(IO_REPARSE_TAG_SYMLINK),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_DFSR),          DEFINED_NAME_RESP(IO_REPARSE_TAG_DEDUP),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_NFS),           DEFINED_NAME_RESP(IO_REPARSE_TAG_FILE_PLACEHOLDER),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_WOF),           DEFINED_NAME_RESP(IO_REPARSE_TAG_WCI),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_WCI_1),         DEFINED_NAME_RESP(IO_REPARSE_TAG_GLOBAL_REPARSE),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD),         DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_1),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_2),       DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_3),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_4),       DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_5),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_6),       DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_7),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_8),       DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_9),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_A),       DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_B),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_C),       DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_D),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_E),       DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_F),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_CLOUD_MASK),    DEFINED_NAME_RESP(IO_REPARSE_TAG_APPEXECLINK),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_PROJFS),        DEFINED_NAME_RESP(IO_REPARSE_TAG_STORAGE_SYNC),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_WCI_TOMBSTONE), DEFINED_NAME_RESP(IO_REPARSE_TAG_UNHANDLED),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_ONEDRIVE),      DEFINED_NAME_RESP(IO_REPARSE_TAG_PROJFS_TOMBSTONE),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_AF_UNIX),       DEFINED_NAME_RESP(IO_REPARSE_TAG_WCI_LINK),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_WCI_LINK_1),    DEFINED_NAME_RESP(IO_REPARSE_TAG_DATALESS_CIM),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_LX_FIFO),       DEFINED_NAME_RESP(IO_REPARSE_TAG_LX_CHR),
      DEFINED_NAME_RESP(IO_REPARSE_TAG_LX_BLK),
      // text index end
  };
  //
  for (const auto &m : tagnames) {
    if (m.t == t) {
      return m.tagName;
    }
  }
  return L"IO_REPARSE_TAG_UNKNOWN";
} // namespace hazel::fs

// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/b41f1cbf-10df-4a47-98d4-1c52a833d913
inline bool DecodeSymbolicLink(const REPARSE_DATA_BUFFER *buffer, FileReparsePoint &frp, bela::error_code &ec) {
  auto wstr = buffer->SymbolicLinkReparseBuffer.PathBuffer +
              (buffer->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR));
  auto wlen = buffer->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
  if (wlen >= MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {
    ec = bela::make_error_code(bela::ErrGeneral, L"symbolic link: SubstituteNameLength ",
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
    ec = bela::make_error_code(bela::ErrGeneral, L"Unresolved reparse point MountPoint'");
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
    ec = bela::make_error_code(bela::ErrGeneral, L"Unresolved reparse point AppExecLink. StringCount=",
                               p->AppExecLinkReparseBuffer.StringCount);
    return false;
  }
  auto szString = (LPWSTR)p->AppExecLinkReparseBuffer.StringList;
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
  auto p = b.as_bytes_view().unchecked_cast<REPARSE_DATA_BUFFER>();
  frp.type = static_cast<reparse_point_t>(p->ReparseTag);
  frp.attributes.emplace(L"TAG", bela::StringCat(L"0x", bela::Hex(frp.type, bela::kZeroPad8)));
  frp.attributes.emplace(L"TagName", lookup_reparse_tagname(frp.type));
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