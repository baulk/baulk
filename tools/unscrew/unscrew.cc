#include "unscrew.hpp"
#include <bela/picker.hpp>
#include <bela/parseargv.hpp>
#include <baulk/fs.hpp>
#include <shellapi.h>
#include <ShlObj_core.h>
#include <CommCtrl.h>
#include <Objbase.h>
#include <version.hpp>

namespace baulk {
constexpr auto AppTitle = L"Unscrew - Baulk modern extractor";

class UnscrewProgressBar : public ProgressBar {
public:
  UnscrewProgressBar() = default;
  UnscrewProgressBar(const UnscrewProgressBar &) = delete;
  UnscrewProgressBar &operator=(const UnscrewProgressBar &) = delete;
  ~UnscrewProgressBar() {
    if (bar && initialized) {
      bar->StopProgressDialog();
    }
  }
  bool NewProgresBar(bela::error_code &ec) {
    if (CoCreateInstance(CLSID_ProgressDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IProgressDialog, (void **)&bar) !=
        S_OK) {
      ec = bela::make_system_error_code(L"CoCreateInstance ");
      return false;
    }
    return true;
  }
  bool Title(const std::wstring &title) {
    if (bar->SetTitle(title.data()) != S_OK) {
      return false;
    }
    if (!initialized) {
      if (bar->StartProgressDialog(nullptr, nullptr, PROGDLG_AUTOTIME, nullptr) != S_OK) {
        return false;
      }
      initialized = true;
    }
    return true;
  }
  bool Update(ULONGLONG ullCompleted, ULONGLONG ullTotal) {
    // update
    return bar->SetProgress64(ullCompleted, ullTotal) == S_OK;
  }
  bool UpdateLine(DWORD dwLineNum, const std::wstring &text, BOOL fCompactPath) {
    //
    return bar->SetLine(dwLineNum, text.data(), fCompactPath, nullptr) == S_OK;
  }
  bool Cancelled() { return false; }

private:
  bela::comptr<IProgressDialog> bar;
  bool initialized{false};
};

inline std::wstring_view strip_extension(const std::filesystem::path &filename) {
  return baulk::archive::PathStripExtension(filename.native());
}

std::optional<std::filesystem::path> make_unqiue_extracted_destination(const std::filesystem::path &archive_file,
                                                                       std::filesystem::path &strict_folder) {
  std::error_code e;
  auto parent_path = archive_file.parent_path();
  strict_folder = baulk::archive::PathStripExtension(archive_file.filename().native());
  auto d = parent_path / strict_folder;
  if (!std::filesystem::exists(d, e)) {
    return std::make_optional(std::move(d));
  }
  for (int i = 1; i < 100; i++) {
    d = parent_path / bela::StringCat(strict_folder, L"-(", i, L")");
    if (!std::filesystem::exists(d, e)) {
      return std::make_optional(std::move(d));
    }
  }
  return std::nullopt;
}

bool Executor::make_flat(const std::filesystem::path &dest) {
  if (!flat) {
    return true;
  }
  bela::error_code ec;
  return baulk::fs::MakeFlattened(dest, ec);
}

bool Executor::Execute(bela::error_code &ec) {
  constexpr bela::filter_t filters[] = {
      // archives
      {L"Zip Archive (*.zip)", L"*.zip"}, // zip archives
      {L"Self-extracting Archive (*.exe;*.com;*.dll)", L"*.exe;*.com;*.dll"},
      {L"7-Zip Archive (*.7z)", L"*.7z"},
      {L"MSIX Package (*.appx;*.msix;*.msixbundle)", L"*.appx;*.msix;*.msixbundle"}, // msix archives
      {L"OpenXML Archive (*.pptx;*.docx;*.xlsx)", L"*.pptx;*.docx;*.xlsx"},          // office archives
      {L"NuGet Package (*.nupkg)", L"*.nupkg"},                                      // nuget
      {L"Unix Archive (*.tar;*.gz;*.zstd;*.xz;*.bz2;*.br)", L"*.tar;*.gz;*.zstd;*.xz;*.bz2;*.br"},
      {L"Java Archive (*.jar)", L"*.jar"},
      {L"All Files (*.*)", L"*.*"}
      //
  };
  if (archive_files.empty()) {
    auto file = bela::FilePicker(nullptr, L"Extract archive", filters);
    if (!file) {
      return false;
    }
    archive_files.emplace_back(*file);
  }
  UnscrewProgressBar bar;
  if (!bar.NewProgresBar(ec)) {
    return false;
  }
  std::filesystem::path strict_folder;
  if (archive_files.size() == 1) {
    if (destination.empty()) {
      const auto &archive_file = archive_files[0];
      auto d = make_unqiue_extracted_destination(archive_file, strict_folder);
      if (!d) {
        ec = bela::make_error_code(bela::ErrGeneral, L"destination '", strict_folder, L"' already exists");
        return false;
      }
      destination = std::move(*d);
    }
    auto e = MakeExtractor(archive_files[0], destination, opts, ec);
    if (!e) {
      return false;
    }
    if (!e->Extract(&bar, ec)) {
      return false;
    }
    return make_flat(destination);
  }
  // Extracting multiple archives will ignore destination
  for (const auto &archive_file : archive_files) {
    auto destination_ = make_unqiue_extracted_destination(archive_file, strict_folder);
    if (!destination_) {
      continue;
    }
    auto e = MakeExtractor(archive_file, *destination_, opts, ec);
    if (!e) {
      return false;
    }
    if (!e->Extract(&bar, ec)) {
      return false;
    }
    make_flat(*destination_);
  }
  return true;
} // namespace baulk

void uncrew_usage() {
  constexpr wchar_t usage[] = LR"(Unscrew - Baulk modern extractor
Usage: unscrew [option] ...
  -h|--help
               Show usage text and quit
  -v|--version
               Show version number and quit
  -V|--verbose
               Make the operation more talkative
  -d|--destination
               Set archive extracted destination (extracting multiple archives will be ignored)
  -z|--flat
               Make destination folder to flat
)";
  bela::BelaMessageBox(nullptr, AppTitle, usage, BAULK_APPLINK, bela::mbs_t::ABOUT);
}

