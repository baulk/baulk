//
#include <bela/io.hpp>
#include <toml.hpp>
#include "vfsinternal.hpp"

namespace baulk::vfs {
bool PathFs::InitializeBaulkEnv(std::wstring_view envfile, bela::error_code &ec) {
  std::string text;
  constexpr int64_t envFileSize = 1024 * 1024 * 4;
  if (!bela::io::ReadFile(envfile, text, ec, envFileSize)) {
    return false;
  }
  try {
    auto tomltable = toml::parse(text);
    if (auto model = tomltable["model"].value<std::string_view>(); model) {
      fsmodel = bela::encode_into<char, wchar_t>(*model);
    }
  } catch (const toml::parse_error &e) {
    ec = bela::make_error_code(bela::encode_into<char, wchar_t>(e.what()));
    return false;
  }
  if (fsmodel.empty()) {
    fsmodel = L"PortableLegacy";
  }
  return true;
}
} // namespace baulk::vfs
