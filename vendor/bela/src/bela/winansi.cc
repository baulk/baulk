// Old Windows ANSI print
// Thanks:
// https://github.com/microsoftarchive/redis/blob/3.0/src/Win32_Interop/Win32_ANSI.c
#include <string_view>
#include <bela/base.hpp>
#include <bela/stdwriter.hpp> // ssize_t
#include <bela/strcat.hpp>

namespace bela {
#define lenof(array) (sizeof(array) / sizeof(*(array)))
#define is_digit(c) ('0' <= (c) && (c) <= '9')

struct GraphicRenditionMode {
  BYTE foreground; // ANSI base color (0 to 7; add 30)
  BYTE background; // ANSI base color (0 to 7; add 40)
  BYTE bold;       // console FOREGROUND_INTENSITY bit
  BYTE underline;  // console BACKGROUND_INTENSITY bit
  BYTE rvideo;     // swap foreground/bold & background/underline
  BYTE concealed;  // set foreground/bold to background/underline
  BYTE reverse;    // swap console foreground & background attributes
};

constexpr BYTE ESC = '\x1B';
constexpr BYTE BEL = '\x07';
constexpr BYTE SO = '\x0E';
constexpr BYTE SI = '\x0F';

constexpr BYTE FirstG1 = '-';
constexpr BYTE LastG1 = '~';

#define MAX_ARG 16 // max number of args in an escape sequence
#define BUFFER_SIZE 2048

#define FOREGROUND_BLACK 0
#define FOREGROUND_WHITE FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE

#define BACKGROUND_BLACK 0
#define BACKGROUND_WHITE BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE

// DEC Special Graphics Character Set from
// http://vt100.net/docs/vt220-rm/table2-4.html
// Some of these may not look right, depending on the font and code page (in
// particular, the Control Pictures probably won't work at all).
constexpr const wchar_t G1[] = {
    ' ',       // _ - blank
    L'\x2666', // ` - Black Diamond Suit
    L'\x2592', // a - Medium Shade
    L'\x2409', // b - HT
    L'\x240c', // c - FF
    L'\x240d', // d - CR
    L'\x240a', // e - LF
    L'\x00b0', // f - Degree Sign
    L'\x00b1', // g - Plus-Minus Sign
    L'\x2424', // h - NL
    L'\x240b', // i - VT
    L'\x2518', // j - Box Drawings Light Up And Left
    L'\x2510', // k - Box Drawings Light Down And Left
    L'\x250c', // l - Box Drawings Light Down And Right
    L'\x2514', // m - Box Drawings Light Up And Right
    L'\x253c', // n - Box Drawings Light Vertical And Horizontal
    L'\x00af', // o - SCAN 1 - Macron
    L'\x25ac', // p - SCAN 3 - Black Rectangle
    L'\x2500', // q - SCAN 5 - Box Drawings Light Horizontal
    L'_',      // r - SCAN 7 - Low Line
    L'_',      // s - SCAN 9 - Low Line
    L'\x251c', // t - Box Drawings Light Vertical And Right
    L'\x2524', // u - Box Drawings Light Vertical And Left
    L'\x2534', // v - Box Drawings Light Up And Horizontal
    L'\x252c', // w - Box Drawings Light Down And Horizontal
    L'\x2502', // x - Box Drawings Light Vertical
    L'\x2264', // y - Less-Than Or Equal To
    L'\x2265', // z - Greater-Than Or Equal To
    L'\x03c0', // { - Greek Small Letter Pi
    L'\x2260', // | - Not Equal To
    L'\x00a3', // } - Pound Sign
    L'\x00b7', // ~ - Middle Dot
};

const BYTE foregroundcolor[8] = {
    FOREGROUND_BLACK,                   // black foreground
    FOREGROUND_RED,                     // red foreground
    FOREGROUND_GREEN,                   // green foreground
    FOREGROUND_RED | FOREGROUND_GREEN,  // yellow foreground
    FOREGROUND_BLUE,                    // blue foreground
    FOREGROUND_BLUE | FOREGROUND_RED,   // magenta foreground
    FOREGROUND_BLUE | FOREGROUND_GREEN, // cyan foreground
    FOREGROUND_WHITE                    // white foreground
};

const BYTE backgroundcolor[8] = {
    BACKGROUND_BLACK,                   // black background
    BACKGROUND_RED,                     // red background
    BACKGROUND_GREEN,                   // green background
    BACKGROUND_RED | BACKGROUND_GREEN,  // yellow background
    BACKGROUND_BLUE,                    // blue background
    BACKGROUND_BLUE | BACKGROUND_RED,   // magenta background
    BACKGROUND_BLUE | BACKGROUND_GREEN, // cyan background
    BACKGROUND_WHITE,                   // white background
};

const BYTE attr2ansi[8] = // map console attribute to ANSI number
    {
        0, // black
        4, // blue
        2, // green
        6, // cyan
        1, // red
        5, // magenta
        3, // yellow
        7  // white
};

class WinAnsiWriter {
public:
  WinAnsiWriter(const WinAnsiWriter &) = delete;
  WinAnsiWriter &operator=(const WinAnsiWriter &) = delete;
  static WinAnsiWriter &Inst() {
    static WinAnsiWriter aw;
    return aw;
  }
  ssize_t WriteAnsi(HANDLE hDev, std::wstring_view msg);

private:
  WinAnsiWriter() {
    //
    memset(es_argv, 0, sizeof(es_argv));
    memset(ChBuffer, 0, sizeof(ChBuffer));
    memset(Pt_arg, 0, sizeof(Pt_arg));
    memset(&grm, 0, sizeof(grm));
    SavePos.X = 0;
    SavePos.Y = 0;
  }
  void FlushBuffer();
  void PushBuffer(wchar_t c);
  void SendSequence(std::wstring_view seq);
  void InterpretEscSeq();
  int state{0};
  wchar_t prefix{0};
  wchar_t prefix2{0};
  wchar_t suffix{0};            // escape sequence suffix
  int es_argc{0};               // escape sequence args count
  int es_argv[MAX_ARG];         // escape sequence args
  wchar_t Pt_arg[MAX_PATH * 2]; // text parameter for Operating System Command
  int Pt_len{0};
  bool shifted{false};
  GraphicRenditionMode grm;
  COORD SavePos;
  int nCharInBuffer{0};
  WCHAR ChBuffer[BUFFER_SIZE];
  HANDLE hConOut{INVALID_HANDLE_VALUE};
};

void WinAnsiWriter::FlushBuffer() {
  if (nCharInBuffer <= 0) {
    return;
  }
  DWORD nWritten = 0;
  WriteConsoleW(hConOut, ChBuffer, nCharInBuffer, &nWritten, nullptr);
  nCharInBuffer = 0;
}

void WinAnsiWriter::PushBuffer(wchar_t c) {
  if (shifted && c >= FirstG1 && c <= LastG1) {
    c = G1[c - FirstG1];
  }
  ChBuffer[nCharInBuffer] = c;
  if (++nCharInBuffer == BUFFER_SIZE) {
    FlushBuffer();
  }
}

//-----------------------------------------------------------------------------
//   SendSequence(std::wstring_view  seq )
// Send the string to the input buffer.
//-----------------------------------------------------------------------------

void WinAnsiWriter::SendSequence(std::wstring_view seq) {
  DWORD out;
  INPUT_RECORD in;
  HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

  in.EventType = KEY_EVENT;
  in.Event.KeyEvent.bKeyDown = TRUE;
  in.Event.KeyEvent.wRepeatCount = 1;
  in.Event.KeyEvent.wVirtualKeyCode = 0;
  in.Event.KeyEvent.wVirtualScanCode = 0;
  in.Event.KeyEvent.dwControlKeyState = 0;
  for (auto ch : seq) {
    in.Event.KeyEvent.uChar.UnicodeChar = ch;
    WriteConsoleInput(hStdIn, &in, 1, &out);
  }
}

//-----------------------------------------------------------------------------
//   InterpretEscSeq()
// Interprets the last escape sequence scanned by ParseAndPrintANSIString
//   prefix             escape sequence prefix
//   es_argc            escape sequence args count
//   es_argv[]          escape sequence args array
//   suffix             escape sequence suffix
//
// for instance, with \e[33;45;1m we have
// prefix = '[',
// es_argc = 3, es_argv[0] = 33, es_argv[1] = 45, es_argv[2] = 1
// suffix = 'm'
//-----------------------------------------------------------------------------

void WinAnsiWriter::InterpretEscSeq() {
  int i{0};
  WORD attribut{0};
  CONSOLE_SCREEN_BUFFER_INFO Info{0};
  CONSOLE_CURSOR_INFO CursInfo{0};
  DWORD len{0};
  DWORD NumberOfCharsWritten{0};
  COORD Pos{0};
  SMALL_RECT Rect{0};
  CHAR_INFO CharInfo{0};

  if (prefix == '[') {
    if (prefix2 == '?' && (suffix == 'h' || suffix == 'l')) {
      if (es_argc == 1 && es_argv[0] == 25) {
        GetConsoleCursorInfo(hConOut, &CursInfo);
        CursInfo.bVisible = (suffix == 'h');
        SetConsoleCursorInfo(hConOut, &CursInfo);
        return;
      }
    }
    // Ignore any other \e[? or \e[> sequences.
    if (prefix2 != 0) {
      return;
    }

    GetConsoleScreenBufferInfo(hConOut, &Info);
    switch (suffix) {
    case 'm':
      if (es_argc == 0) {
        es_argv[es_argc++] = 0;
      }
      for (i = 0; i < es_argc; i++) {
        if (30 <= es_argv[i] && es_argv[i] <= 37) {
          grm.foreground = es_argv[i] - 30;
        } else if (40 <= es_argv[i] && es_argv[i] <= 47) {
          grm.background = es_argv[i] - 40;
        } else
          switch (es_argv[i]) {
          case 0:
          case 39:
          case 49: {
            wchar_t def[4];
            int a;
            *def = '7';
            def[1] = '\0';
            GetEnvironmentVariableW(L"ANSICON_DEF", def, lenof(def));
            a = wcstol(def, NULL, 16);
            grm.reverse = FALSE;
            if (a < 0) {
              grm.reverse = TRUE;
              a = -a;
            }
            if (es_argv[i] != 49)
              grm.foreground = attr2ansi[a & 7];
            if (es_argv[i] != 39)
              grm.background = attr2ansi[(a >> 4) & 7];
            if (es_argv[i] == 0) {
              if (es_argc == 1) {
                grm.bold = a & FOREGROUND_INTENSITY;
                grm.underline = a & BACKGROUND_INTENSITY;
              } else {
                grm.bold = 0;
                grm.underline = 0;
              }
              grm.rvideo = 0;
              grm.concealed = 0;
            }
          } break;

          case 1:
            grm.bold = FOREGROUND_INTENSITY;
            break;
          case 5: // blink
          case 4:
            grm.underline = BACKGROUND_INTENSITY;
            break;
          case 7:
            grm.rvideo = 1;
            break;
          case 8:
            grm.concealed = 1;
            break;
          case 21: // oops, this actually turns on double underline
          case 22:
            grm.bold = 0;
            break;
          case 25:
          case 24:
            grm.underline = 0;
            break;
          case 27:
            grm.rvideo = 0;
            break;
          case 28:
            grm.concealed = 0;
            break;
          }
      }
      if (grm.concealed) {
        if (grm.rvideo) {
          attribut =
              foregroundcolor[grm.foreground] | backgroundcolor[grm.foreground];
          if (grm.bold)
            attribut |= FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;
        } else {
          attribut =
              foregroundcolor[grm.background] | backgroundcolor[grm.background];
          if (grm.underline)
            attribut |= FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;
        }
      } else if (grm.rvideo) {
        attribut =
            foregroundcolor[grm.background] | backgroundcolor[grm.foreground];
        if (grm.bold)
          attribut |= BACKGROUND_INTENSITY;
        if (grm.underline)
          attribut |= FOREGROUND_INTENSITY;
      } else
        attribut = foregroundcolor[grm.foreground] | grm.bold |
                   backgroundcolor[grm.background] | grm.underline;
      if (grm.reverse)
        attribut = ((attribut >> 4) & 15) | ((attribut & 15) << 4);
      SetConsoleTextAttribute(hConOut, attribut);
      return;

    case 'J':
      if (es_argc == 0)
        es_argv[es_argc++] = 0; // ESC[J == ESC[0J
      if (es_argc != 1)
        return;
      switch (es_argv[0]) {
      case 0: // ESC[0J erase from cursor to end of display
        len = (Info.dwSize.Y - Info.dwCursorPosition.Y - 1) * Info.dwSize.X +
              Info.dwSize.X - Info.dwCursorPosition.X - 1;
        FillConsoleOutputCharacterW(hConOut, ' ', len, Info.dwCursorPosition,
                                    &NumberOfCharsWritten);
        FillConsoleOutputAttribute(hConOut, Info.wAttributes, len,
                                   Info.dwCursorPosition,
                                   &NumberOfCharsWritten);
        return;

      case 1: // ESC[1J erase from start to cursor.
        Pos.X = 0;
        Pos.Y = 0;
        len = Info.dwCursorPosition.Y * Info.dwSize.X +
              Info.dwCursorPosition.X + 1;
        FillConsoleOutputCharacterW(hConOut, ' ', len, Pos,
                                    &NumberOfCharsWritten);
        FillConsoleOutputAttribute(hConOut, Info.wAttributes, len, Pos,
                                   &NumberOfCharsWritten);
        return;

      case 2: // ESC[2J Clear screen and home cursor
        Pos.X = 0;
        Pos.Y = 0;
        len = Info.dwSize.X * Info.dwSize.Y;
        FillConsoleOutputCharacterW(hConOut, ' ', len, Pos,
                                    &NumberOfCharsWritten);
        FillConsoleOutputAttribute(hConOut, Info.wAttributes, len, Pos,
                                   &NumberOfCharsWritten);
        SetConsoleCursorPosition(hConOut, Pos);
        return;

      default:
        return;
      }

    case 'K':
      if (es_argc == 0)
        es_argv[es_argc++] = 0; // ESC[K == ESC[0K
      if (es_argc != 1)
        return;
      switch (es_argv[0]) {
      case 0: // ESC[0K Clear to end of line
        len = Info.dwSize.X - Info.dwCursorPosition.X + 1;
        FillConsoleOutputCharacterW(hConOut, ' ', len, Info.dwCursorPosition,
                                    &NumberOfCharsWritten);
        FillConsoleOutputAttribute(hConOut, Info.wAttributes, len,
                                   Info.dwCursorPosition,
                                   &NumberOfCharsWritten);
        return;

      case 1: // ESC[1K Clear from start of line to cursor
        Pos.X = 0;
        Pos.Y = Info.dwCursorPosition.Y;
        FillConsoleOutputCharacterW(hConOut, ' ', Info.dwCursorPosition.X + 1,
                                    Pos, &NumberOfCharsWritten);
        FillConsoleOutputAttribute(hConOut, Info.wAttributes,
                                   Info.dwCursorPosition.X + 1, Pos,
                                   &NumberOfCharsWritten);
        return;

      case 2: // ESC[2K Clear whole line.
        Pos.X = 0;
        Pos.Y = Info.dwCursorPosition.Y;
        FillConsoleOutputCharacterW(hConOut, ' ', Info.dwSize.X, Pos,
                                    &NumberOfCharsWritten);
        FillConsoleOutputAttribute(hConOut, Info.wAttributes, Info.dwSize.X,
                                   Pos, &NumberOfCharsWritten);
        return;

      default:
        return;
      }

    case 'X': // ESC[#X Erase # characters.
      if (es_argc == 0)
        es_argv[es_argc++] = 1; // ESC[X == ESC[1X
      if (es_argc != 1)
        return;
      FillConsoleOutputCharacterW(hConOut, ' ', es_argv[0],
                                  Info.dwCursorPosition, &NumberOfCharsWritten);
      FillConsoleOutputAttribute(hConOut, Info.wAttributes, es_argv[0],
                                 Info.dwCursorPosition, &NumberOfCharsWritten);
      return;

    case 'L': // ESC[#L Insert # blank lines.
      if (es_argc == 0) {
        es_argv[es_argc++] = 1; // ESC[L == ESC[1L
      }
      if (es_argc != 1) {
        return;
      }
      Rect.Left = 0;
      Rect.Top = Info.dwCursorPosition.Y;
      Rect.Right = Info.dwSize.X - 1;
      Rect.Bottom = Info.dwSize.Y - 1;
      Pos.X = 0;
      Pos.Y = Info.dwCursorPosition.Y + es_argv[0];
      CharInfo.Char.UnicodeChar = ' ';
      CharInfo.Attributes = Info.wAttributes;
      ScrollConsoleScreenBufferW(hConOut, &Rect, nullptr, Pos, &CharInfo);
      return;

    case 'M': // ESC[#M Delete # lines.
      if (es_argc == 0) {
        es_argv[es_argc++] = 1; // ESC[M == ESC[1M
      }
      if (es_argc != 1) {
        return;
      }
      if (es_argv[0] > Info.dwSize.Y - Info.dwCursorPosition.Y) {
        es_argv[0] = Info.dwSize.Y - Info.dwCursorPosition.Y;
      }
      Rect.Left = 0;
      Rect.Top = Info.dwCursorPosition.Y + es_argv[0];
      Rect.Right = Info.dwSize.X - 1;
      Rect.Bottom = Info.dwSize.Y - 1;
      Pos.X = 0;
      Pos.Y = Info.dwCursorPosition.Y;
      CharInfo.Char.UnicodeChar = ' ';
      CharInfo.Attributes = Info.wAttributes;
      ScrollConsoleScreenBuffer(hConOut, &Rect, NULL, Pos, &CharInfo);
      return;

    case 'P': // ESC[#P Delete # characters.
      if (es_argc == 0) {
        es_argv[es_argc++] = 1; // ESC[P == ESC[1P
      }
      if (es_argc != 1) {
        return;
      }
      if (Info.dwCursorPosition.X + es_argv[0] > Info.dwSize.X - 1) {
        es_argv[0] = Info.dwSize.X - Info.dwCursorPosition.X;
      }
      Rect.Left = Info.dwCursorPosition.X + es_argv[0];
      Rect.Top = Info.dwCursorPosition.Y;
      Rect.Right = Info.dwSize.X - 1;
      Rect.Bottom = Info.dwCursorPosition.Y;
      CharInfo.Char.UnicodeChar = ' ';
      CharInfo.Attributes = Info.wAttributes;
      ScrollConsoleScreenBuffer(hConOut, &Rect, NULL, Info.dwCursorPosition,
                                &CharInfo);
      return;

    case '@': // ESC[#@ Insert # blank characters.
      if (es_argc == 0) {
        es_argv[es_argc++] = 1; // ESC[@ == ESC[1@
      }
      if (es_argc != 1) {
        return;
      }
      if (Info.dwCursorPosition.X + es_argv[0] > Info.dwSize.X - 1) {
        es_argv[0] = Info.dwSize.X - Info.dwCursorPosition.X;
      }
      Rect.Left = Info.dwCursorPosition.X;
      Rect.Top = Info.dwCursorPosition.Y;
      Rect.Right = Info.dwSize.X - 1 - es_argv[0];
      Rect.Bottom = Info.dwCursorPosition.Y;
      Pos.X = Info.dwCursorPosition.X + es_argv[0];
      Pos.Y = Info.dwCursorPosition.Y;
      CharInfo.Char.UnicodeChar = ' ';
      CharInfo.Attributes = Info.wAttributes;
      ScrollConsoleScreenBuffer(hConOut, &Rect, NULL, Pos, &CharInfo);
      return;

    case 'k': // ESC[#k
    case 'A': // ESC[#A Moves cursor up # lines
      if (es_argc == 0) {
        es_argv[es_argc++] = 1; // ESC[A == ESC[1A
      }
      if (es_argc != 1) {
        return;
      }
      Pos.Y = Info.dwCursorPosition.Y - es_argv[0];
      if (Pos.Y < 0) {
        Pos.Y = 0;
      }
      Pos.X = Info.dwCursorPosition.X;
      SetConsoleCursorPosition(hConOut, Pos);
      return;

    case 'e': // ESC[#e
    case 'B': // ESC[#B Moves cursor down # lines
      if (es_argc == 0) {
        es_argv[es_argc++] = 1; // ESC[B == ESC[1B
      }
      if (es_argc != 1) {
        return;
      }
      Pos.Y = Info.dwCursorPosition.Y + es_argv[0];
      if (Pos.Y >= Info.dwSize.Y) {
        Pos.Y = Info.dwSize.Y - 1;
      }
      Pos.X = Info.dwCursorPosition.X;
      SetConsoleCursorPosition(hConOut, Pos);
      return;

    case 'a': // ESC[#a
    case 'C': // ESC[#C Moves cursor forward # spaces
      if (es_argc == 0) {
        es_argv[es_argc++] = 1; // ESC[C == ESC[1C
      }
      if (es_argc != 1) {
        return;
      }
      Pos.X = Info.dwCursorPosition.X + es_argv[0];
      if (Pos.X >= Info.dwSize.X) {
        Pos.X = Info.dwSize.X - 1;
      }
      Pos.Y = Info.dwCursorPosition.Y;
      SetConsoleCursorPosition(hConOut, Pos);
      return;

    case 'j': // ESC[#j
    case 'D': // ESC[#D Moves cursor back # spaces
      if (es_argc == 0) {
        es_argv[es_argc++] = 1; // ESC[D == ESC[1D
      }
      if (es_argc != 1) {
        return;
      }
      Pos.X = Info.dwCursorPosition.X - es_argv[0];
      if (Pos.X < 0) {
        Pos.X = 0;
      }
      Pos.Y = Info.dwCursorPosition.Y;
      SetConsoleCursorPosition(hConOut, Pos);
      return;

    case 'E': // ESC[#E Moves cursor down # lines, column 1.
      if (es_argc == 0)
        es_argv[es_argc++] = 1; // ESC[E == ESC[1E
      if (es_argc != 1) {
        return;
      }
      Pos.Y = Info.dwCursorPosition.Y + es_argv[0];
      if (Pos.Y >= Info.dwSize.Y)
        Pos.Y = Info.dwSize.Y - 1;
      Pos.X = 0;
      SetConsoleCursorPosition(hConOut, Pos);
      return;

    case 'F': // ESC[#F Moves cursor up # lines, column 1.
      if (es_argc == 0) {
        es_argv[es_argc++] = 1; // ESC[F == ESC[1F
      }
      if (es_argc != 1) {
        return;
      }
      Pos.Y = Info.dwCursorPosition.Y - es_argv[0];
      if (Pos.Y < 0) {
        Pos.Y = 0;
      }
      Pos.X = 0;
      SetConsoleCursorPosition(hConOut, Pos);
      return;

    case '`': // ESC[#`
    case 'G': // ESC[#G Moves cursor column # in current row.
      if (es_argc == 0) {
        es_argv[es_argc++] = 1; // ESC[G == ESC[1G
      }
      if (es_argc != 1) {
        return;
      }
      Pos.X = es_argv[0] - 1;
      if (Pos.X >= Info.dwSize.X) {
        Pos.X = Info.dwSize.X - 1;
      }
      if (Pos.X < 0) {
        Pos.X = 0;
      }
      Pos.Y = Info.dwCursorPosition.Y;
      SetConsoleCursorPosition(hConOut, Pos);
      return;

    case 'd': // ESC[#d Moves cursor row #, current column.
      if (es_argc == 0) {
        es_argv[es_argc++] = 1; // ESC[d == ESC[1d
      }
      if (es_argc != 1) {
        return;
      }
      Pos.Y = es_argv[0] - 1;
      if (Pos.Y < 0) {
        Pos.Y = 0;
      }
      if (Pos.Y >= Info.dwSize.Y) {
        Pos.Y = Info.dwSize.Y - 1;
      }
      SetConsoleCursorPosition(hConOut, Pos);
      return;

    case 'f': // ESC[#;#f
    case 'H': // ESC[#;#H Moves cursor to line #, column #
      if (es_argc == 0) {
        es_argv[es_argc++] = 1; // ESC[H == ESC[1;1H
      }
      if (es_argc == 1) {
        es_argv[es_argc++] = 1; // ESC[#H == ESC[#;1H
      }
      if (es_argc > 2) {
        return;
      }
      Pos.X = es_argv[1] - 1;
      if (Pos.X < 0) {
        Pos.X = 0;
      }
      if (Pos.X >= Info.dwSize.X) {
        Pos.X = Info.dwSize.X - 1;
      }
      Pos.Y = es_argv[0] - 1;
      if (Pos.Y < 0) {
        Pos.Y = 0;
      }
      if (Pos.Y >= Info.dwSize.Y) {
        Pos.Y = Info.dwSize.Y - 1;
      }
      SetConsoleCursorPosition(hConOut, Pos);
      return;

    case 's': // ESC[s Saves cursor position for recall later
      if (es_argc != 0) {
        return;
      }
      SavePos = Info.dwCursorPosition;
      return;

    case 'u': // ESC[u Return to saved cursor position
      if (es_argc != 0) {
        return;
      }
      SetConsoleCursorPosition(hConOut, SavePos);
      return;

    case 'n': // ESC[#n Device status report
      if (es_argc != 1) {
        return; // ESC[n == ESC[0n -> ignored
      }
      switch (es_argv[0]) {
      case 5:                    // ESC[5n Report status
        SendSequence(L"\33[0n"); // "OK"
        return;

      case 6: {
        // ESC[6n Report cursor position
        auto buf = bela::StringCat(L"\33[", Info.dwCursorPosition.Y + 1, L";",
                                   Info.dwCursorPosition.X + 1, L"R");
        SendSequence(buf);
      }
        return;

      default:
        return;
      }

    case 't': // ESC[#t Window manipulation
      if (es_argc != 1) {
        return;
      }
      if (es_argv[0] == 21) // ESC[21t Report xterm window's title
      {
        wchar_t buf[MAX_PATH * 2];
        DWORD tlen = GetConsoleTitleW(buf + 3, lenof(buf) - 3 - 2);
        // Too bad if it's too big or fails.
        buf[0] = ESC;
        buf[1] = ']';
        buf[2] = 'l';
        buf[3 + tlen] = ESC;
        buf[3 + tlen + 1] = '\\';
        buf[3 + tlen + 2] = '\0';
        SendSequence(buf);
      }
      return;

    default:
      return;
    }
  } else {
    // (prefix == ']')
    // Ignore any \e]? or \e]> sequences.
    if (prefix2 != 0) {
      return;
    }

    if (es_argc == 1 && es_argv[0] == 0) {
      // ESC]0;titleST
      SetConsoleTitle(Pt_arg);
    }
  }
}

ssize_t WinAnsiWriter::WriteAnsi(HANDLE hDev, std::wstring_view msg) {
  if (hDev != hConOut) {
    hConOut = hDev;
    state = 1;
    shifted = false;
  }
  size_t i = msg.size();
  auto s = msg.data();
  for (; i > 0; i--, s++) {
    if (state == 1) {
      switch (*s) {
      case ESC:
        state = 2;
        break;
      case SO:
        shifted = true;
        break;
      case SI:
        shifted = false;
        break;
      default:
        PushBuffer(*s);
        break;
      }
      continue;
    }
    if (state == 2) {
      switch (*s) {
      case ESC:
        break;
      case '[':
        [[fallthrough]];
      case ']':
        FlushBuffer();
        prefix = *s;
        prefix2 = 0;
        state = 3;
        Pt_len = 0;
        *Pt_arg = '\0';
        break;
      case '(':
        [[fallthrough]];
      case ')':
        state = 6;
        break;
      default:
        state = 1;
        break;
      }
      continue;
    }
    if (state == 3) {
      if (is_digit(*s)) {
        es_argc = 0;
        es_argv[0] = *s - '0';
        state = 4;
      } else if (*s == ';') {
        es_argc = 1;
        es_argv[0] = 0;
        es_argv[1] = 0;
        state = 4;
      } else if (*s == '?' || *s == '>') {
        prefix2 = *s;
      } else {
        es_argc = 0;
        suffix = *s;
        InterpretEscSeq();
        state = 1;
      }
      continue;
    }
    if (state == 4) {
      if (is_digit(*s)) {
        es_argv[es_argc] = 10 * es_argv[es_argc] + (*s - '0');
        continue;
      }
      if (*s == ';') {
        if (es_argc < MAX_ARG - 1) {
          es_argc++;
        }
        es_argv[es_argc] = 0;
        if (prefix == ']') {
          state = 5;
        }
        continue;
      }
      es_argc++;
      suffix = *s;
      InterpretEscSeq();
      state = 1;
      continue;
    }
    if (state == 5) {
      if (*s == BEL) {
        Pt_arg[Pt_len] = '\0';
        InterpretEscSeq();
        state = 1;
      } else if (*s == '\\' && Pt_len > 0 && Pt_arg[Pt_len - 1] == ESC) {
        Pt_arg[--Pt_len] = '\0';
        InterpretEscSeq();
        state = 1;
      } else if (static_cast<size_t>(Pt_len) < lenof(Pt_arg) - 1) {
        Pt_arg[Pt_len++] = *s;
      }
      continue;
    }
    if (state == 6) {
      // Ignore it (ESC ) 0 is implicit; nothing else is supported).
      state = 1;
    }
  }
  FlushBuffer();
  return static_cast<ssize_t>(msg.size()) - i;
}

ssize_t BelaWriteAnsi(HANDLE hDev, std::wstring_view msg) {
  return WinAnsiWriter::Inst().WriteAnsi(hDev, msg);
}

} // namespace bela