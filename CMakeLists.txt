# Author: Force Charlie Copyright (C) 2020. Force Charlie. All Rights Reserved.
cmake_minimum_required(VERSION 3.15)
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

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY_DEFAULT "")
  # FIXME: This block should go away after a transition period.
  if(NOT CMAKE_VERSION VERSION_LESS 3.15)
    # Filter out MSVC runtime library flags that may have come from the cache of an existing build tree or from scripts.
    foreach(l C CXX)
      foreach(c DEBUG MINSIZEREL RELEASE RELWITHDEBINFO)
        string(REGEX REPLACE "[-/]M[DT]d?( |$)" "" "CMAKE_${l}_FLAGS_${c}" "${CMAKE_${l}_FLAGS_${c}}")
      endforeach()
    endforeach()
  endif()
  # http://stackoverflow.com/a/32128977/486990
  add_compile_options(
    "$<$<CONFIG:Debug>:/MTd>" # -MTd
    "$<$<CONFIG:RelWithDebInfo>:/MT>" # -MT
    "$<$<CONFIG:Release>:/MT>" # * MT
    "$<$<CONFIG:MinSizeRel>:/MT>")
  add_compile_options("-D_CRT_SECURE_NO_WARNINGS=1")
  add_compile_options("-utf-8")
  add_compile_options("-permissive-")
  add_compile_options("-Zc:__cplusplus")
  add_compile_options("-DUNICODE=1")
  add_compile_options("-D_UNICODE=1")
  add_compile_options("-W3")
  set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -c65001")
else()
  add_compile_options("-DUNICODE=1")
  add_compile_options("-D_UNICODE=1")
  add_compile_options("-g")
  add_compile_options("-Wall")
  add_compile_options("-Wextra")
  if(USE_SANITIZE)
    add_compile_options("-fsanitize=address")
    add_compile_options("-fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address ")
  endif()
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
option(BUILD_TEST "build test" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
# Gen version
include(VersionFromVCS)
get_source_info(${CMAKE_CURRENT_SOURCE_DIR} BAULK_REVISION BAULK_REMOTE_URL BAULK_REFNAME)
string(TIMESTAMP BAULK_COPYRIGHT_YEAR "%Y")

# BAULK_REFNAME maybe defined by ci
if("${BAULK_REFNAME}" STREQUAL "none")
  if(DEFINED ENV{BAULK_REFNAME})
    set(BAULK_REFNAME $ENV{BAULK_REFNAME})
  endif()
endif()

if("${BAULK_REFNAME}" MATCHES "refs/tags/*")
  string(TIMESTAMP BAULK_BUILD_TIME "%Y-%m-%dT%H:%M:%SZ")
else()
  set(BAULK_BUILD_TIME "none")
endif()

if(DEFINED ENV{GITHUB_RUN_NUMBER})
  set(BAULK_VERSION_BUILD $ENV{GITHUB_RUN_NUMBER})
  message(STATUS "baulk detect GITHUB_RUN_NUMBER: $ENV{GITHUB_RUN_NUMBER}")
else()
  set(BAULK_VERSION_BUILD 261)
endif()

set(BAULK_VERSION_MAJOR 1)
set(BAULK_VERSION_MINOR 6)
set(BAULK_VERSION_PATCH 6)
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
configure_file(${CMAKE_SOURCE_DIR}/include/baulkversion.h.cmake ${CMAKE_BINARY_DIR}/include/baulkversion.h)

set(CMAKE_CXX_STANDARD 17)
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

include_directories("${CMAKE_BINARY_DIR}/include" "./include" "./vendor/bela/include")

configure_file("config/baulk.json" "${PROJECT_BINARY_DIR}/config/baulk.json" COPYONLY)
install(FILES "config/baulk.json" DESTINATION config)
install(FILES "script/installmenu.bat" DESTINATION script)
install(FILES "script/installmenu.ps1" DESTINATION script)
install(FILES "script/uninstallmenu.bat" DESTINATION script)
install(FILES "script/uninstallmenu.ps1" DESTINATION script)

install(FILES "script/arm64/installmenu.bat" DESTINATION script/arm64)
install(FILES "script/arm64/installmenu.ps1" DESTINATION script/arm64)
install(FILES "script/arm64/uninstallmenu.bat" DESTINATION script/arm64)
install(FILES "script/arm64/uninstallmenu.ps1" DESTINATION script/arm64)

add_subdirectory(vendor/bela)
add_subdirectory(lib/archive)
add_subdirectory(lib/env)
add_subdirectory(tools)

if(BUILD_TEST)
  add_subdirectory(test)
endif()
