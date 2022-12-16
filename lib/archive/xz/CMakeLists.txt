# XZ LIB

include(CMakePushCheckState)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CheckCSourceCompiles)
include(cmake/tuklib_integer.cmake)
include(cmake/tuklib_cpucores.cmake)
include(cmake/tuklib_physmem.cmake)
include(cmake/tuklib_progname.cmake)
include(cmake/tuklib_mbstr.cmake)

# Definitions common to all targets:
add_compile_definitions(
    # Package info:
    PACKAGE_NAME="XZ Utils"
    PACKAGE_BUGREPORT="xz@tukaani.org"
    PACKAGE_URL="https://tukaani.org/xz/"

    # Features:
    HAVE_CHECK_CRC32
    HAVE_CHECK_CRC64
    HAVE_CHECK_SHA256
    HAVE_DECODERS
    HAVE_DECODER_ARM
    HAVE_DECODER_ARMTHUMB
    HAVE_DECODER_ARM64
    HAVE_DECODER_DELTA
    HAVE_DECODER_IA64
    HAVE_DECODER_LZMA1
    HAVE_DECODER_LZMA2
    HAVE_DECODER_POWERPC
    HAVE_DECODER_SPARC
    HAVE_DECODER_X86
    HAVE_ENCODERS
    HAVE_ENCODER_ARM
    HAVE_ENCODER_ARMTHUMB
    HAVE_ENCODER_ARM64
    HAVE_ENCODER_DELTA
    HAVE_ENCODER_IA64
    HAVE_ENCODER_LZMA1
    HAVE_ENCODER_LZMA2
    HAVE_ENCODER_POWERPC
    HAVE_ENCODER_SPARC
    HAVE_ENCODER_X86
    HAVE_MF_BT2
    HAVE_MF_BT3
    HAVE_MF_BT4
    HAVE_MF_HC3
    HAVE_MF_HC4
    HAVE_LZIP_DECODER

    # Standard headers and types are available:
    HAVE_STDBOOL_H
    HAVE__BOOL
    HAVE_STDINT_H
    HAVE_INTTYPES_H

    # Disable assert() checks when no build type has been specified. Non-empty
    # build types like "Release" and "Debug" handle this by default.
    $<$<CONFIG:>:NDEBUG>
)

tuklib_integer(ALL)

# Threading support:
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
if(CMAKE_USE_WIN32_THREADS_INIT)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        # Define to 1 when using Windows 95 (and thus XP) compatible threads. This
        # avoids use of features that were added in Windows Vista.
        # This is used for 32-bit x86 builds for compatibility reasons since it
        # makes no measurable difference in performance compared to Vista threads.
        add_compile_definitions(MYTHREAD_WIN95)
    else()
        # Define to 1 when using Windows Vista compatible threads. This uses features
        # that are not available on Windows XP.
        add_compile_definitions(MYTHREAD_VISTA)
    endif()
else()
    add_compile_definitions(MYTHREAD_POSIX)

    # Check if pthread_condattr_setclock() exists to use CLOCK_MONOTONIC.
    if(HAVE_DECL_CLOCK_MONOTONIC)
        list(INSERT CMAKE_REQUIRED_LIBRARIES 0 "${CMAKE_THREAD_LIBS_INIT}")
        check_symbol_exists(pthread_condattr_setclock pthread.h
                            HAVE_PTHREAD_CONDATTR_SETCLOCK)
        tuklib_add_definition_if(ALL HAVE_PTHREAD_CONDATTR_SETCLOCK)
    endif()
endif()

