///
///
#ifndef BAULK_EXTRACTOR_HPP
#define BAULK_EXTRACTOR_HPP
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <bela/terminal.hpp>
#include <filesystem>
#include <baulk/archive/extractor.hpp>

namespace baulk {
using baulk::archive::ExtractorOptions;
class Extractor {
public:
  virtual bool Extract(bela::error_code &ec) = 0;
};

std::shared_ptr<Extractor> MakeExtractor(const std::filesystem::path &archive_file,
                                         const std::filesystem::path &destination, const ExtractorOptions &opts,
                                         bela::error_code &ec);

bool extract_exe(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                 bela::error_code &ec);
bool extract_msi(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                 bela::error_code &ec);
bool extract_zip(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                 bela::error_code &ec);
bool extract_7z(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                bela::error_code &ec);
bool extract_tar(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                 bela::error_code &ec);
bool extract_auto(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                  bela::error_code &ec);

// command support
bool extract_command_auto(const std::filesystem::path &archive_file, const std::filesystem::path &destination,
                          bela::error_code &ec);
std::optional<std::filesystem::path> make_unqiue_extracted_destination(const std::filesystem::path &archive_file,
                                                                       std::filesystem::path &strict_folder);

using extract_method_t = decltype(&extract_exe);

inline auto resolve_extract_handle(const std::wstring_view extension) -> extract_method_t {
  static constexpr struct {
    std::wstring_view ext;
    extract_method_t fn;
  } extract_methods[]{
      {L"exe", extract_exe},   // exe
      {L"msi", extract_msi},   // msi
      {L"zip", extract_zip},   // zip
      {L"7z", extract_7z},     // 7z
      {L"tar", extract_tar},   // tar
      {L"auto", extract_auto}, // detect
  };
  for (const auto &m : extract_methods) {
    if (m.ext == extension) {
      return m.fn;
    }
  }
  return nullptr;
}

inline int extract_command_unchecked(const std::vector<std::wstring_view> &argv, extract_method_t fn) {
  std::filesystem::path archive_file(argv[0]);
  auto destination = [&]() -> std::optional<std::filesystem::path> {
    if (argv.size() > 1) {
      return std::make_optional<std::filesystem::path>(argv[1]);
    }
    std::filesystem::path strict_folder;
    if (auto d = make_unqiue_extracted_destination(archive_file, strict_folder); d) {
      return d;
    }
    bela::FPrintF(stderr, L"destination '%v' already exists\n", strict_folder);
    return std::nullopt;
  }();
  if (!destination) {
    return 1;
  }
  bela::error_code ec;
  if (!fn(archive_file, *destination, ec)) {
    if (ec) {
      bela::FPrintF(stderr, L"baulk extract: %v error: %v\n", archive_file.filename(), ec);
    }
    return 1;
  }
  return 0;
}

} // namespace baulk

#endif