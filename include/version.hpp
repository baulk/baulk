///
#ifndef BAULK_BAULKREV_HPP
#define BAULK_BAULKREV_HPP

#ifdef __has_include
#if __has_include(<version.h>)
#include <version.h>
#endif
#endif

#ifndef BAULK_VERSION_MAJOR
#define BAULK_VERSION_MAJOR 4
#endif

#ifndef BAULK_VERSION_MINOR
#define BAULK_VERSION_MINOR 0
#endif

#ifndef BAULK_VERSION_PATCH
#define BAULK_VERSION_PATCH 0
#endif

#ifndef BAULK_VERSION_BUILD
#define BAULK_VERSION_BUILD 0
#endif

#ifndef BAULK_VERSION
#define BAULK_VERSION L"4.0.0"
#endif

#ifndef BAULK_REVISION
#define BAULK_REVISION L"none"
#endif

#ifndef BAULK_REFNAME
#define BAULK_REFNAME L"none"
#endif

#ifndef BAULK_BUILD_TIME
#define BAULK_BUILD_TIME L"none"
#endif

#ifndef BAULK_REMOTE_URL
#define BAULK_REMOTE_URL L"none"
#endif

#ifndef BAULK_APPLINK
#define BAULK_APPLINK                                                                                                  \
  L"For more information about this tool. \nVisit: <a "                                                                \
  L"href=\"https://github.com/baulk/baulk\">Baulk</"                                                                   \
  L"a>\nVisit: <a "                                                                                                    \
  L"href=\"https://forcemz.net/\">forcemz.net</a>"
#endif

#ifndef BAULK_APPLINKE
#define BAULK_APPLINKE                                                                                                 \
  L"Ask for help with this issue. \nVisit: <a "                                                                        \
  L"href=\"https://github.com/baulk/baulk/issues\">Baulk "                                                             \
  L"Issues</a>"
#endif

#ifndef BAULK_APPVERSION
#define BAULK_APPVERSION L"Version: 4.0.0\nCopyright \xA9 2022, Baulk contributors."
#endif

#ifndef BAULK_COPYRIGHT
#define BAULK_COPYRIGHT L"Copyright \xA9 2022, Baulk contributors."
#endif

#endif