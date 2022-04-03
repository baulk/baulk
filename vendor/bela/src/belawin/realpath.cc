//////////
#include <bela/base.hpp>
#include <bela/match.hpp>
#include <bela/repasepoint.hpp>
#include <bela/path.hpp>

namespace bela {
// Thanks MSVC STL filesystem
template <class Ty> [[nodiscard]] Ty Unaligned_load(const void *Ptr) { // load a _Ty from _Ptr
  static_assert(std::is_trivial_v<Ty>, "Unaligned loads require trivial types");
  Ty Tmp;
  std::memcpy(&Tmp, Ptr, sizeof(Tmp));
  return Tmp;
}

[[nodiscard]] inline bool Is_drive_prefix(const wchar_t *const First) {
  // test if first points to a prefix of the form X:
  // pre: first points to at least 2 wchar_t instances
  // pre: Little endian
  auto Value = Unaligned_load<unsigned int>(First);
  Value &= 0xFFFF'FFDFU; // transform lowercase drive letters into uppercase ones
  Value -= (static_cast<unsigned int>(L':') << (sizeof(wchar_t) * CHAR_BIT)) | L'A';
  return Value < 26;
}

inline bool Is_drive_prefix_with_slash_slash_question(const std::wstring_view text) {
  return text.size() > 6 && bela::StartsWith(text, LR"(\\?\)") && Is_drive_prefix(text.data() + 4);
}

std::optional<std::wstring> RealPathByHandle(HANDLE FileHandle, bela::error_code &ec) {
  std::wstring buffer;
  buffer.resize(260); // opt
  DWORD kind = VOLUME_NAME_DOS;
  for (;;) {
    const auto blen = buffer.size();
    auto len = GetFinalPathNameByHandleW(FileHandle, buffer.data(), static_cast<DWORD>(blen), kind);
    if (len == 0) {
      auto e = GetLastError();
      if (e == ERROR_PATH_NOT_FOUND && kind == VOLUME_NAME_DOS) {
        kind = VOLUME_NAME_NT;
        continue;
      }
      ec.code = e;
      ec.message = bela::resolve_system_error_message(ec.code);
      return std::nullopt;
    }
    buffer.resize(len);
    if (len < static_cast<DWORD>(blen)) {
      break;
    }
  }
  if (kind != VOLUME_NAME_DOS) {
    // result is in the NT namespace, so apply the DOS to NT namespace prefix
    constexpr std::wstring_view ntprefix = LR"(\\?\GLOBALROOT)";
    return bela::StringCat(ntprefix, buffer);
  }
  // '\\?\C:\Path'
  if (Is_drive_prefix_with_slash_slash_question(buffer)) {
    return std::make_optional(buffer.substr(4));
  }
  if (bela::StartsWithIgnoreCase(buffer, LR"(\\?\UNC\)")) {
    std::wstring_view sv(buffer);
    return std::make_optional(bela::StringCat(LR"(\\)", sv.substr(6)));
  }
  return std::make_optional(std::move(buffer));
}

std::optional<std::wstring> RealPath(std::wstring_view src, bela::error_code &ec) {
  auto FileHandle = CreateFileW(src.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  // FILE_FLAG_BACKUP_SEMANTICS open directory require
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  auto closer = bela::finally([&] { CloseHandle(FileHandle); });
  return bela::RealPathByHandle(FileHandle, ec);
}

std::optional<std::wstring> Executable(bela::error_code &ec) {
  std::wstring buffer;
  buffer.resize(MAX_PATH);
  for (;;) {
    const auto blen = buffer.size();
    auto n = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(blen));
    if (n == 0) {
      ec = bela::make_system_error_code();
      return std::nullopt;
    }
    buffer.resize(n);
    if (n < static_cast<DWORD>(blen)) {
      break;
    }
  }
  return std::make_optional(std::move(buffer));
}

std::optional<std::wstring> ExecutableFinalPath(bela::error_code &ec) {
  if (auto exe = bela::Executable(ec); exe) {
    return RealPath(*exe, ec);
  }
  return std::nullopt;
}

inline bool DecodeAppLink(const REPARSE_DATA_BUFFER *buffer, AppExecTarget &target) {
  auto szString = (LPWSTR)buffer->AppExecLinkReparseBuffer.StringList;
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
        ((wstr[4] >= L'A' && wstr[4] <= L'Z') || (wstr[4] >= L'a' && wstr[4] <= L'z')) && wstr[5] == L':' &&
        (wlen == 6 || wstr[6] == L'\\'))) {
    return false;
  }

  /* Remove leading \??\ */
  wstr += 4;
  wlen -= 4;
  target.assign(wstr, wlen);
  return true;
}

bool LookupAppExecLinkTarget(std::wstring_view src, AppExecTarget &target) {
  bela::error_code ec;
  bela::Buffer b(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
  if (!LookupReparsePoint(src, b, ec)) {
    return false;
  }
  if (b.size() == 0) {
    return false;
  }
  auto p = b.as_bytes_view().unchecked_cast<REPARSE_DATA_BUFFER>();
  if (p->ReparseTag != IO_REPARSE_TAG_APPEXECLINK) {
    return false;
  }
  return DecodeAppLink(p, target);
}

std::optional<std::wstring> RealPathEx(std::wstring_view src, bela::error_code &ec) {
  bela::Buffer b(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
  if (!LookupReparsePoint(src, b, ec)) {
    return std::nullopt;
  }

  if (b.size() == 0) {
    return std::make_optional(bela::PathAbsolute(src));
  }
  auto p = b.as_bytes_view().unchecked_cast<REPARSE_DATA_BUFFER>();

  switch (p->ReparseTag) {
  case IO_REPARSE_TAG_APPEXECLINK:
    if (AppExecTarget target; DecodeAppLink(p, target)) {
      return std::make_optional(std::move(target.target));
    }
    ec = bela::make_error_code(ErrGeneral, L"BAD: unable decode AppLinkExec");
    return std::nullopt;
  case IO_REPARSE_TAG_SYMLINK:
    if (auto target = bela::RealPath(src, ec); target) {
      return std::make_optional(std::move(*target));
    }
    return std::nullopt;
  case IO_REPARSE_TAG_GLOBAL_REPARSE:
    if (std::wstring target; DecodeSymbolicLink(p, target)) {
      return std::make_optional(std::move(target));
    }
    ec = bela::make_error_code(ErrGeneral, L"BAD: unable decode Global SymbolicLink");
    return std::nullopt;
  default:
    break;
  }
  return std::make_optional<std::wstring>(src);
}

} // namespace bela
