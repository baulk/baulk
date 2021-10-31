#ifndef BAULK_GRAPHICS_HPP
#define BAULK_GRAPHICS_HPP
#include <bela/base.hpp>
#include <bela/color.hpp>

namespace baulk::ui {
struct theme_mode {
  bela::color background;
  bela::color text;
  bela::color border;
};
constexpr auto light_mode = theme_mode{
    .background = bela::color(243, 243, 243),
    .text = bela::color(0, 0, 0),
    .border = bela::color(245, 245, 245),
};
constexpr auto dark_mode = theme_mode{
    .background = bela::color(26, 31, 52),
    .text = bela::color(255, 255, 255),
    .border = bela::color(51, 52, 54),
};
} // namespace baulk::ui

#endif