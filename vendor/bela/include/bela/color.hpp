// base from https://github.com/microsoft/terminal/blob/master/src/inc/til/color.h
#ifndef BELA_COLOR_HPP
#define BELA_COLOR_HPP
#include <charconv>
#include "charconv.hpp"
#include "base.hpp"
#include "numbers.hpp"
#include "narrow/strcat.hpp"

namespace bela {
// color is a universal integral 8bpp RGBA (0-255) color type implicitly convertible to/from
// a number of other color types.
struct color {
  // Clang (10) has no trouble optimizing the COLORREF conversion operator, below, to a
  // simple 32-bit load with mask (!) even though it's a series of bit shifts across
  // multiple struct members.
  // CL (19.24) doesn't make the same optimization decision, and it emits three 8-bit loads
  // and some shifting.
  // In any case, the optimization only applies at -O2 (clang) and above.
  // Here, we leverage the spec-legality of using unions for type conversions and the
  // overlap of four uint8_ts and a uint32_t to make the conversion very obvious to
  // both compilers.
  union {
    struct {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ // Clang, GCC
      uint8_t a, b, g, r;
#else
      uint8_t r, g, b, a;
#endif
    };
    uint32_t abgr;
  };

  constexpr color() noexcept : r{0}, g{0}, b{0}, a{0} {}

  constexpr color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255) noexcept : r{_r}, g{_g}, b{_b}, a{_a} {}

  constexpr color(const color &) = default;
  constexpr color(color &&) = default;
  color &operator=(const color &) = default;
  color &operator=(color &&) = default;
  ~color() = default;

#ifdef _WINDEF_
  constexpr color(COLORREF c) : abgr{static_cast<uint32_t>(c | 0xFF000000u)} {}

  operator COLORREF() const noexcept { return static_cast<COLORREF>(abgr & 0x00FFFFFFu); }
#endif

  // Method Description:
  // - Converting constructor for any other color structure type containing integral R, G, B, A (case sensitive.)
  // Notes:
  // - This and all below conversions make use of std::enable_if and a default parameter to disambiguate themselves.
  //   enable_if will result in an <error-type> if the constraint within it is not met, which will make this
  //   template ill-formed. Because SFINAE, ill-formed templated members "disappear" instead of causing an error.
  template <typename TOther>
  constexpr color(const TOther &other, std::enable_if_t<std::is_integral_v<decltype(std::declval<TOther>().R)> &&
                                                            std::is_integral_v<decltype(std::declval<TOther>().A)>,
                                                        int> /*sentinel*/
                                       = 0)
      : r{static_cast<uint8_t>(other.R)}, g{static_cast<uint8_t>(other.G)}, b{static_cast<uint8_t>(other.B)},
        a{static_cast<uint8_t>(other.A)} {}

  // Method Description:
  // - Converting constructor for any other color structure type containing integral r, g, b, a (case sensitive.)
  template <typename TOther>
  constexpr color(const TOther &other, std::enable_if_t<std::is_integral_v<decltype(std::declval<TOther>().r)> &&
                                                            std::is_integral_v<decltype(std::declval<TOther>().a)>,
                                                        int> /*sentinel*/
                                       = 0)
      : r{static_cast<uint8_t>(other.r)}, g{static_cast<uint8_t>(other.g)}, b{static_cast<uint8_t>(other.b)},
        a{static_cast<uint8_t>(other.a)} {}

  // Method Description:
  // - Converting constructor for any other color structure type containing floating-point R, G, B, A (case sensitive.)
  template <typename TOther>
  constexpr color(const TOther &other,
                  std::enable_if_t<std::is_floating_point_v<decltype(std::declval<TOther>().R)> &&
                                       std::is_floating_point_v<decltype(std::declval<TOther>().A)>,
                                   float> /*sentinel*/
                  = 1.0f)
      : r{static_cast<uint8_t>(other.R * 255.0f)}, g{static_cast<uint8_t>(other.G * 255.0f)},
        b{static_cast<uint8_t>(other.B * 255.0f)}, a{static_cast<uint8_t>(other.A * 255.0f)} {}

  // Method Description:
  // - Converting constructor for any other color structure type containing floating-point r, g, b, a (case sensitive.)
  template <typename TOther>
  constexpr color(const TOther &other,
                  std::enable_if_t<std::is_floating_point_v<decltype(std::declval<TOther>().r)> &&
                                       std::is_floating_point_v<decltype(std::declval<TOther>().a)>,
                                   float> /*sentinel*/
                  = 1.0f)
      : r{static_cast<uint8_t>(other.r * 255.0f)}, g{static_cast<uint8_t>(other.g * 255.0f)},
        b{static_cast<uint8_t>(other.b * 255.0f)}, a{static_cast<uint8_t>(other.a * 255.0f)} {}

