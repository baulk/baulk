# bela sources
cmake_minimum_required(VERSION 3.31)

project(bela)

include(FeatureSummary)

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

option(ENABLE_BELA_TEST "Enable test" OFF)
option(BELA_ENABLE_LTO "bela enable LTO" OFF)
option(BELA_ENABLE_ASSEMBLY_FILES "bela enable assembly files" OFF)

message(STATUS "CMAKE_ASM_COMPILER_ID ${CMAKE_ASM_COMPILER_ID}")

if(NOT (DEFINED CMAKE_CXX_STANDARD))
  set(CMAKE_CXX_STANDARD 23)
endif()

if(CMAKE_CXX_STANDARD LESS 23 OR CMAKE_CXX_STANDARD STREQUAL "98")
  message(FATAL_ERROR "Bela requires C++23 or later")
endif()

set(CMAKE_CXX_STANDARD_REQUIRED YES)

if("^${CMAKE_SOURCE_DIR}$" STREQUAL "^${PROJECT_SOURCE_DIR}$")
  set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
  set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
  if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -utf-8 -W3 -DUNICODE=1 -D_UNICODE=1 -wd26812")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -utf-8 -W3 -permissive- -Zc:__cplusplus -DUNICODE=1 -D_UNICODE=1 -wd26812")
  endif(MSVC)
endif()

include(CheckSymbolExists)

if(MSVC)
  check_symbol_exists(_M_X64 "" BELA_ARCHITECTURE_64BIT)
  check_symbol_exists(_M_IX86 "" BELA_ARCHITECTURE_32BIT)
  check_symbol_exists(_M_ARM64 "" BELA_ARCHITECTURE_ARM64)
else()
  check_symbol_exists(__x86_64__ "" BELA_ARCHITECTURE_64BIT)
  check_symbol_exists(__aarch64__ "" BELA_ARCHITECTURE_ARM64)
endif()

include_directories(include)

add_subdirectory(src/bela)
add_subdirectory(src/belawin)
add_subdirectory(src/belashl)
add_subdirectory(src/belatime)
add_subdirectory(src/belaund)
add_subdirectory(src/belahash)
add_subdirectory(src/hazel)
if(ENABLE_BELA_TEST)
  add_subdirectory(test)
endif()
