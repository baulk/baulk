# archive libs

# liblzma
add_library(
  lzma STATIC
  liblzma/common/tuklib_cpucores.c
  liblzma/common/tuklib_physmem.c
  liblzma/check/check.c
  liblzma/check/crc32_fast.c
  liblzma/check/crc32_table.c
  liblzma/check/crc64_fast.c
  liblzma/check/crc64_table.c
  liblzma/check/sha256.c
  liblzma/common/alone_decoder.c
  liblzma/common/alone_encoder.c
  liblzma/common/auto_decoder.c
  liblzma/common/block_buffer_decoder.c
  liblzma/common/block_buffer_encoder.c
  liblzma/common/block_decoder.c
  liblzma/common/block_encoder.c
  liblzma/common/block_header_decoder.c
  liblzma/common/block_header_encoder.c
  liblzma/common/block_util.c
  liblzma/common/common.c
  liblzma/common/easy_buffer_encoder.c
  liblzma/common/easy_decoder_memusage.c
  liblzma/common/easy_encoder.c
  liblzma/common/easy_encoder_memusage.c
  liblzma/common/easy_preset.c
  liblzma/common/filter_buffer_decoder.c
  liblzma/common/filter_buffer_encoder.c
  liblzma/common/filter_common.c
  liblzma/common/filter_decoder.c
  liblzma/common/filter_encoder.c
  liblzma/common/filter_flags_decoder.c
  liblzma/common/filter_flags_encoder.c
  liblzma/common/hardware_cputhreads.c
  liblzma/common/hardware_physmem.c
  liblzma/common/index.c
  liblzma/common/index_decoder.c
  liblzma/common/index_encoder.c
  liblzma/common/index_hash.c
  liblzma/common/outqueue.c
  liblzma/common/stream_buffer_decoder.c
  liblzma/common/stream_buffer_encoder.c
  liblzma/common/stream_decoder.c
  liblzma/common/stream_encoder.c
  liblzma/common/stream_encoder_mt.c
  liblzma/common/stream_flags_common.c
  liblzma/common/stream_flags_decoder.c
  liblzma/common/stream_flags_encoder.c
  liblzma/common/vli_decoder.c
  liblzma/common/vli_encoder.c
  liblzma/common/vli_size.c
  liblzma/delta/delta_common.c
  liblzma/delta/delta_decoder.c
  liblzma/delta/delta_encoder.c
  liblzma/lzma/fastpos_table.c
  liblzma/lzma/lzma2_decoder.c
  liblzma/lzma/lzma2_encoder.c
  liblzma/lzma/lzma_decoder.c
  liblzma/lzma/lzma_encoder.c
  liblzma/lzma/lzma_encoder_optimum_fast.c
  liblzma/lzma/lzma_encoder_optimum_normal.c
  liblzma/lzma/lzma_encoder_presets.c
  liblzma/lz/lz_decoder.c
  liblzma/lz/lz_encoder.c
  liblzma/lz/lz_encoder_mf.c
  liblzma/rangecoder/price_table.c
  liblzma/simple/arm.c
  liblzma/simple/armthumb.c
  liblzma/simple/ia64.c
  liblzma/simple/powerpc.c
  liblzma/simple/simple_coder.c
  liblzma/simple/simple_decoder.c
  liblzma/simple/simple_encoder.c
  liblzma/simple/sparc.c
  liblzma/simple/x86.c)

set(LZMA_INC)
list(APPEND LZMA_INC liblzma)
list(APPEND LZMA_INC liblzma/api)
list(APPEND LZMA_INC liblzma/check)
list(APPEND LZMA_INC liblzma/common)
list(APPEND LZMA_INC liblzma/delta)
list(APPEND LZMA_INC liblzma/lz)
list(APPEND LZMA_INC liblzma/lzma)
list(APPEND LZMA_INC liblzma/rangecoder)
list(APPEND LZMA_INC liblzma/simple)
list(APPEND LZMA_DEF -DLZMA_API_STATIC)