add_library(liblzma
    src/common/mythread.h
    src/common/sysdefs.h
    src/common/tuklib_common.h
    src/common/tuklib_config.h
    src/common/tuklib_cpucores.c
    src/common/tuklib_cpucores.h
    src/common/tuklib_integer.h
    src/common/tuklib_physmem.c
    src/common/tuklib_physmem.h
    src/liblzma/api/lzma.h
    src/liblzma/api/lzma/base.h
    src/liblzma/api/lzma/bcj.h
    src/liblzma/api/lzma/block.h
    src/liblzma/api/lzma/check.h
    src/liblzma/api/lzma/container.h
    src/liblzma/api/lzma/delta.h
    src/liblzma/api/lzma/filter.h
    src/liblzma/api/lzma/hardware.h
    src/liblzma/api/lzma/index.h
    src/liblzma/api/lzma/index_hash.h
    src/liblzma/api/lzma/lzma12.h
    src/liblzma/api/lzma/stream_flags.h
    src/liblzma/api/lzma/version.h
    src/liblzma/api/lzma/vli.h
    src/liblzma/check/check.c
    src/liblzma/check/check.h
    src/liblzma/check/crc32_fast.c
    src/liblzma/check/crc32_table.c
    src/liblzma/check/crc32_table_be.h
    src/liblzma/check/crc32_table_le.h
    src/liblzma/check/crc64_fast.c
    src/liblzma/check/crc64_table.c
    src/liblzma/check/crc64_table_be.h
    src/liblzma/check/crc64_table_le.h
    src/liblzma/check/crc_macros.h
    src/liblzma/check/sha256.c
    src/liblzma/common/alone_decoder.c
    src/liblzma/common/alone_decoder.h
    src/liblzma/common/alone_encoder.c
    src/liblzma/common/auto_decoder.c
    src/liblzma/common/block_buffer_decoder.c
    src/liblzma/common/block_buffer_encoder.c
    src/liblzma/common/block_buffer_encoder.h
    src/liblzma/common/block_decoder.c
    src/liblzma/common/block_decoder.h
    src/liblzma/common/block_encoder.c
    src/liblzma/common/block_encoder.h
    src/liblzma/common/block_header_decoder.c
    src/liblzma/common/block_header_encoder.c
    src/liblzma/common/block_util.c
    src/liblzma/common/common.c
    src/liblzma/common/common.h
    src/liblzma/common/easy_buffer_encoder.c
    src/liblzma/common/easy_decoder_memusage.c
    src/liblzma/common/easy_encoder.c
    src/liblzma/common/easy_encoder_memusage.c
    src/liblzma/common/easy_preset.c
    src/liblzma/common/easy_preset.h
    src/liblzma/common/file_info.c
    src/liblzma/common/filter_buffer_decoder.c
    src/liblzma/common/filter_buffer_encoder.c
    src/liblzma/common/filter_common.c
    src/liblzma/common/filter_common.h
    src/liblzma/common/filter_decoder.c
    src/liblzma/common/filter_decoder.h
    src/liblzma/common/filter_encoder.c
    src/liblzma/common/filter_encoder.h
    src/liblzma/common/filter_flags_decoder.c
    src/liblzma/common/filter_flags_encoder.c
    src/liblzma/common/hardware_cputhreads.c
    src/liblzma/common/hardware_physmem.c
    src/liblzma/common/index.c
    src/liblzma/common/index.h
    src/liblzma/common/index_decoder.c
    src/liblzma/common/index_decoder.h
    src/liblzma/common/index_encoder.c
    src/liblzma/common/index_encoder.h
    src/liblzma/common/index_hash.c
    src/liblzma/common/lzip_decoder.c
    src/liblzma/common/lzip_decoder.h
    src/liblzma/common/memcmplen.h
    src/liblzma/common/outqueue.c
    src/liblzma/common/outqueue.h
    src/liblzma/common/stream_buffer_decoder.c
    src/liblzma/common/stream_buffer_encoder.c
    src/liblzma/common/stream_decoder.c
    src/liblzma/common/stream_decoder_mt.c
    src/liblzma/common/stream_decoder.h
    src/liblzma/common/stream_encoder.c
    src/liblzma/common/stream_encoder_mt.c
    src/liblzma/common/stream_flags_common.c
    src/liblzma/common/stream_flags_common.h
    src/liblzma/common/stream_flags_decoder.c
    src/liblzma/common/stream_flags_encoder.c
    src/liblzma/common/string_conversion.c
    src/liblzma/common/vli_decoder.c
    src/liblzma/common/vli_encoder.c
    src/liblzma/common/vli_size.c
    src/liblzma/delta/delta_common.c
    src/liblzma/delta/delta_common.h
    src/liblzma/delta/delta_decoder.c
    src/liblzma/delta/delta_decoder.h
    src/liblzma/delta/delta_encoder.c
    src/liblzma/delta/delta_encoder.h
    src/liblzma/delta/delta_private.h
    src/liblzma/lz/lz_decoder.c
    src/liblzma/lz/lz_decoder.h
    src/liblzma/lz/lz_encoder.c
    src/liblzma/lz/lz_encoder.h
    src/liblzma/lz/lz_encoder_hash.h
    src/liblzma/lz/lz_encoder_hash_table.h
    src/liblzma/lz/lz_encoder_mf.c
    src/liblzma/lzma/fastpos.h
    src/liblzma/lzma/fastpos_table.c
    src/liblzma/lzma/lzma2_decoder.c
    src/liblzma/lzma/lzma2_decoder.h
    src/liblzma/lzma/lzma2_encoder.c
    src/liblzma/lzma/lzma2_encoder.h
    src/liblzma/lzma/lzma_common.h
    src/liblzma/lzma/lzma_decoder.c
    src/liblzma/lzma/lzma_decoder.h
    src/liblzma/lzma/lzma_encoder.c
    src/liblzma/lzma/lzma_encoder.h
    src/liblzma/lzma/lzma_encoder_optimum_fast.c
    src/liblzma/lzma/lzma_encoder_optimum_normal.c
    src/liblzma/lzma/lzma_encoder_presets.c
    src/liblzma/lzma/lzma_encoder_private.h
    src/liblzma/rangecoder/price.h
    src/liblzma/rangecoder/price_table.c
    src/liblzma/rangecoder/range_common.h
    src/liblzma/rangecoder/range_decoder.h
    src/liblzma/rangecoder/range_encoder.h
    src/liblzma/simple/arm.c
    src/liblzma/simple/armthumb.c
    src/liblzma/simple/arm64.c
    src/liblzma/simple/ia64.c
    src/liblzma/simple/powerpc.c
    src/liblzma/simple/simple_coder.c
    src/liblzma/simple/simple_coder.h
    src/liblzma/simple/simple_decoder.c
    src/liblzma/simple/simple_decoder.h
    src/liblzma/simple/simple_encoder.c
    src/liblzma/simple/simple_encoder.h
    src/liblzma/simple/simple_private.h
    src/liblzma/simple/sparc.c
    src/liblzma/simple/x86.c
)

