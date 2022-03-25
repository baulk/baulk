///
#include <bela/path.hpp>
#include <bela/match.hpp>
#include <bela/ascii.hpp>
#include <baulk/fs.hpp>

namespace baulk::fs {

inline bool has_executable_extension(const std::filesystem::path &p) {
  constexpr std::wstring_view exe_extensions[] = {L".exe", L".com", L".bat", L".cmd"};
  auto le = bela::AsciiStrToLower(p.extension().native());
  for (const auto &e : exe_extensions) {
    if (e == le) {
      return true;
    }
  }
  return false;
}

bool IsExecutablePath(const std::filesystem::path &p) {
  std::error_code e;
  for (const auto &d : std::filesystem::directory_iterator(p, e)) {
    if (d.is_directory(e)) {
      continue;
    }
    if (has_executable_extension(d.path())) {
      return true;
    }
  }
  return false;
}

std::optional<std::filesystem::path> FindExecutablePath(const std::filesystem::path &p) {
  std::error_code e;
  if (!std::filesystem::is_directory(p, e)) {
    return std::nullopt;
  }
  if (IsExecutablePath(p)) {
    return std::make_optional<std::filesystem::path>(p);
  }
  auto bin = p / L"bin";
  if (IsExecutablePath(bin)) {
    return std::make_optional(std::move(bin));
  }
  auto cmd = p / L"cmd";
  if (IsExecutablePath(cmd)) {
    return std::make_optional(std::move(cmd));
  }
  return std::nullopt;
}

inline std::optional<std::filesystem::path> flattened_recursive(std::filesystem::path &current, std::error_code &e) {
  int entries = 0;
  std::filesystem::path folder0;
  for (const auto &entry : std::filesystem::directory_iterator{current, e}) {
    if (!entry.is_directory(e)) {
      return std::make_optional(current);
    }
    if (bela::EqualsIgnoreCase(entry.path().filename().native(), L"bin")) {
      return std::make_optional(current);
    }
    entries++;
    if (entries) {
      folder0 = entry.path();
    }
  }
  if (entries != 1) {
    return std::make_optional(current);
  }
  current = folder0;
  return std::nullopt;
}

std::optional<std::filesystem::path> flattened_internal(const std::filesystem::path &d, std::filesystem::path &depth1) {
  std::error_code e;
  if (!std::filesystem::is_directory(d, e)) {
    return std::nullopt;
  }
  std::filesystem::path current{std::filesystem::absolute(d, e)};
  for (int i = 0; i < 20; i++) {
    if (auto flatd = flattened_recursive(current, e); flatd) {
      return flatd;
    }
    if (depth1.empty()) {
      depth1 = current;
    }
  }
  return std::nullopt;
}

std::optional<std::filesystem::path> Flattened(const std::filesystem::path &d) {
  std::filesystem::path unused;
  return flattened_internal(d, unused);
}

bool MakeFlattened(const std::filesystem::path &d, bela::error_code &ec) {
  std::error_code e;
  std::filesystem::path depth1;
  auto flat = flattened_internal(d, depth1);
  if (!flat) {
    ec = bela::make_error_code(L"no conditions for a flattened path");
    return false;
  }
  if (std::filesystem::equivalent(d, *flat, e)) {
    return true;
  }
  for (const auto &entry : std::filesystem::directory_iterator{*flat, e}) {
    auto newPath = d / entry.path().filename();
    std::filesystem::rename(entry.path(), newPath, e);
  }
  if (!depth1.empty()) {
    std::filesystem::remove_all(depth1, e);
  }
  return true;
}

std::optional<std::filesystem::path> NewTempFolder(bela::error_code &ec) {
  std::error_code e;
  auto temp = std::filesystem::temp_directory_path(e);
  if (e) {
    ec = bela::from_std_error_code(e);
    return std::nullopt;
  }
  static std::atomic_uint32_t instanceId{0};
  bela::AlphaNum an(GetCurrentThreadId());
  for (wchar_t X = 'A'; X < 'Z'; X++) {
    auto newPath = temp / bela::StringCat(L"bauk-build-", an, L"-", static_cast<uint32_t>(instanceId));
    instanceId++;
    if (!std::filesystem::exists(newPath, e) && baulk::fs::MakeDirectories(newPath, ec)) {
      return std::make_optional(std::move(newPath));
    }
  }
  ec = bela::make_error_code(bela::ErrGeneral, L"cannot create tempdir");
  return std::nullopt;
}

} // namespace baulk::fs