target_include_directories(lzma PRIVATE ${LZMA_INC})
target_compile_definitions(lzma PRIVATE ${LZMA_DEF})

# zlib
set(ZLIB_INC)
if("${CMAKE_C_COMPILER_ARCHITECTURE_ID}" STREQUAL "x86" OR "${CMAKE_C_COMPILER_ARCHITECTURE_ID}" STREQUAL "x64")
  set(ZLIB_SOURCES
      zlib-intel/adler32.c
      zlib-intel/compress.c
      zlib-intel/crc_folding.c
      zlib-intel/crc32.c
      zlib-intel/deflate_medium.c
      zlib-intel/deflate_quick.c
      zlib-intel/deflate.c
      zlib-intel/inffast.c
      zlib-intel/inflate.c
      zlib-intel/inftrees.c
      zlib-intel/match.c
      zlib-intel/slide_sse.c
      zlib-intel/trees.c
      zlib-intel/x86.c
      zlib-intel/zutil.c)
  set(ZLIB_INC zlib-intel)
else()
  set(ZLIB_SOURCES
      zlib/adler32.c
      zlib/compress.c
      zlib/crc32.c
      zlib/deflate.c
      zlib/inffast.c
      zlib/inflate.c
      zlib/inftrees.c
      zlib/trees.c
      zlib/zutil.c)
  set(ZLIB_INC zlib)
endif()

add_library(zlib STATIC ${ZLIB_SOURCES})

add_library(deflate64 STATIC
    deflate64/infback9.c
    deflate64/inftree9.c
)

target_include_directories(deflate64 PRIVATE ${ZLIB_INC})

set(ZSTD_COMMON_SOURCES
    zstd/common/debug.c
    zstd/common/entropy_common.c
    zstd/common/error_private.c
    zstd/common/fse_decompress.c
    zstd/common/pool.c
    zstd/common/threading.c
    zstd/common/zstd_common.c)

