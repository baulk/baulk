///
#include <bela/path.hpp>
#include <bela/match.hpp>
#include "fs.hpp"

namespace baulk::fs {

std::optional<std::wstring> UniqueSubdirectory(std::wstring_view dir) {
  Finder finder;
  bela::error_code ec;
  if (!finder.First(dir, L"*", ec)) {
    return std::nullopt;
  }
  int count = 0;
  std::wstring subdir;
  do {
    if (finder.Ignore()) {
      continue;
    }
    if (!finder.IsDir()) {
      return std::nullopt;
    }
    if (subdir.empty()) {
      subdir = bela::StringCat(dir, L"\\", finder.Name());
    }
    count++;
  } while (finder.Next());

  if (count != 1) {
    return std::nullopt;
  }
  if (bela::EndsWithIgnoreCase(subdir, L"\\bin")) {
    return std::nullopt;
  }
  return std::make_optional(std::move(subdir));
}

bool IsExecutableSuffix(std::wstring_view name) {
  constexpr std::wstring_view suffixs[] = {L".exe", L".com", L".bat", L".cmd"};
  for (const auto s : suffixs) {
    if (bela::EndsWithIgnoreCase(name, s)) {
      return true;
    }
  }
  return false;
}

bool IsExecutablePath(std::wstring_view p) {
  bela::error_code ec;
  Finder finder;
  if (!finder.First(p, L"*", ec)) {
    return false;
  }
  do {
    if (finder.Ignore()) {
      continue;
    }
    if (finder.IsDir()) {
      continue;
    }
    if (IsExecutableSuffix(finder.Name())) {
      return true;
    }
  } while (finder.Next());
  return false;
}

// https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-deletefilew
// The following list identifies some tips for deleting, removing, or closing
// files:

// To delete a read-only file, first you must remove the read-only attribute.
// To delete or rename a file, you must have either delete permission on the
// file, or delete child permission in the parent directory. To recursively
// delete the files in a directory, use the SHFileOperation function. To remove
// an empty directory, use the RemoveDirectory function. To close an open file,
// use the CloseHandle function.

bool PathRemoveInternal(std::wstring_view path, bela::error_code &ec) {
  Finder finder;
  if (!finder.First(path, L"*", ec)) {
    return false;
  }
  do {
    if (finder.Ignore()) {
      continue;
    }
    auto child = bela::StringCat(path, L"\\", finder.Name());
    if (finder.IsDir()) {

      if (!PathRemoveInternal(child, ec)) {
        return false;
      }
      continue;
    }
    SetFileAttributesW(child.data(), GetFileAttributesW(child.data()) & ~FILE_ATTRIBUTE_READONLY);
    if (DeleteFileW(child.data()) != TRUE) {
      ec = bela::make_system_error_code(bela::StringCat(L"del '", path, L"': "));
      return false;
    }

  } while (finder.Next());
  SetFileAttributesW(path.data(), GetFileAttributesW(path.data()) & ~FILE_ATTRIBUTE_READONLY);
  if (RemoveDirectoryW(path.data()) != TRUE) {
    ec = bela::make_system_error_code(bela::StringCat(L"rmdir '", path, L"': "));
    return false;
  }
  return true;
}

bool PathRemoveEx(std::wstring_view path, bela::error_code &ec) {
  if ((GetFileAttributesW(path.data()) & FILE_ATTRIBUTE_DIRECTORY) == 0) {
    SetFileAttributesW(path.data(), GetFileAttributesW(path.data()) & ~FILE_ATTRIBUTE_READONLY);
    if (DeleteFileW(path.data()) != TRUE) {
      ec = bela::make_system_error_code(bela::StringCat(L"del '", path, L"': "));
      return false;
    }
    return true;
  }
  return PathRemoveInternal(path, ec);
}

std::optional<std::wstring> FindExecutablePath(std::wstring_view p) {
  if (!bela::PathExists(p, bela::FileAttribute::Dir)) {
    return std::nullopt;
  }
  if (IsExecutablePath(p)) {
    return std::make_optional<std::wstring>(p);
  }
  auto p2 = bela::StringCat(p, L"\\bin");
  if (IsExecutablePath(p2)) {
    return std::make_optional(std::move(p2));
  }
  p2 = bela::StringCat(p, L"\\cmd");
  if (IsExecutablePath(p2)) {
    return std::make_optional(std::move(p2));
  }
  return std::nullopt;
}

bool FlatPackageInitialize(std::wstring_view dir, std::wstring_view dest, bela::error_code &ec) {
  auto subfirst = UniqueSubdirectory(dir);
  if (!subfirst) {
    return true;
  }
  std::wstring currentdir = *subfirst;
  for (int i = 0; i < 10; i++) {
    auto subdir_ = UniqueSubdirectory(currentdir);
    if (!subdir_) {
      break;
    }
    currentdir.assign(std::move(*subdir_));
  }
  std::error_code e;
  std::filesystem::path destpath(dest);
  for (const auto &p : std::filesystem::directory_iterator(currentdir)) {
    auto newpath = destpath / p.path().filename();
    std::filesystem::rename(p.path(), newpath, e);
  }
  if (!PathRemoveEx(*subfirst, ec)) {
    return false;
  }
  return true;
}

std::optional<std::wstring> BaulkMakeTempDir(bela::error_code &ec) {
  std::error_code e;
  auto tmppath = std::filesystem::temp_directory_path(e);
  if (e) {
    ec = bela::from_std_error_code(e);
    return std::nullopt;
  }
  auto tmpdir = tmppath.wstring();
  if (!tmpdir.empty() && bela::IsPathSeparator(tmpdir.back())) {
    tmpdir.pop_back();
  }
  auto len = tmpdir.size();
  bela::AlphaNum an(GetCurrentThreadId());
  for (wchar_t X = 'A'; X < 'Z'; X++) {
    bela::StrAppend(&tmpdir, L"\\BaulkTemp", X, an);
    if (!bela::PathExists(tmpdir, bela::FileAttribute::Dir) && baulk::fs::MakeDir(tmpdir, ec)) {
      return std::make_optional(std::move(tmpdir));
    }
    tmpdir.resize(len);
  }
  ec = bela::make_error_code(1, L"cannot create tempdir");
  return std::nullopt;
}

} // namespace baulk::fs