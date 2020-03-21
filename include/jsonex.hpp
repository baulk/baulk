///
#ifndef BAULK_JSON_HPP
#define BAULK_JSON_HPP
#include <json.hpp>
#include <bela/base.hpp>

namespace baulk::json {
//
class JsonAssignor {
public:
  JsonAssignor(const nlohmann::json &obj_) : obj(obj_) {}
  JsonAssignor(const JsonAssignor &) = delete;
  JsonAssignor &operator=(const JsonAssignor &) = delete;
  std::wstring get(std::string_view name, std::wstring_view dv = L"") {
    if (auto it = obj.find(name); it != obj.end()) {
      return bela::ToWide(it->get_ref<const std::string &>());
    }
    return std::wstring(dv);
  }
  template <typename Integer>
  Integer integer(std::string_view name, const Integer dv) {
    if (auto it = obj.find(name); it != obj.end()) {
      return it->get<Integer>();
    }
    return dv;
  }
  bool boolean(std::string_view name, bool dv = false) {
    if (auto it = obj.find(name); it != obj.end()) {
      return it->get<bool>();
    }
    return dv;
  }

private:
  const nlohmann::json &obj;
};
} // namespace baulk::json

#endif