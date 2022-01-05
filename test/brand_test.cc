#include <cstdio>
#include <string>

// constexpr const std::string_view colors[] = {
//     //
//     "\x1b[48;2;231;245;255m", // 0
//     "\x1b[48;2;206;235;255m", // 1
//     "\x1b[48;2;182;225;255m", // 2
//     "\x1b[48;2;158;215;255m", // 3
//     "\x1b[48;2;134;205;255m", // 4
//     "\x1b[48;2;109;195;255m", // 5
//     "\x1b[48;2;85;185;255m",  // 6
//     "\x1b[48;2;61;175;255m",  // 7
//     "\x1b[48;2;36;165;255m",  // 8
//     "\x1b[48;2;12;155;255m",  // 9
//     "\x1b[48;2;11;155;255m",  // 10
//     "\x1b[48;2;10;155;255m",  // 11
//     "\x1b[48;2;10;154;255m",  // 12
//     "\x1b[48;2;8;154;255m",   // 13
//     "\x1b[48;2;7;153;255m",   // 14
//     "\x1b[48;2;6;153;255m",   // 15
//     "\x1b[48;2;5;152;255m",   // 16
//     "\x1b[48;2;4;152;255m",   // 17
//     "\x1b[48;2;2;151;255m",   // 18
//     "\x1b[48;2;1;151;255m",   // 19
//     "\x1b[48;2;0;150;255m",   // 20
// };

constexpr const std::string_view windows_colors[] = {
    "\x1b[48;2;109;195;255m", // 5
    "\x1b[48;2;85;185;255m",  // 6
    "\x1b[48;2;61;175;255m",  // 7
    "\x1b[48;2;36;165;255m",  // 8
};

// constexpr auto color_len = sizeof(colors) / sizeof(std::string_view);
constexpr std::string_view space = "                ";
struct Render {
  std::string prefix;
  std::string text;
  void draw_line(bool half) {
    text.append(prefix)
        .append(windows_colors[half ? 0 : 1])
        .append(space)
        .append("\x1b[48;5;15m  ")
        .append(windows_colors[half ? 2 : 3])
        .append(space)
        .append("\x1b[0m\n");
  }
  void draw() {
    for (size_t i = 0; i < 8; i++) {
      draw_line(true);
    }
    text.append(prefix).append("\x1b[48;5;15m").append(space).append("  ").append(space).append("\x1b[0m\n");
    for (size_t i = 0; i < 8; i++) {
      draw_line(false);
    }
  }
};

int main() {
  Render render;
  render.prefix = " ";
  render.draw();
  fprintf(stderr, "\x1b[38;2;209;40;40m Windows 11 Pro\x1b[0m\n\n%s\n", render.text.data());
  return 0;
}
