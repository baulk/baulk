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

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
option(BUILD_TEST "build test" OFF)

set(BAULK_VERSION_MAJOR 1)
set(BAULK_VERSION_MINOR 0)
set(BAULK_VERSION_PATCH 0)
set(PACKAGE_VERSION "${BAULK_VERSION_MAJOR}.${BAULK_VERSION_MINOR}.${BAULK_VERSION_PATCH}")

set(CPACK_PACKAGE_NAME "Baulk")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Baulk")
set(CPACK_PACKAGE_VERSION_MAJOR ${BAULK_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${BAULK_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${BAULK_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
set(CPACK_PACKAGE_VENDOR "Baulk")
set(CPACK_PACKAGE_CONTACT "Baulk contributors")

include(CPack)

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

if(MSVC)
  add_compile_options("-D_CRT_SECURE_NO_WARNINGS=1")
  add_compile_options("-utf-8")
  add_compile_options("-permissive-")
  add_compile_options("-Zc:__cplusplus")
  add_compile_options("-DUNICODE=1")
  add_compile_options("-D_UNICODE=1")
  set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -c65001")
  set(WIN_COMPILER_FLAGS
      CMAKE_CXX_FLAGS
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_C_FLAGS
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_RELEASE)
  foreach(WIN_COMPILER_FLAG ${WIN_COMPILER_FLAGS})
    string(REPLACE "/MD" "/MT" ${WIN_COMPILER_FLAG} "${${WIN_COMPILER_FLAG}}")
  endforeach()
else()
  add_compile_options("-g")
  add_compile_options("-Wall")
  add_compile_options("-Wextra")
  if(USE_SANITIZE)
    add_compile_options("-fsanitize=address")
    add_compile_options("-fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address ")
  endif()
endif()

add_definitions(-D_UNICODE -DUNICODE)

include_directories("./include" "./vendor/bela/include")

add_subdirectory(vendor/bela)
add_subdirectory(vendor/blake3)
add_subdirectory(tools/baulk)
add_subdirectory(tools/baulkcli)

if(BUILD_TEST)
  add_subdirectory(test)
endif()