target_include_directories(liblzma PRIVATE
    src/liblzma/api
    src/liblzma/common
    src/liblzma/check
    src/liblzma/lz
    src/liblzma/rangecoder
    src/liblzma/lzma
    src/liblzma/delta
    src/liblzma/simple
    src/common
)

target_link_libraries(liblzma Threads::Threads)

# Put the tuklib functions under the lzma_ namespace.
target_compile_definitions(liblzma PRIVATE TUKLIB_SYMBOL_PREFIX=lzma_)

tuklib_cpucores(liblzma)
tuklib_physmem(liblzma)

# While liblzma can be built without tuklib_cpucores or tuklib_physmem
# modules, the liblzma API functions lzma_cputhreads() and lzma_physmem()
# will then be useless (which isn't too bad but still unfortunate). Since
# I expect the CMake-based builds to be only used on systems that are
# supported by these tuklib modules, problems with these tuklib modules
# are considered a hard error for now. This hopefully helps to catch bugs
# in the CMake versions of the tuklib checks.
if(NOT TUKLIB_CPUCORES_FOUND OR NOT TUKLIB_PHYSMEM_FOUND)
    # Use SEND_ERROR instead of FATAL_ERROR. If someone reports a bug,
    # seeing the results of the remaining checks can be useful too.
    message(SEND_ERROR
            "tuklib_cpucores() or tuklib_physmem() failed. "
            "Unless you really are building for a system where these "
            "modules are not supported (unlikely), this is a bug in the "
            "included cmake/tuklib_*.cmake files that should be fixed. "
            "To build anyway, edit this CMakeLists.txt to ignore this error.")
endif()

# Check for __attribute__((__constructor__)) support.
# This needs -Werror because some compilers just warn
# about this being unsupported.
cmake_push_check_state()
set(CMAKE_REQUIRED_FLAGS "-Werror")
check_c_source_compiles("
        __attribute__((__constructor__))
        static void my_constructor_func(void) { return; }
        int main(void) { return 0; }
    "
    HAVE_FUNC_ATTRIBUTE_CONSTRUCTOR)
cmake_pop_check_state()
tuklib_add_definition_if(liblzma HAVE_FUNC_ATTRIBUTE_CONSTRUCTOR)

# cpuid.h
check_include_file(cpuid.h HAVE_CPUID_H)
tuklib_add_definition_if(liblzma HAVE_CPUID_H)

# immintrin.h:
check_include_file(immintrin.h HAVE_IMMINTRIN_H)
if(HAVE_IMMINTRIN_H)
    target_compile_definitions(liblzma PRIVATE HAVE_IMMINTRIN_H)

    # SSE2 intrinsics:
    check_c_source_compiles("
            #include <immintrin.h>
            int main(void)
            {
                __m128i x = { 0 };
                _mm_movemask_epi8(x);
                return 0;
            }
        "
        HAVE__MM_MOVEMASK_EPI8)
    tuklib_add_definition_if(liblzma HAVE__MM_MOVEMASK_EPI8)

    # CLMUL intrinsic:
    check_c_source_compiles("
            #include <immintrin.h>
            #if defined(__e2k__) && __iset__ < 6
            #   error
            #endif
            #if (defined(__GNUC__) || defined(__clang__)) && !defined(__EDG__)
            __attribute__((__target__(\"ssse3,sse4.1,pclmul\")))
            #endif
            __m128i my_clmul(__m128i a, __m128i b)
            {
                return _mm_clmulepi64_si128(a, b, 0);
            }
            int main(void) { return 0; }
    "
    HAVE_USABLE_CLMUL)
    tuklib_add_definition_if(liblzma HAVE_USABLE_CLMUL)
endif()

target_compile_definitions(liblzma PRIVATE HAVE_VISIBILITY=0)
target_compile_definitions(liblzma INTERFACE LZMA_API_STATIC)