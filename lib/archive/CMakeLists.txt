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

# zlib (chromium)
add_subdirectory(zlib)

# deflate64
add_library(deflate64 STATIC deflate64/infback9.c deflate64/inftree9.c)
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
