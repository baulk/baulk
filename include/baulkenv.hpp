//
#ifndef BAULK_BAULKENV_HPP
#define BAULK_BAULKENV_HPP
#include <bela/path.hpp>
#include <bela/env.hpp>

namespace baulk::env {
struct Searcher {
  Searcher(bela::env::Derivator &dev_) : dev{dev_} {}
  using vector_t = std::vector<std::wstring>;
  bela::env::Derivator &dev;
  vector_t paths;
  vector_t libs;
  vector_t includes;
  vector_t libpaths;
  bool JoinEnvInternal(vector_t &vec, std::wstring &&p) {
    if (bela::PathExists(p)) {
      vec.emplace_back(std::move(p));
      return true;
    }
    return false;
  }
  void JoinForceEnv(vector_t &vec, std::wstring_view p) { vec.emplace_back(std::wstring(p)); }
  void JoinForceEnv(vector_t &vec, std::wstring_view a, std::wstring_view b) {
    vec.emplace_back(bela::StringCat(a, b));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view p) { return JoinEnvInternal(vec, std::wstring(p)); }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b) {
    auto p = bela::StringCat(a, b);
    return JoinEnvInternal(vec, std::wstring(p));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c) {
    auto p = bela::StringCat(a, b, c);
    return JoinEnvInternal(vec, std::wstring(p));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c,
               std::wstring_view d) {
    auto p = bela::StringCat(a, b, c, d);
    return JoinEnvInternal(vec, std::wstring(p));
  }
  template <typename... Args>
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c,
               std::wstring_view d, Args... args) {
    auto p = bela::strings_internal::CatPieces({a, b, c, d, args...});
    return JoinEnvInternal(vec, std::wstring(p));
  }
  std::wstring CleanupEnv();
  std::wstring MakeEnv();
  bool InitializeWindowsKitEnv(bela::error_code &ec);
  bool InitializeVisualStudioEnv(bool clang, bela::error_code &ec);
  bool InitializeBaulk(bela::error_code &ec);
  bool InitializeGit(bool cleanup, bela::error_code &ec);
};

} // namespace baulk::env

#endif