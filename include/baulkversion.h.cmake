// Generate code by cmake, don't modify
#ifndef BAULK_VERSION_H
#define BAULK_VERSION_H

#define BAULK_VERSION_MAJOR ${BAULK_VERSION_MAJOR}
#define BAULK_VERSION_MINOR ${BAULK_VERSION_MINOR}
#define BAULK_VERSION_PATCH ${BAULK_VERSION_PATCH}
#define BAULK_VERSION_BUILD ${BAULK_VERSION_BUILD}

#define BAULK_VERSION L"${BAULK_VERSION_MAJOR}.${BAULK_VERSION_MINOR}.${BAULK_VERSION_PATCH}"
#define BAULK_REVISION L"${BAULK_REVISION}"
#define BAULK_REFNAME L"${BAULK_REFNAME}"
#define BAULK_BUILD_TIME L"${BAULK_BUILD_TIME}"
#define BAULK_REMOTE_URL L"${BAULK_REMOTE_URL}"

#define BAULK_APPLINK                                                                              \
  L"For more information about this tool. \nVisit: <a "                                            \
  L"href=\"https://github.com/baulk/baulk\">Baulk</"                                               \
  L"a>\nVisit: <a "                                                                                \
  L"href=\"https://forcemz.net/\">forcemz.net</a>"

#define BAULK_APPLINKE                                                                             \
  L"Ask for help with this issue. \nVisit: <a "                                                    \
  L"href=\"https://github.com/baulk/baulk/issues\">Baulk "                                         \
  L"Issues</a>"

#define BAULK_APPVERSION                                                                           \
  L"Version: ${BAULK_VERSION_MAJOR}.${BAULK_VERSION_MINOR}.${BAULK_VERSION_PATCH}\nCopyright "     \
  L"\xA9 ${BAULK_COPYRIGHT_YEAR}, Baulk contributors."

#define BAULKUI_COPYRIGHT L"Copyright \xA9 ${BAULK_COPYRIGHT_YEAR}, Baulk contributors."

#endif