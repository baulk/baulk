//
#ifndef BAULKTERMINAL_HPP
#define BAULKTERMINAL_HPP
#pragma once
#include <bela/base.hpp>

namespace baulkterminal {
constexpr const wchar_t *string_nullable(std::wstring_view str) {
  return str.empty() ? nullptr : str.data();
}
std::optional<std::wstring> MakeEnv(bool usevs);
} // namespace baulkterminal

#endif