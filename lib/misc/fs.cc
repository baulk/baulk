///
#include <bela/path.hpp>
#include <bela/match.hpp>
#include <baulk/fs.hpp>

namespace baulk::fs {
std::optional<std::wstring> UniqueSubdirectory(std::wstring_view dir) {
  bela::fs::Finder finder;
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
  bela::fs::Finder finder;
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

bool MakeFlattened(std::wstring_view dir, std::wstring_view dest, bela::error_code &ec) {
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
  if (!bela::fs::ForceDeleteFolders(*subfirst, ec)) {
    return false;
  }
  return true;
}

std::optional<std::wstring> NewTempFolder(bela::error_code &ec) {
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
  static std::atomic_uint32_t instanceId{0};
  auto len = tmpdir.size();
  bela::AlphaNum an(GetCurrentThreadId());
  for (wchar_t X = 'A'; X < 'Z'; X++) {
    bela::StrAppend(&tmpdir, L"\\BaulkTemp-", an, L"-", static_cast<uint32_t>(instanceId));
    instanceId++;
    if (!bela::PathExists(tmpdir, bela::FileAttribute::Dir) && baulk::fs::MakeDir(tmpdir, ec)) {
      return std::make_optional(std::move(tmpdir));
    }
    tmpdir.resize(len);
  }
  ec = bela::make_error_code(bela::ErrGeneral, L"cannot create tempdir");
  return std::nullopt;
}

} // namespace baulk::fs