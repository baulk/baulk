///
#ifndef EXTRACTOR_HPP
#define EXTRACTOR_HPP
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <filesystem>
#include <baulk/archive/extractor.hpp>

namespace baulk {
using baulk::archive::ExtractorOptions;
class ProgressBar {
public:
  virtual bool Title(const std::wstring &title) = 0;
  virtual bool Update(ULONGLONG ullCompleted, ULONGLONG ullTotal) = 0;
  virtual bool UpdateLine(DWORD dwLineNum, const std::wstring &text, BOOL fCompactPath) = 0;
  virtual bool Cancelled() = 0;
};

class Extractor {
public:
  virtual bool Extract(ProgressBar *bar, bela::error_code &ec) = 0;
};

std::shared_ptr<Extractor> MakeExtractor(const std::filesystem::path &archive_file,
                                         const std::filesystem::path &destination, const ExtractorOptions &opts,
                                         bela::error_code &ec);

} // namespace baulk

#endif