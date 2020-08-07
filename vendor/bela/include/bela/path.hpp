///
#ifndef BELA_WIN_PATH_HPP
#define BELA_WIN_PATH_HPP
#pragma once
#include "strcat.hpp"
#include "match.hpp"
#include "span.hpp"
#include "base.hpp"

namespace bela {
// Windows Path base
constexpr const wchar_t PathSeparator = L'\\';
constexpr const wchar_t PathUnixSeparator = L'/';
constexpr const size_t PathMax = 0x8000;
inline constexpr bool IsPathSeparator(wchar_t c) {
  return c == PathSeparator || c == PathUnixSeparator;
}
// BaseName DirName - parse pathname components
std::wstring_view BaseName(std::wstring_view name);
// BaseName DirName - parse pathname components
std::wstring_view DirName(std::wstring_view path);

inline std::wstring_view Extension(std::wstring_view path) {
  for (auto i = static_cast<intptr_t>(path.size() - 1); i >= 0; i--) {
    if (bela::IsPathSeparator(path[i])) {
      break;
    }
    if (path[i] == '.') {
      return path.substr(i);
    }
  }
  return L"";
}

inline std::wstring_view ExtensionEx(std::wstring_view path) {
  constexpr std::wstring_view complexExtensions[] = {L".tar.gz",  L".tar.xz", L".tar.sz",
                                                     L".tar.bz2", L".tar.br", L".tar.zst"};
  for (const auto e : complexExtensions) {
    if (bela::EndsWithIgnoreCase(path, e)) {
      return e;
    }
  }
  return Extension(path);
}

std::vector<std::wstring_view> SplitPath(std::wstring_view sv);
void PathStripName(std::wstring &s);
std::wstring PathAbsolute(std::wstring_view p);
namespace path_internal {
std::wstring PathCatPieces(bela::Span<std::wstring_view> pieces);
std::wstring PathAbsoluteCatPieces(bela::Span<std::wstring_view> pieces);
} // namespace path_internal

[[nodiscard]] inline std::wstring PathAbsoluteCat(const AlphaNum &a) {
  std::wstring_view pv[] = {a.Piece()};
  return path_internal::PathAbsoluteCatPieces(pv);
}

[[nodiscard]] inline std::wstring PathAbsoluteCat(const AlphaNum &a, const AlphaNum &b) {
  std::wstring_view pv[] = {a.Piece(), b.Piece()};
  return path_internal::PathAbsoluteCatPieces(pv);
}

[[nodiscard]] inline std::wstring PathAbsoluteCat(const AlphaNum &a, const AlphaNum &b,
                                                  const AlphaNum &c) {
  std::wstring_view pv[] = {a.Piece(), b.Piece(), c.Piece()};
  return path_internal::PathAbsoluteCatPieces(pv);
}

[[nodiscard]] inline std::wstring PathAbsoluteCat(const AlphaNum &a, const AlphaNum &b,
                                                  const AlphaNum &c, const AlphaNum &d) {
  std::wstring_view pv[] = {a.Piece(), b.Piece(), c.Piece(), d.Piece()};
  return path_internal::PathAbsoluteCatPieces(pv);
}

[[nodiscard]] inline std::wstring PathCat(const AlphaNum &a) {
  std::wstring_view pv[] = {a.Piece()};
  return path_internal::PathCatPieces(pv);
}

[[nodiscard]] inline std::wstring PathCat(const AlphaNum &a, const AlphaNum &b) {
  std::wstring_view pv[] = {a.Piece(), b.Piece()};
  return path_internal::PathCatPieces(pv);
}

[[nodiscard]] inline std::wstring PathCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c) {
  std::wstring_view pv[] = {a.Piece(), b.Piece(), c.Piece()};
  return path_internal::PathCatPieces(pv);
}

