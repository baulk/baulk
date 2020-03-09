///
#include <bela/base.hpp>
#include <bela/stdwriter.hpp>
#include <json.hpp>

namespace internal {
constexpr std::string_view vsj = R"([
  {
    "instanceId": "8575c259",
    "installDate": "2019-04-03T12:11:47Z",
    "installationName": "VisualStudio/16.4.5+29806.167",
    "installationPath": "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community",
    "installationVersion": "16.4.29806.167",
    "productId": "Microsoft.VisualStudio.Product.Community",
    "productPath": "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\Common7\\IDE\\devenv.exe",
    "state": 4294967295,
    "isComplete": true,
    "isLaunchable": true,
    "isPrerelease": false,
    "isRebootRequired": false,
    "displayName": "Visual Studio Community 2019",
    "description": "功能强大的 IDE，供学生、开放源代码参与者和个人免费使用",
    "channelId": "VisualStudio.16.Release",
    "channelUri": "https://aka.ms/vs/16/release/channel",
    "enginePath": "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\resources\\app\\ServiceHub\\Services\\Microsoft.VisualStudio.Setup.Service",
    "releaseNotes": "https://go.microsoft.com/fwlink/?LinkId=660893#16.4.5",
    "thirdPartyNotices": "https://go.microsoft.com/fwlink/?LinkId=660909",
    "updateDate": "2020-02-12T04:49:02.4552062Z",
    "catalog": {
      "buildBranch": "d16.4",
      "buildVersion": "16.4.29806.167",
      "id": "VisualStudio/16.4.5+29806.167",
      "localBuild": "build-lab",
      "manifestName": "VisualStudio",
      "manifestType": "installer",
      "productDisplayVersion": "16.4.5",
      "productLine": "Dev16",
      "productLineVersion": "2019",
      "productMilestone": "RTW",
      "productMilestoneIsPreRelease": "False",
      "productName": "Visual Studio",
      "productPatchVersion": "5",
      "productPreReleaseMilestoneSuffix": "1.0",
      "productSemanticVersion": "16.4.5+29806.167",
      "requiredEngineVersion": "2.4.1111.43337"
    },
    "properties": {
      "campaignId": "883107330.1507352461",
      "canceled": "0",
      "channelManifestId": "VisualStudio.16.Release/16.4.5+29806.167",
      "nickname": "",
      "setupEngineFilePath": "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vs_installershell.exe"          }
  }
])";

struct VisualStudioInstance {
  std::wstring installationPath;
  std::wstring installationVersion;
  std::wstring instanceId;
  std::wstring productId;
  bool isLaunchable{true};
  bool isPrerelease{false};
  bool Encode(const std::string_view result, bela::error_code &ec) {
    try {
      auto j0 = nlohmann::json::parse(result);
      if (!j0.is_array() || j0.empty()) {
        ec = bela::make_error_code(1, L"empty visual studio instance");
        return false;
      }
      auto &j = j0[0];
      if (auto it = j.find("installationPath"); it != j.end()) {
        installationPath = bela::ToWide(it->get_ref<const std::string &>());
      }
      if (auto it = j.find("installationVersion"); it != j.end()) {
        installationVersion = bela::ToWide(it->get_ref<const std::string &>());
      }
      if (auto it = j.find("instanceId"); it != j.end()) {
        instanceId = bela::ToWide(it->get_ref<const std::string &>());
      }
      if (auto it = j.find("productId"); it != j.end()) {
        productId = bela::ToWide(it->get_ref<const std::string &>());
      }
      if (auto it = j.find("isLaunchable"); it != j.end()) {
        isLaunchable = it->get<bool>();
      }
      if (auto it = j.find("isPrerelease"); it != j.end()) {
        isPrerelease = it->get<bool>();
      }
    } catch (const std::exception &e) {
      ec = bela::make_error_code(1, bela::ToWide(e.what()));
      return false;
    }
    return true;
  }
};
} // namespace internal

int wmain(int argc, wchar_t **argv) {
  internal::VisualStudioInstance vci;
  bela::error_code ec;
  if (!vci.Encode(internal::vsj, ec)) {
    bela::FPrintF(stderr, L"unable parse json: %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr,
                L"installationPath: %s\ninstallationVersion: %s\ninstanceId: "
                L"%s\nproductId: %s\nisLaunchable: %b\nisPrerelease： %b\n",
                vci.installationPath, vci.installationVersion, vci.instanceId,
                vci.productId, vci.isLaunchable, vci.isPrerelease);
  return 0;
}