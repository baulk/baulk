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
    if (auto it = obj.find(name); it != obj.end() && it->is_string()) {
      return bela::ToWide(it->get_ref<const std::string &>());
    }
    return std::wstring(dv);
  }
  bool get(std::string_view name, std::vector<std::wstring> &a) {
    if (auto it = obj.find(name); it != obj.end()) {
      if (it->is_string()) {
        a.emplace_back(bela::ToWide(it->get<std::string_view>()));
        return true;
      }
      if (it->is_array()) {
        for (const auto &o : it.value()) {
          a.emplace_back(bela::ToWide(o.get<std::string_view>()));
        }
        return true;
      }
      return false;
    }
    return false;
  }
  template <typename T> bool array(std::string_view name, std::vector<T> &arr) {
    if (auto it = obj.find(name); it != obj.end()) {
      for (const auto &o : it.value()) {
        arr.emplace_back(bela::ToWide(o.get<std::string_view>()));
      }
      return true;
    }
    return false;
  }
  template <typename Integer>
  Integer integer(std::string_view name, const Integer dv) {
    if (auto it = obj.find(name); it != obj.end() && it->is_number_integer()) {
      return it->get<Integer>();
    }
    return dv;
  }
  bool boolean(std::string_view name, bool dv = false) {
    if (auto it = obj.find(name); it != obj.end() && it->is_boolean()) {
      return it->get<bool>();
    }
    return dv;
  }

private:
  const nlohmann::json &obj;
};
} // namespace baulk::json

#endif