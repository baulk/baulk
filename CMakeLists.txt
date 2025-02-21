# Copyright © 2024. Baulk contributors. All Rights Reserved.
cmake_minimum_required(VERSION 3.27)
project(Baulk)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE)
  message(
    FATAL_ERROR
      "In-source builds are not allowed.
CMake would overwrite the makefiles distributed with Baulk.
Please create a directory and run cmake from there, passing the path
to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
Please delete them.")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(BAULK_ENABLE_LTO OFF)
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error)
if(lto_supported)
  message(STATUS "IPO/LTO supported")
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(BAULK_ENABLE_LTO ON)
    set(BELA_ENABLE_LTO ON)
    message(STATUS "IPO/LTO enabled")
  endif()
endif()

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY_DEFAULT "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS=1 -utf-8 -W3 -DUNICODE=1 -D_UNICODE=1 -wd26812")
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS=1 -utf-8 -W3 -permissive- -Zc:__cplusplus -DUNICODE=1 -D_UNICODE=1 -wd26812"
  )
  set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -c65001")
endif(MSVC)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
option(BUILD_TEST "build test" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
# Gen version
include(VersionFromVCS)
get_source_info(${CMAKE_CURRENT_SOURCE_DIR} BAULK_REVISION BAULK_REMOTE_URL BAULK_REFNAME)
string(TIMESTAMP BAULK_COPYRIGHT_YEAR "%Y")

# replace to GITHUB_REF
if(DEFINED ENV{GITHUB_REF})
  set(BAULK_REFNAME $ENV{GITHUB_REF})
endif()

message(STATUS "BAULK_REFNAME: ${BAULK_REFNAME}")

if("$ENV{GITHUB_REF_TYPE}" MATCHES "tag")
  string(TIMESTAMP BAULK_BUILD_TIME "%Y-%m-%dT%H:%M:%SZ")
else()
  set(BAULK_BUILD_TIME "none")
endif()

message(STATUS "baulk build time: ${BAULK_BUILD_TIME}")

if(DEFINED ENV{GITHUB_RUN_NUMBER})
  set(BAULK_VERSION_BUILD $ENV{GITHUB_RUN_NUMBER})
  message(STATUS "baulk detect GITHUB_RUN_NUMBER: $ENV{GITHUB_RUN_NUMBER}")
else()
  set(BAULK_VERSION_BUILD 520)
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/VERSION")
  file(READ "${CMAKE_CURRENT_LIST_DIR}/VERSION" BAULK_LOCAL_VERSION)
  message(STATUS "baulk detect VERSION: ${BAULK_LOCAL_VERSION}")
  string(REPLACE "." ";" VERSION_LIST ${BAULK_LOCAL_VERSION})
  list(GET VERSION_LIST 0 BAULK_VERSION_MAJOR)
  list(GET VERSION_LIST 1 BAULK_VERSION_MINOR)
  list(GET VERSION_LIST 2 BAULK_VERSION_PATCH)
  message(STATUS "baulk detect ${BAULK_VERSION_MAJOR}.${BAULK_VERSION_MINOR}.${BAULK_VERSION_PATCH}")
else()
  set(BAULK_VERSION_MAJOR 5)
  set(BAULK_VERSION_MINOR 1)
  set(BAULK_VERSION_PATCH 0)
endif()

set(PACKAGE_VERSION "${BAULK_VERSION_MAJOR}.${BAULK_VERSION_MINOR}.${BAULK_VERSION_PATCH}")

string(TOLOWER "${CMAKE_C_COMPILER_ARCHITECTURE_ID}" BAULK_ARCH_NAME)
set(CPACK_SYSTEM_NAME "win-${BAULK_ARCH_NAME}")
set(CPACK_PACKAGE_NAME "Baulk")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Baulk")
set(CPACK_PACKAGE_VERSION_MAJOR ${BAULK_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${BAULK_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${BAULK_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
set(CPACK_PACKAGE_VENDOR "Baulk")
set(CPACK_PACKAGE_CONTACT "Baulk contributors")

include(CPack)
# Generate version code
configure_file(${CMAKE_SOURCE_DIR}/include/version.h.cmake ${CMAKE_BINARY_DIR}/include/version.h)

include_directories("${CMAKE_BINARY_DIR}/include" "./include" "./vendor/bela/include")

configure_file("config/baulk.json" "${PROJECT_BINARY_DIR}/config/baulk.json" COPYONLY)
install(FILES "config/baulk.json" DESTINATION config)
install(
  FILES "res/screw-driver.ico"
  DESTINATION "share/baulk"
  RENAME baulk.ico)

install(FILES "script/fragments.ps1" DESTINATION script)
install(FILES "script/fragments.bat" DESTINATION script)
install(FILES "script/fragments-arm64.ps1" DESTINATION script)
install(FILES "script/fragments-arm64.bat" DESTINATION script)
install(FILES "script/fragments-delete.bat" DESTINATION script)
install(FILES "script/register-unscrew.ps1" DESTINATION script)
install(FILES "script/register-unscrew.bat" DESTINATION script)
install(FILES "script/unregister-unscrew.ps1" DESTINATION script)
install(FILES "script/unregister-unscrew.bat" DESTINATION script)

add_subdirectory(vendor/bela)
add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(extension)

if(BUILD_TEST)
  add_subdirectory(test)
endif()
