# archive libs

add_subdirectory(chromium_zlib)
add_subdirectory(lzmasdk)

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

add_library(deflate64 STATIC deflate64/infback9.c deflate64/inftree9.c)
target_include_directories(deflate64 PRIVATE chromium_zlib)

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

add_subdirectory(ced)

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
  baulk.archive STATIC
  archive.cc
  ced.cc
  extractor.cc
  format.cc
  tar/brotli.cc
  tar/bzip.cc
  tar/decompressor.cc
  tar/format.cc
  tar/gzip.cc
  tar/tar.cc
  tar/xz.cc
  tar/zstd.cc
  zip/brotli.cc
  zip/bzip.cc
  zip/Crc32.cpp
  zip/decompress.cc
  zip/deflate.cc
  zip/deflate64.cc
  zip/filemode.cc
  zip/ppmd.cc
  zip/xz.cc
  zip/zip.cc
  zip/zstd.cc
  bzip2/blocksort.c
  bzip2/bzlib.c
  bzip2/compress.c
  bzip2/crctable.c
  bzip2/decompress.c
  bzip2/huffman.c
  bzip2/randtable.c
  ${BROTLI_SOURCES})

set(BAULK_ZIP_INC)
list(APPEND BAULK_ZIP_INC bzip2)
list(APPEND BAULK_ZIP_INC chromium_zlib)
list(APPEND BAULK_ZIP_INC liblzma/api)
list(APPEND BAULK_ZIP_INC zstd)
list(APPEND BAULK_ZIP_INC brotli/include)
list(APPEND BAULK_ZIP_INC ced)

target_include_directories(baulk.archive PRIVATE ${BAULK_ZIP_INC})
target_compile_options(baulk.archive PRIVATE ${LZMA_DEF})
target_link_libraries(
  baulk.archive
  baulk.mem
  belawin
  deflate64
  lzma
  lzmasdk
  zlib
  zstd
  ced
  advapi32)