file(GLOB ZSTD_COMPRESS_SOURCES zstd/compress/*.c)
file(GLOB ZSTD_DECOMPRESS_SOURCES zstd/decompress/*.c)
file(GLOB ZSTD_DICTBUILDER_SOURCES zstd/dictBuilder/*.c)
file(GLOB ZSTD_DEPRECATED_SOURCES zstd/deprecated/*.c)

add_library(
  zstd STATIC
  ${ZSTD_COMMON_SOURCES}
  ${ZSTD_COMPRESS_SOURCES}
  ${ZSTD_DECOMPRESS_SOURCES}
  ${ZSTD_DICTBUILDER_SOURCES}
  ${ZSTD_DEPRECATED_SOURCES}
  zstd/legacy/zstd_v01.c
  zstd/legacy/zstd_v02.c
  zstd/legacy/zstd_v03.c
  zstd/legacy/zstd_v04.c
  zstd/legacy/zstd_v05.c
  zstd/legacy/zstd_v06.c
  zstd/legacy/zstd_v07.c)

target_compile_definitions(zstd PRIVATE XXH_PRIVATE_API)
target_compile_definitions(zstd PRIVATE ZSTD_MULTITHREAD)
target_include_directories(zstd PRIVATE zstd zstd/common zstd/legacy)

# minizip
add_library(
  minizip STATIC
  minizip/mz_crypt_win32.c
  minizip/mz_crypt.c
  minizip/mz_os.c
  minizip/mz_os_win32.c
  minizip/mz_strm.c
  minizip/mz_strm_buf.c
  minizip/mz_strm_bzip.c
  minizip/mz_strm_lzma.c
  minizip/mz_strm_mem.c
  minizip/mz_strm_os_win32.c
  minizip/mz_strm_split.c
  minizip/mz_strm_wzaes.c
  minizip/mz_strm_lzma.c
  minizip/mz_strm_zlib.c
  minizip/mz_strm_zstd.c
  minizip/mz_zip.c
  minizip/mz_zip_rw.c)

set(MINIZIP_DEF)
set(MINIZIP_INC)
list(APPEND MINIZIP_DEF -DHAVE_BZIP2)
list(APPEND MINIZIP_DEF -D_CRT_SECURE_NO_DEPRECATE)
list(APPEND MINIZIP_DEF -DHAVE_WZAES)
list(APPEND MINIZIP_DEF -DHAVE_ZLIB)
list(APPEND MINIZIP_DEF -DHAVE_LZMA)
list(APPEND MINIZIP_DEF -DHAVE_ZSTD)
list(APPEND MINIZIP_DEF -DLZMA_API_STATIC)
list(APPEND MINIZIP_DEF -DHAVE_IMMINTRIN_H)
list(APPEND MINIZIP_DEF -DHAVE_INTTYPES_H)
list(APPEND MINIZIP_DEF -DHAVE_LIMITS_H)
list(APPEND MINIZIP_DEF -DHAVE_MEMORY_H)
list(APPEND MINIZIP_DEF -DHAVE_STDBOOL_H)
list(APPEND MINIZIP_DEF -DHAVE_STDDEF_H)
list(APPEND MINIZIP_DEF -DHAVE_STDINT_H)
list(APPEND MINIZIP_DEF -DHAVE_STRING_H)
list(APPEND MINIZIP_DEF -DHAVE_SYS_TYPES_H)

list(APPEND MINIZIP_INC bzip2)
list(APPEND MINIZIP_INC ${ZLIB_INC})
list(APPEND MINIZIP_INC liblzma/api)
list(APPEND MINIZIP_INC zstd)

target_compile_definitions(minizip PRIVATE ${MINIZIP_DEF})
target_include_directories(minizip PRIVATE ${MINIZIP_INC})
target_link_libraries(
  minizip
  lzma
  zlib
  zstd
  advapi32)

add_subdirectory(ced)
add_subdirectory(ppmd)

# Brotli
set(BROTLI_SOURCES
    brotli/common/constants.c
    brotli/common/context.c
    brotli/common/dictionary.c
    brotli/common/platform.c
    brotli/common/transform.c
    brotli/dec/bit_reader.c
    brotli/dec/decode.c
    brotli/dec/huffman.c
    brotli/dec/state.c
    brotli/enc/backward_references.c
    brotli/enc/backward_references_hq.c
    brotli/enc/bit_cost.c
    brotli/enc/block_splitter.c
    brotli/enc/brotli_bit_stream.c
    brotli/enc/cluster.c
    brotli/enc/command.c
    brotli/enc/compress_fragment.c
    brotli/enc/compress_fragment_two_pass.c
    brotli/enc/dictionary_hash.c
    brotli/enc/encode.c
    brotli/enc/encoder_dict.c
    brotli/enc/entropy_encode.c
    brotli/enc/fast_log.c
    brotli/enc/histogram.c
    brotli/enc/literal_cost.c
    brotli/enc/memory.c
    brotli/enc/metablock.c
    brotli/enc/static_dict.c
    brotli/enc/utf8_util.c)

add_library(
  baulkarchive STATIC
  archive.cc
  zip/brotli.cc
  zip/bzlib.cc
  zip/Crc32.cpp
  zip/decompress.cc
  zip/deflate.cc
  zip/deflate64.cc
  zip/filemode.cc
  zip/path.cc
  zip/ppmd.cc
  zip/xz.cc
  zip/zip.cc
  zip/zstd.cc
  oldzip.cc
  bzip2/blocksort.c
  bzip2/bzlib.c
  bzip2/compress.c
  bzip2/crctable.c
  bzip2/decompress.c
  bzip2/huffman.c
  bzip2/randtable.c
  ${BROTLI_SOURCES})

target_include_directories(baulkarchive PRIVATE brotli/include)
target_include_directories(baulkarchive PRIVATE ${MINIZIP_INC})
target_include_directories(baulkarchive PRIVATE ced)
target_compile_definitions(baulkarchive PRIVATE ${LZMA_DEF})
target_link_libraries(baulkarchive belawin deflate64 minizip Ppmd ced)