bool Executor::ParseArgv(bela::error_code &ec) {
  int Argc = 0;
  auto Argv = CommandLineToArgvW(GetCommandLineW(), &Argc);
  if (Argv == nullptr) {
    ec = bela::make_system_error_code();
    return false;
  }
  auto closer = bela::finally([&] { LocalFree(Argv); });
  std::error_code e;
  bela::ParseArgv pa(Argc, Argv);
  pa.Add(L"help", bela::no_argument, L'h')
      .Add(L"version", bela::no_argument, L'v')
      .Add(L"verbose", bela::no_argument, L'V')
      .Add(L"destination", bela::required_argument, L'd')
      .Add(L"flat", bela::no_argument, L'z');
  auto ret = pa.Execute(
      [&](int val, const wchar_t *oa, const wchar_t *) {
        switch (val) {
        case 'h':
          uncrew_usage();
          ExitProcess(0);
        case 'v':
          bela::BelaMessageBox(nullptr, AppTitle, BAULK_APPVERSION, BAULK_APPLINK, bela::mbs_t::ABOUT);
          ExitProcess(0);
        case 'V':
          debugMode = true;
          break;
        case 'd':
          destination = std::filesystem::absolute(oa, e);
          break;
        case 'z':
          flat = true;
          break;
        default:
          break;
        }
        return true;
      },
      ec);
  if (!ret) {
    return false;
  }
  for (const auto s : pa.UnresolvedArgs()) {
    archive_files.emplace_back(std::filesystem::absolute(s, e));
  }
  return true;
}

} // namespace baulk

class dot_global_initializer {
public:
  dot_global_initializer() {
    if (FAILED(::CoInitialize(nullptr))) {
      ::MessageBoxW(nullptr, L"CoInitialize() failed", L"COM initialize failed", MB_OK | MB_ICONERROR);
      ExitProcess(1);
    }
  }
  dot_global_initializer(const dot_global_initializer &) = delete;
  dot_global_initializer &operator=(const dot_global_initializer &) = delete;
  ~dot_global_initializer() { ::CoUninitialize(); }

private:
};

// Main
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow) {
  dot_global_initializer di;
  HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
  INITCOMMONCONTROLSEX info = {sizeof(INITCOMMONCONTROLSEX),
                               ICC_TREEVIEW_CLASSES | ICC_COOL_CLASSES | ICC_LISTVIEW_CLASSES};
  InitCommonControlsEx(&info);
  baulk::Executor executor;
  bela::error_code ec;
  if (!executor.ParseArgv(ec)) {
    bela::BelaMessageBox(nullptr, baulk::AppTitle, ec.data(), BAULK_APPLINK, bela::mbs_t::FATAL);
    return 0;
  }
  if (!executor.Execute(ec)) {
    if (ec != bela::ErrEOF) {
      bela::BelaMessageBox(nullptr, baulk::AppTitle, ec.data(), BAULK_APPLINK, bela::mbs_t::FATAL);
    }
    return 1;
  }
  return 0;
}