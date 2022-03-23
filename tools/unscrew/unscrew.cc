#include "unscrew.hpp"
#include <bela/picker.hpp>
#include <shellapi.h>
#include <CommCtrl.h>
#include <Objbase.h>
#include <version.hpp>

namespace baulk {

bool Executor::Execute(bela::error_code &ec) {
  constexpr bela::filter_t filters[] = {
      // archives
      {L"Zip Archive (*.zip)", L"*.zip"}, // zip archives
      {L"Self-extracting Archive (*.exe;*.com;*.dll)", L"*.exe;*.com;*.dll"},
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
  if (destination.empty()) {
    destination = archive_files[0].parent_path() / archive::PathStripExtension(archive_files[0].filename().native());
  }
  bela::comptr<IProgressDialog> bar;
  if (CoCreateInstance(CLSID_ProgressDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IProgressDialog, (void **)&bar) !=
      S_OK) {
    auto ec = bela::make_system_error_code(L"CoCreateInstance ");
    return false;
  }
  auto closer = bela::finally([&] { bar->StopProgressDialog(); });
  for (const auto &archive_file : archive_files) {
    auto e = MakeExtractor(archive_file, destination, opts, ec);
    if (!e) {
      return false;
    }
    if (!e->Extract(bar.get(), ec)) {
      return false;
    }
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
               Set archive extracted destination

)";
  bela::BelaMessageBox(nullptr, L"Unscrew - Baulk modern extractor", usage, BAULK_APPLINK, bela::mbs_t::ABOUT);
}

bool Executor::ParseArgv(bela::error_code &ec) {
  int Argc = 0;
  auto Argv = CommandLineToArgvW(GetCommandLineW(), &Argc);
  if (Argv == nullptr) {
    ec = bela::make_system_error_code();
    return false;
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
  if (!executor.Execute(ec)) {

    return 1;
  }
  return 0;
}