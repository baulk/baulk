// base from https://github.com/microsoft/terminal/blob/master/src/inc/til/color.h
#ifndef BELA_COLOR_HPP
#define BELA_COLOR_HPP
#include "types.hpp"
#include <string>
#include <string_view>

namespace bela {
namespace color_internal {
constexpr int hexval_table[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0~0f
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 10~1f
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 20~2f
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, // 30~3f
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 40~4f
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 50~5f
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 60~6f
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 70~7f
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 80~8f
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 90~9f
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // a0~af
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // b0~bf
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // c0~cf
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // d0~df
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // e0~ef
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1  // f0~ff
};
constexpr int decode_byte_couple(uint8_t a, uint8_t b) {
  auto a1 = hexval_table[a];
  auto b1 = hexval_table[b];
  if (a1 < 0 || b1 < 0) {
    return -1;
  }
  return ((a1 << 4) | b1);
}

} // namespace color_internal
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

  template <typename T>
    requires bela::character<T>
  constexpr int encode(T *buffer, size_t len, const bool omitAlpha = false) const {
    constexpr char hex[] = "0123456789ABCDEF";
    if (len < 7) {
      return 0;
    }
    buffer[0] = '#';
    buffer[1] = hex[r >> 4];
    buffer[2] = hex[r & 0xF];
    buffer[3] = hex[g >> 4];
    buffer[4] = hex[g & 0xF];
    buffer[5] = hex[b >> 4];
    buffer[6] = hex[b & 0xF];
    if (!omitAlpha || len < 9) {
      return 7;
    }
    buffer[7] = hex[a >> 4];
    buffer[8] = hex[a & 0xF];
    return 9;
  }

  template <typename T = wchar_t, typename Allocator = std::allocator<T>>
    requires bela::character<T>
  [[nodiscard]] std::basic_string<T, std::char_traits<T>, Allocator> encode(const bool omitAplha = false) const {
    using string_t = std::basic_string<T, std::char_traits<T>, Allocator>;
    string_t text;
    text.resize(omitAplha ? 9 : 7);
    auto n = encode(text.data(), text.size(), omitAplha);
    text.resize(n);
    return text;
  }
  static constexpr bela::color unresolved() { return bela::color(); }
  // decode color from text eg: #F5F5F5
  template <typename CharT = char8_t>
    requires bela::character<CharT>
  static constexpr bela::color decode(std::basic_string_view<CharT> text, bela::color defaults = unresolved()) {
    if (text.size() < 7 || text.front() != '#') {
      return defaults;
    }
    text.remove_prefix(1);
    int a = 255;
    if (text.size() >= 8) {
      a = color_internal::decode_byte_couple(static_cast<uint8_t>(text[6]), static_cast<uint8_t>(text[7]));
    }
    auto r = color_internal::decode_byte_couple(static_cast<uint8_t>(text[0]), static_cast<uint8_t>(text[1]));
    auto g = color_internal::decode_byte_couple(static_cast<uint8_t>(text[2]), static_cast<uint8_t>(text[3]));
    auto b = color_internal::decode_byte_couple(static_cast<uint8_t>(text[4]), static_cast<uint8_t>(text[5]));
    if (r == -1 || g == -1 || b == -1 || a == -1) {
      return defaults;
    }
    return bela::color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b),
                       static_cast<uint8_t>(a));
  }
  template <typename CharT = char8_t, size_t N>
    requires bela::character<CharT>
  [[nodiscard]] static constexpr bela::color decode(CharT (&str)[N], bela::color defaults = unresolved()) {
    return decode(std::basic_string_view(str, N), defaults);
  }
};

} // namespace bela

#endif