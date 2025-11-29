# Baulk archive code
# ----------------------------
# bzip2 support
file(GLOB BZIP2_SOURCES bzip2/*.c)
add_library(bzip2 STATIC ${BZIP2_SOURCES})
target_compile_definitions(
  bzip2
  PRIVATE
    _CRT_NONSTDC_NO_DEPRECATE
    _CRT_SECURE_NO_DEPRECATE
)

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

# zlib-ng

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
#  -DHAVE_BUILTIN_ASSUME_ALIGNED -DHAVE_CPUID_MS -DNO_FSEEKO -DWITH_GZFILEOP -DWITH_OPTIM -DX86_AVX2 -DX86_AVX512 
#  -DX86_AVX512VNNI -DX86_FEATURES -DX86_HAVE_XSAVE_INTRIN -DX86_PCLMULQDQ_CRC -DX86_SSE2 -DX86_SSE41 -DX86_SSE42 -DX86_SSSE3 
## -DX86_VPCLMULQDQ_CRC -DZLIBNG_NATIVE_API -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_DEPRECATE
# Although my local machine's AMD 7490 HS supports AVX512VNNI, AVX512 cannot be enabled for compatibility reasons.
  target_compile_definitions(
    zlib
    PRIVATE HAVE_BUILTIN_ASSUME_ALIGNED
            DHAVE_CPUID_MS
            NO_FSEEKO
            WITH_GZFILEOP
            WITH_OPTIM
            ZLIBNG_NATIVE_API
            X86_AVX2
            #X86_AVX512
           # X86_AVX512VNNI
            X86_FEATURES
            X86_HAVE_XSAVE_INTRIN
            X86_SSE2
            X86_SSE41
            X86_SSE42
            X86_SSSE3
            X86_PCLMULQDQ_CRC)
elseif("${BAULK_ARCH_NAME}" STREQUAL "arm64")
#  -DARM_CRC32 -DARM_CRC32_INTRIN -DARM_FEATURES -DARM_NEON -DARM_NEON_HASLD4 -DHAVE_BUILTIN_ASSUME_ALIGNED 
#  -DNO_FSEEKO -DWITH_ALL_FALLBACKS -DWITH_GZFILEOP -DWITH_OPTIM -DZLIBNG_NATIVE_API -D_ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE 
#  -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_DEPRECATE -D__ARM_NEON__
  target_compile_definitions(
    zlib
    PRIVATE HAVE_BUILTIN_ASSUME_ALIGNED
            NO_FSEEKO
            WITH_ALL_FALLBACKS
            WITH_GZFILEOP
            WITH_OPTIM
            ZLIBNG_NATIVE_API
            ARM_CRC32
            ARM_CRC32_INTRIN
            ARM_FEATURES
            ARM_NEON
            ARM_NEON_HASLD4
            _ARM64_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE=1
            __ARM_NEON__=1)
endif()

target_include_directories(zlib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/zlib)

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
