//
#ifndef HAZEL_HAZEL_HPP
#define HAZEL_HAZEL_HPP
#include <variant>
#include <bela/base.hpp>
#include <bela/phmap.hpp>
#include <bela/buffer.hpp>
#include <bela/time.hpp>
#include <bela/io.hpp>
#include "types.hpp"

namespace hazel {
using bela::ErrGeneral;
// https://en.cppreference.com/w/cpp/utility/variant/visit
// https://en.cppreference.com/w/cpp/utility/variant

// helper constant for the visitor #3
template <class> constexpr bool always_false_v = false;

// helper type for the visitor #4
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

class hazel_result;
bool LookupFile(const bela::io::FD &fd, hazel_result &hr, bela::error_code &ec, int64_t offset = 0);
bool LookupBytes(bela::bytes_view bv, hazel_result &hr, bela::error_code &ec);
using hazel_value_t = std::variant<std::string, std::wstring, std::vector<std::string>, std::vector<std::wstring>,
                                   int16_t, int32_t, int64_t, uint16_t, uint32_t, uint64_t, bela::Time>;
class hazel_result {
public:
  hazel_result() = default;
  hazel_result(const hazel_result &) = delete;
  hazel_result &operator=(const hazel_result &) = delete;
  hazel_result &assign(types::hazel_types_t ty, std::wstring &&desc) {
    t = ty;
    description_.assign(std::move(desc));
    return *this;
  }
  hazel_result &append(std::wstring_view key, const std::wstring_view &value) {
    align_len_ = (std::max)(key.size(), align_len_);
    values_.emplace(key, hazel_value_t(std::wstring(value)));
    return *this;
  }
  hazel_result &append(std::wstring_view key, std::wstring &&value) {
    align_len_ = (std::max)(key.size(), align_len_);
    values_.emplace(key, hazel_value_t(std::move(value)));
    return *this;
  }
  hazel_result &append(std::wstring_view key, const std::string_view &value) {
    align_len_ = (std::max)(key.size(), align_len_);
    values_.emplace(key, hazel_value_t(std::string(value)));
    return *this;
  }
  hazel_result &append(std::wstring_view key, std::string &&value) {
    align_len_ = (std::max)(key.size(), align_len_);
    values_.emplace(key, hazel_value_t(std::move(value)));
    return *this;
  }
  hazel_result &append(std::wstring_view key, std::vector<std::wstring> &&value) {
    align_len_ = (std::max)(key.size(), align_len_);
    values_.emplace(key, hazel_value_t(std::move(value)));
    return *this;
  }
  hazel_result &append(std::wstring_view key, std::vector<std::string> &&value) {
    align_len_ = (std::max)(key.size(), align_len_);
    values_.emplace(key, hazel_value_t(std::move(value)));
    return *this;
  }
  template <typename T> hazel_result &append(std::wstring_view key, T value) {
    align_len_ = (std::max)(key.size(), align_len_);
    values_.emplace(key, hazel_value_t(value));
    return *this;
  }
  const auto &description() const { return description_; }
  auto type() const { return t; }
  auto size() const { return size_; }
  auto align_length() const { return align_len_; }
  const auto &values() const { return values_; }
  bool LooksLikeELF() const {
    return t == types::elf || t == types::elf_executable || t == types::elf_relocatable ||
           t == types::elf_shared_object;
  }
  bool LooksLikeMachO() const {
    constexpr types::hazel_types_t machos[] = {
        types::macho_bundle,
        types::macho_core,
        types::macho_dsym_companion,
        types::macho_dynamic_linker,
        types::macho_dynamically_linked_shared_lib,
        types::macho_dynamically_linked_shared_lib_stub,
        types::macho_executable,
        types::macho_fixed_virtual_memory_shared_lib,
        types::macho_kext_bundle,
        types::macho_object,
        types::macho_object,
        types::macho_universal_binary,
    };
    return std::find(std::begin(machos), std::end(machos), t) != std::end(machos);
  }
  bool LooksLikePE() const { return t == types::pecoff_executable; }
  bool LooksLikeZIP() const {
    return t == types::zip || t == types::docx || t == types::xlsx || t == types::pptx || t == types::ofd;
  }
  bool ZeroExists() const { return zeroPosition != -1; }

private:
  friend bool LookupBytes(bela::bytes_view bv, hazel_result &hr, bela::error_code &ec);
  friend bool LookupFile(const bela::io::FD &fd, hazel_result &hr, bela::error_code &ec, int64_t offset);
  std::wstring description_;
  bela::flat_hash_map<std::wstring, hazel_value_t> values_;
  int64_t size_{bela::SizeUnInitialized};
  size_t align_len_{sizeof("description") - 1};
  types::hazel_types_t t{types::none};
  int64_t zeroPosition{-1};
};

const wchar_t *LookupMIME(types::hazel_types_t t);

} // namespace hazel

#endif