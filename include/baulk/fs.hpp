#ifndef BAULK_FS_HPP
#define BAULK_FS_HPP
#include <bela/base.hpp>
#include <bela/fs.hpp>
#include <optional>
#include <string_view>
#include <filesystem>

namespace baulk::fs {
bool IsExecutablePath(const std::filesystem::path &p);
std::optional<std::filesystem::path> FindExecutablePath(const std::filesystem::path &p);
// Flattened: find flatten child directories
std::optional<std::filesystem::path> Flattened(const std::filesystem::path &d);
// MakeFlattened: Flatten directories
bool MakeFlattened(const std::filesystem::path &d, bela::error_code &ec);

inline std::wstring FileName(std::wstring_view p) { return std::filesystem::path(p).filename().wstring(); }

inline bool MakeDirectories(const std::filesystem::path &path, bela::error_code &ec) {
  std::error_code e;
  if (std::filesystem::exists(path, e)) {
    return true;
  }
  if (std::filesystem::create_directories(path, e); e) {
    ec = bela::make_error_code_from_std(e);
    return false;
  }
  return true;
}

inline bool MakeParentDirectories(const std::filesystem::path &path, bela::error_code &ec) {
  std::error_code e;
  auto parent = path.parent_path();
  if (std::filesystem::exists(parent, e)) {
    return true;
  }
  if (std::filesystem::create_directories(parent, e); e) {
    ec = bela::make_error_code_from_std(e);
    return false;
  }
  return true;
}

inline bool SymLink(const std::filesystem::path &_To, const std::filesystem::path &_New_symlink, bela::error_code &ec) {
  std::error_code e;
  if (std::filesystem::create_symlink(_To, _New_symlink, e); e) {
    ec = bela::make_error_code_from_std(e);
    return false;
  }
  return true;
}

std::optional<std::filesystem::path> NewTempFolder(bela::error_code &ec);
} // namespace baulk::fs

#endif