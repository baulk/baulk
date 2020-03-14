////////////
#ifndef BAULK_VERSION_H
#define BAULK_VERSION_H

#ifdef APPVEYOR_BUILD_NUMBER
#define BAULK_BUILD_NUMBER APPVEYOR_BUILD_NUMBER
#else
#define BAULK_BUILD_NUMBER 1
#endif

#define TOSTR_1(x) L#x
#define TOSTR(x) TOSTR_1(x)

#define BAULKSUBVER TOSTR(BAULK_BUILD_NUMBER)

#define BAULK_MAJOR_VERSION 1
#define BAULK_MINOR_VERSION 0
#define BAULK_PATCH_VERSION 0

#define BAULK_MAJOR TOSTR(BAULK_MAJOR_VERSION)
#define BAULK_MINOR TOSTR(BAULK_MINOR_VERSION)
#define BAULK_PATCH TOSTR(BAULK_PATCH_VERSION)

#define BAULK_VERSION_MAIN BAULK_MAJOR L"." BAULK_MINOR
#define BAULK_VERSION_FULL BAULK_VERSION_MAIN L"." BAULK_PATCH

#ifdef APPVEYOR_BUILD_NUMBER
#define BAULK_BUILD_VERSION BAULK_VERSION_FULL L"." BAULKSUBVER L" (appveyor)"
#else
#define BAULK_BUILD_VERSION BAULK_VERSION_FULL L"." BAULKSUBVER L" (dev)"
#endif

#define BAULK_APPLINK                                                          \
  L"For more information about this tool. \nVisit: <a "                        \
  L"href=\"https://github.com/baulk/baulk\">BAULK</"                           \
  L"a>\nVisit: <a "                                                            \
  L"href=\"https://forcemz.net/\">forcemz.net</a>"

#define BAULK_APPLINKE                                                         \
  L"Ask for help with this issue. \nVisit: <a "                                \
  L"href=\"https://github.com/baulk/baulk/issues\">BAULK "                     \
  L"Issues</a>"

#define BAULK_APPVERSION                                                       \
  L"Version: " BAULK_VERSION_FULL L"\nCopyright \xA9 2020, Baulk "             \
  L"contributors."

#define BAULKUI_COPYRIGHT L"Copyright \xA9 2020, Baulk contributors."

#endif
