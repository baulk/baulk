# Baulk archive code
# ----------------------------
# bzip2 support
file(GLOB BZIP2_SOURCES bzip2/*.c)
add_library(bzip2 STATIC ${BZIP2_SOURCES})

# brotli
file(
  GLOB
  BROTLI_SOURCES
  brotli/common/*.c
  brotli/dec/*.c
  brotli/enc/*.c)
add_library(brotli STATIC ${BROTLI_SOURCES})
target_include_directories(brotli PRIVATE brotli/include)

# ced
add_subdirectory(ced)

# zlib: use zlib-ng

file(
  GLOB
  ZLIB_GENERIC_SOURCES
  zlib/arch/generic/*.c)

file(
  GLOB
  ZLIB_X86_SOURCES
  zlib/arch/x86/*.c)

file(
  GLOB
  ZLIB_ARM_SOURCES
  zlib/arch/arm/*.c)

list(
  APPEND
  SRC_ZLIB
	"zlib/adler32.c"
	"zlib/compress.c"
	"zlib/cpu_features.c"
	"zlib/crc32_braid_comb.c"
  "zlib/crc32.c"
	"zlib/deflate.c"
	"zlib/deflate_fast.c"
	"zlib/deflate_huff.c"
	"zlib/deflate_quick.c"
	"zlib/deflate_medium.c"
	"zlib/deflate_rle.c"
	"zlib/deflate_slow.c"
	"zlib/deflate_stored.c"
	"zlib/functable.c"
	"zlib/infback.c"
	"zlib/inflate.c"
	"zlib/inftrees.c"
	"zlib/insert_string.c"
	"zlib/insert_string_roll.c"
	"zlib/trees.c"
	"zlib/uncompr.c"
	"zlib/zutil.c"
  ${ZLIB_GENERIC_SOURCES}
)

if("${BAULK_ARCH_NAME}" STREQUAL "x86_64"
   OR "${BAULK_ARCH_NAME}" STREQUAL "amd64"
   OR "${BAULK_ARCH_NAME}" STREQUAL "x64"
   OR "${BAULK_ARCH_NAME}" STREQUAL "x86")
   list(APPEND SRC_ZLIB ${ZLIB_X86_SOURCES})
elseif("${BAULK_ARCH_NAME}" STREQUAL "arm64")
  list(APPEND SRC_ZLIB ${ZLIB_ARM_SOURCES})
endif()

add_library(zlib STATIC ${SRC_ZLIB})

if("${BAULK_ARCH_NAME}" STREQUAL "x86_64"
   OR "${BAULK_ARCH_NAME}" STREQUAL "amd64"
   OR "${BAULK_ARCH_NAME}" STREQUAL "x64"
   OR "${BAULK_ARCH_NAME}" STREQUAL "x86")
  target_compile_definitions(
    zlib
    PRIVATE ZLIB_COMPAT
            X86_FEATURES
            X86_PCLMULQDQ_CRC
            X86_SSE2
            X86_SSE42
            X86_SSSE3
            X86_AVX2)
elseif("${BAULK_ARCH_NAME}" STREQUAL "arm64")
  target_compile_definitions(
    zlib
    PRIVATE ZLIB_COMPAT
           _ARM64_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE=1
           ARM_NEON_HASLD4
           ARM_FEATURES
           ARM_ACLE
           __ARM_NEON__=1
           ARM_NEON
           ARM_NOCHECK_NEON)
endif()

target_include_directories(zlib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/zlib)

# deflate64
add_library(deflate64 STATIC deflate64/infback9.c deflate64/inftree9.c)
# ZLIB_COMPAT
target_compile_definitions(deflate64 PRIVATE ZLIB_COMPAT)
target_include_directories(deflate64 PRIVATE zlib)

add_subdirectory(xz)

# ppmd
add_subdirectory(ppmd)

# zstd lib
file(
  GLOB
  ZSTD_SOURCES
  zstd/common/*.c
  zstd/compress/*.c
  zstd/decompress/*.c
  zstd/dictBuilder/*.c)

add_library(zstd STATIC ${ZSTD_SOURCES})

target_compile_definitions(zstd PRIVATE XXH_PRIVATE_API ZSTD_MULTITHREAD ZSTD_DISABLE_ASM)
target_include_directories(zstd PRIVATE zstd zstd/common)

file(
  GLOB
  BAULK_ARCHIVE_SOURCES
  *.cc
  tar/*.cc
  zip/*.cc)

add_library(baulk.archive STATIC ${BAULK_ARCHIVE_SOURCES})

target_include_directories(
  baulk.archive
  PRIVATE bzip2
          zlib
          xz/src/liblzma/api
          zstd
          brotli/include
          ced)

target_compile_options(baulk.archive PRIVATE ${LZMA_DEF})
target_link_libraries(
  baulk.archive
  baulk.mem
  belawin
  bzip2
  brotli
  deflate64
  liblzma
  ppmd
  zlib
  zstd
  ced
  advapi32)
