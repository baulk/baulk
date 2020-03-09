///
#ifndef BAULK_JSON_HPP
#define BAULK_JSON_HPP
#include <json.hpp>
#include <bela/base.hpp>

namespace baulk::json {
//
inline bool BindTo(nlohmann::json &j, std::string_view name,
                   std::wstring &out) {
  if (auto it = j.find(name); it != j.end()) {
    out = bela::ToWide(it->get_ref<const std::string &>());
    return true;
  }
  return false;
}

inline bool BindTo(nlohmann::json &j, std::string_view name, bool &b) {
  if (auto it = j.find(name); it != j.end()) {
    b = it->get<bool>();
    return true;
  }
  return false;
}

template <typename Integer>
inline Integer Acquire(nlohmann::json &j, std::string_view name,
                       const Integer di) {
  if (auto it = j.find(name); it != j.end()) {
    return it->get<Integer>();
  }
  return di;
}

} // namespace baulk::json

#endif