  constexpr color with_alpha(uint8_t alpha) const { return color{r, g, b, alpha}; }

#ifdef D3DCOLORVALUE_DEFINED
  constexpr operator D3DCOLORVALUE() const { return D3DCOLORVALUE{r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f}; }
#endif

#ifdef WINRT_Windows_UI_H
  constexpr color(const winrt::Windows::UI::Color &winUIColor)
      : color(winUIColor.R, winUIColor.G, winUIColor.B, winUIColor.A) {}

  operator winrt::Windows::UI::Color() const {
    winrt::Windows::UI::Color ret;
    ret.R = r;
    ret.G = g;
    ret.B = b;
    ret.A = a;
    return ret;
  }
#endif

  constexpr bool operator==(const bela::color &other) const { return abgr == other.abgr; }

  constexpr bool operator!=(const bela::color &other) const { return !(*this == other); }

  std::wstring Encode(const bool omitAlpha = true) const {
    if (omitAlpha) {
      return bela::StringCat(L"#", bela::AlphaNum(bela::Hex(r, bela::kZeroPad2)),
                             bela::AlphaNum(bela::Hex(g, bela::kZeroPad2)),
                             bela::AlphaNum(bela::Hex(b, bela::kZeroPad2)));
    }
    return bela::StringCat(L"#", bela::AlphaNum(bela::Hex(a, bela::kZeroPad2)),
                           bela::AlphaNum(bela::Hex(r, bela::kZeroPad2)), bela::AlphaNum(bela::Hex(g, bela::kZeroPad2)),
                           bela::AlphaNum(bela::Hex(b, bela::kZeroPad2)));
  }
  std::string NarrowEncode(const bool omitAlpha = true) const {
    if (omitAlpha) {
      return bela::narrow::StringCat("#", bela::narrow::AlphaNum(bela::narrow::Hex(r, bela::narrow::kZeroPad2)),
                                     bela::narrow::AlphaNum(bela::narrow::Hex(g, bela::narrow::kZeroPad2)),
                                     bela::narrow::AlphaNum(bela::narrow::Hex(b, bela::narrow::kZeroPad2)));
    }
    return bela::narrow::StringCat("#", bela::narrow::AlphaNum(bela::narrow::Hex(a, bela::narrow::kZeroPad2)),
                                   bela::narrow::AlphaNum(bela::narrow::Hex(r, bela::narrow::kZeroPad2)),
                                   bela::narrow::AlphaNum(bela::narrow::Hex(g, bela::narrow::kZeroPad2)),
                                   bela::narrow::AlphaNum(bela::narrow::Hex(b, bela::narrow::kZeroPad2)));
  }
  static bool Decode(std::string_view sr, color &c) {
    if (sr.empty() || sr.front() != '#') {
      return false;
    }
    sr.remove_prefix(1);
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;
    if (sr.size() == 8) {
      auto r0 = std::from_chars(sr.data(), sr.data() + 2, a, 16);
      if (r0.ec != std::errc{}) {
        return false;
      }
      sr.remove_prefix(2);
    }
    if (sr.size() != 6) {
      return false;
    }
    auto r1 = std::from_chars(sr.data(), sr.data() + 2, r, 16);
    auto r2 = std::from_chars(sr.data() + 2, sr.data() + 4, g, 16);
    auto r3 = std::from_chars(sr.data() + 4, sr.data() + 6, b, 16);
    if (r1.ec != std::errc{} || r2.ec != std::errc{} || r3.ec != std::errc{}) {
      return false;
    }
    c = color(r, g, b, a);
    return true;
  }
  static bool Decode(std::wstring_view sr, color &c) {
    if (sr.empty() || sr.front() != L'#') {
      return false;
    }
    sr.remove_prefix(1);
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;
    if (sr.size() == 8) {
      auto r0 = bela::from_chars(sr.data(), sr.data() + 2, a, 16);
      if (r0.ec != std::errc{}) {
        return false;
      }
      sr.remove_prefix(2);
    }
    if (sr.size() != 6) {
      return false;
    }
    auto r1 = bela::from_chars(sr.data(), sr.data() + 2, r, 16);
    auto r2 = bela::from_chars(sr.data() + 2, sr.data() + 4, g, 16);
    auto r3 = bela::from_chars(sr.data() + 4, sr.data() + 6, b, 16);
    if (r1.ec != std::errc{} || r2.ec != std::errc{} || r3.ec != std::errc{}) {
      return false;
    }
    c = color(r, g, b, a);
    return true;
  }
};

} // namespace bela

#endif