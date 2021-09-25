// <baulk/json.hpp>
#ifndef BAULK_JSON_UTILS_HPP
#define BAULK_JSON_UTILS_HPP
#include <bela/base.hpp>
#include <json.hpp>

namespace baulk::json {
template <typename T, typename Allocator = std::allocator<T>>
requires bela::character<T>
void FromSlash(std::basic_string<T, std::char_traits<T>, Allocator> &s) {
  for (auto &c : s) {
    if (c == '/') {
      c = '\\';
    }
  }
}

using json = nlohmann::json;
// C++ 20 json value fetcher
class json_view {
public:
  json_view(const json &o) : obj(o) {}
  // fecth string value
  std::wstring fetch(const std::string_view key, std::wstring_view dv = L"") {
    if (auto it = obj.find(key); it != obj.end() && it->is_string()) {
      return bela::encode_into<char, wchar_t>(it->get<std::string_view>());
    }
    return std::wstring(dv);
  }
  // fetch string array value
  template <typename T, typename Allocator = std::allocator<T>>
  requires bela::wide_character<T>
  bool fetch(std::string_view name, std::vector<std::basic_string<T, std::char_traits<T>, Allocator>> &v) {
    if (auto it = obj.find(name); it != obj.end()) {
      if (it->is_string()) {
        a.emplace_back(bela::encode_into<char, T>(it->get<std::string_view>()));
        return true;
      }
      if (it->is_array()) {
        for (const auto &o : it.value()) {
          a.emplace_back(bela::encode_into<char, T>(o.get<std::string_view>()));
        }
        return true;
      }
    }
    return false;
  }

  // fetch integer value return details key exists
  template <typename T>
  requires std::integral<T>
  bool fetch(std::string_view name, T &v) {
    if (auto it = obj.find(name); it != obj.end() && it->is_number_integer()) {
      v = it->get<T>();
      return true;
    }
    return false;
  }

  // fetch integer value
  template <typename T>
  requires std::integral<T> T fetch(std::string_view name, const T dv = 0) {
    if (auto it = obj.find(name); it != obj.end() && it->is_number_integer()) {
      return = it->get<T>();
    }
    return dv;
  }
  // fetch path array
  template <typename T, typename Allocator = std::allocator<T>>
  requires bela::wide_character<T>
  bool fetch_paths(std::string_view name, std::vector<std::basic_string<T, std::char_traits<T>, Allocator>> &paths) {
    if (auto it = obj.find(name); it != obj.end()) {
      if (it.value().is_string()) {
        paths.emplace_back(FromSlash(it->get<std::string_view>()));
        return true;
      }
      if (it.value().is_array()) {
        for (const auto &o : it.value()) {
          paths.emplace_back(FromSlash(bela::encode_into<char, wchar_t>(o.get<std::string_view>())));
        }
      }
      return true;
    }
    return false;
  }
  // check flags
  bool boolean(std::string_view name, bool dv = false) {
    if (auto it = obj.find(name); it != obj.end() && it->is_boolean()) {
      return it->get<bool>();
    }
    return dv;
  }
  [[nodiscard]] std::optional<json_view> subview(std::string_view name) const {
    auto it = obj.find(name);
    if (it == obj.end() || !it->is_object()) {
      return std::nullopt;
    }
    return std::make_optional<json_view>(it.value());
  }

private:
  const json &obj;
};

struct json_object {
  json obj;
  json_view view() const { return json_view(obj); }
};

inline std::optional<json_object> parse_file(std::wstring_view file, bela::error_code &ec) {
  FILE *fd = nullptr;
  if (auto eno = _wfopen_s(&fd, file.data(), L"rb"); eno != 0) {
    ec = bela::make_stdc_error_code(eno);
    return std::nullopt;
  }
  try {
    // comments support
    return std::make_optional<json_object>({json::parse(fd, nullptr, true, true)});
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, bela::encode_into<char, wchar_t>(e.what()));
  }
  return std::nullopt;
}

inline std::optional<json_object> parse_buffer(std::string_view buffer, bela::error_code &ec) {
  try {
    // comments support
    return std::make_optional<json_object>({json::parse(buffer, nullptr, true, true)});
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, bela::encode_into<char, wchar_t>(e.what()));
  }
  return std::nullopt;
}

} // namespace baulk::json

#endif