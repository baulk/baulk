# cmake

list(
  APPEND
  SRC_ZLIB
  "adler32.c"
  "chromeconf.h"
  "compress.c"
  "contrib/optimizations/insert_string.h"
  "cpu_features.c"
  "cpu_features.h"
  "crc32.c"
  "crc32.h"
  "deflate.c"
  "deflate.h"
  "gzclose.c"
  "gzguts.h"
  "gzlib.c"
  "gzread.c"
  "gzwrite.c"
  "infback.c"
  "inffast.c"
  "inffast.h"
  "inffixed.h"
  "inflate.h"
  "inftrees.c"
  "inftrees.h"
  "trees.c"
  "trees.h"
  "uncompr.c"
  "zconf.h"
  "zlib.h"
  "zutil.c"
  "zutil.h")

if("${BAULK_ARCH_NAME}" STREQUAL "x86_64"
   OR "${BAULK_ARCH_NAME}" STREQUAL "amd64"
   OR "${BAULK_ARCH_NAME}" STREQUAL "x64"
   OR "${BAULK_ARCH_NAME}" STREQUAL "x86")
  list(
    APPEND
    SRC_ZLIB
    "adler32_simd.c"
    "adler32_simd.h"
    "contrib/optimizations/chunkcopy.h"
    "contrib/optimizations/inffast_chunk.c"
    "contrib/optimizations/inffast_chunk.h"
    "contrib/optimizations/inflate.c"
    "crc32_simd.c"
    "crc32_simd.h"
    "crc_folding.c"
    "fill_window_sse.c")
elseif("${BAULK_ARCH_NAME}" STREQUAL "arm64")
  list(
    APPEND
    SRC_ZLIB
    "contrib/optimizations/chunkcopy.h"
    "contrib/optimizations/inffast_chunk.c"
    "contrib/optimizations/inffast_chunk.h"
    "contrib/optimizations/inflate.c")
endif()

add_library(zlib STATIC ${SRC_ZLIB})

if("${BAULK_ARCH_NAME}" STREQUAL "x86_64"
   OR "${BAULK_ARCH_NAME}" STREQUAL "amd64"
   OR "${BAULK_ARCH_NAME}" STREQUAL "x64"
   OR "${BAULK_ARCH_NAME}" STREQUAL "x86")
  target_compile_definitions(
    zlib
    PRIVATE ZLIB_IMPLEMENTATION
            X86_WINDOWS
            ADLER32_SIMD_SSSE3
            CRC32_SIMD_SSE42_PCLMUL
            DEFLATE_FILL_WINDOW_SSE2
            INFLATE_CHUNK_READ_64LE
            INFLATE_CHUNK_SIMD_SSE2)
elseif("${BAULK_ARCH_NAME}" STREQUAL "arm64")
  target_compile_definitions(zlib PRIVATE ZLIB_IMPLEMENTATION ARMV8_OS_WINDOWS INFLATE_CHUNK_SIMD_NEON)
endif()

target_include_directories(zlib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