[[nodiscard]] inline std::wstring PathCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
                                          const AlphaNum &d) {
  std::wstring_view pv[] = {a.Piece(), b.Piece(), c.Piece(), d.Piece()};
  return path_internal::PathCatPieces(pv);
}

// Support 5 or more arguments
template <typename... AV>
[[nodiscard]] inline std::wstring PathCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
                                          const AlphaNum &d, const AlphaNum &e,
                                          const AV &... args) {
  return path_internal::PathCatPieces({a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(),
                                       static_cast<const AlphaNum &>(args).Piece()...});
}

enum class FileAttribute : DWORD {
  None = 0, //
  Archive = 0x20,
  Compressed = 0x800,
  Device = 0x40,
  Dir = 0x10,
  Encrypted = 0x4000,
  Hidden = 0x2,
  IntegrityStream = 0x8000,
  Normal = 0x80,
  NotContentIndexed = 0x2000,
  NoScrubData = 0x20000,
  Offline = 0x1000,
  ReadOnly = 0x1,
  NoDataAccess = 0x400000,
  ReccallOnOpen = 0x40000,
  ReparsePoint = 0x400,
  SparseFile = 0x200,
  System = 0x4,
  Temporary = 0x100,
  Virtual = 0x10000
};

[[nodiscard]] constexpr FileAttribute operator&(FileAttribute L, FileAttribute R) noexcept {
  using I = std::underlying_type_t<FileAttribute>;
  return static_cast<FileAttribute>(static_cast<I>(L) & static_cast<I>(R));
}

[[nodiscard]] constexpr FileAttribute operator|(FileAttribute L, FileAttribute R) noexcept {
  using I = std::underlying_type_t<FileAttribute>;
  return static_cast<FileAttribute>(static_cast<I>(L) | static_cast<I>(R));
}

// std::wstring_view ::data() must Null-terminated string
[[nodiscard]] inline bool PathExists(std::wstring_view src,
                                     FileAttribute fa = FileAttribute::None) {
  // GetFileAttributesExW()
  auto a = GetFileAttributesW(src.data());
  if (a == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  return fa == FileAttribute::None ? true : ((static_cast<DWORD>(fa) & a) != 0);
}

[[nodiscard]] inline bool PathFileIsExists(std::wstring_view file) {
  auto at = GetFileAttributesW(file.data());
  return (INVALID_FILE_ATTRIBUTES != at && (at & FILE_ATTRIBUTE_DIRECTORY) == 0);
}
bool ExecutableExistsInPath(std::wstring_view cmd, std::wstring &exe);
bool ExecutableExistsInPath(std::wstring_view cmd, std::wstring &exe,
                            const std::vector<std::wstring> &paths);
// Use GetFinalPathNameByHandleW
std::optional<std::wstring> RealPathByHandle(HANDLE FileHandle, bela::error_code &ec);
std::optional<std::wstring> RealPath(std::wstring_view src, bela::error_code &ec);
// RealPathEx parse reparsepoint
std::optional<std::wstring> RealPathEx(std::wstring_view src, bela::error_code &ec);
struct AppExecTarget {
  std::wstring pkid;
  std::wstring appuserid;
  std::wstring target;
};
bool LookupAppExecLinkTarget(std::wstring_view src, AppExecTarget &target);
std::optional<std::wstring> Executable(bela::error_code &ec);
inline std::optional<std::wstring> ExecutableParent(bela::error_code &ec) {
  auto exe = bela::Executable(ec);
  if (!exe) {
    return std::nullopt;
  }
  bela::PathStripName(*exe);
  return exe;
}
std::optional<std::wstring> ExecutableFinalPath(bela::error_code &ec);
inline std::optional<std::wstring> ExecutableFinalPathParent(bela::error_code &ec) {
  auto exe = bela::ExecutableFinalPath(ec);
  if (!exe) {
    return std::nullopt;
  }
  bela::PathStripName(*exe);
  return exe;
}
} // namespace bela

#endif
