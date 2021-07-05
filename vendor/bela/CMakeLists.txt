# top cmake
cmake_minimum_required(VERSION 3.18)

project(bela CXX C ASM)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE)
  message(
    FATAL_ERROR
      "In-source builds are not allowed.
CMake would overwrite the makefiles distributed with bela.
Please create a directory and run cmake from there, passing the path
to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
Please delete them.")
endif()

option(ENABLE_TEST "Enable test" OFF)
option(BELA_ENABLE_LTO "bela enable LTO" OFF)

message(STATUS "CMAKE_ASM_COMPILER_ID ${CMAKE_ASM_COMPILER_ID}")

if(NOT (DEFINED CMAKE_CXX_STANDARD))
  set(CMAKE_CXX_STANDARD 20) # /std:c++latest
endif()

if(CMAKE_CXX_STANDARD LESS 20 OR CMAKE_CXX_STANDARD STREQUAL "98")
  message(FATAL_ERROR "Bela requires C++20 or later")
endif()

set(CMAKE_CXX_STANDARD_REQUIRED YES)

if("^${CMAKE_SOURCE_DIR}$" STREQUAL "^${PROJECT_SOURCE_DIR}$")
  set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
  set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
endif()

if(MSVC)
  add_compile_options("-utf-8")
  add_compile_options("-permissive-")
  add_compile_options("-Zc:__cplusplus")
  add_compile_options("-W3")
  add_compile_options("-DUNICODE=1")
  add_compile_options("-D_UNICODE=1")
  add_compile_options("-wd26812")
endif(MSVC)

include_directories(./include)

add_subdirectory(src/bela)
add_subdirectory(src/belawin)
add_subdirectory(src/belashl)
add_subdirectory(src/belatime)
add_subdirectory(src/belaund)
add_subdirectory(src/belahash)
add_subdirectory(src/hazel)
if(ENABLE_TEST)
  add_subdirectory(test)
endif()
