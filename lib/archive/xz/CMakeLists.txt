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
    PACKAGE_NAME="${PACKAGE_NAME}"
    PACKAGE_BUGREPORT="xz@tukaani.org"
    PACKAGE_URL="https://tukaani.org/xz/"

    # Standard headers and types are available:
    HAVE_STDBOOL_H
    HAVE__BOOL
    HAVE_STDINT_H
    HAVE_INTTYPES_H

    # Always enable CRC32 since liblzma should never build without it.
    HAVE_CHECK_CRC32

    # Disable assert() checks when no build type has been specified. Non-empty
    # build types like "Release" and "Debug" handle this by default.
    $<$<CONFIG:>:NDEBUG>
)


######################
# System definitions #
######################

# _GNU_SOURCE and such definitions. This specific macro is special since
# it also adds the definitions to CMAKE_REQUIRED_DEFINITIONS.
tuklib_use_system_extensions(ALL)

# This is needed by liblzma and xz.
tuklib_integer(ALL)

# Check for clock_gettime(). Do this before checking for threading so
# that we know there if CLOCK_MONOTONIC is available.
if(NOT WIN32 AND NOT DEFINED HAVE_CLOCK_GETTIME)
    check_symbol_exists(clock_gettime time.h HAVE_CLOCK_GETTIME)
    if(NOT HAVE_CLOCK_GETTIME)
        # With glibc <= 2.17 or Solaris 10 this needs librt.
        unset(HAVE_CLOCK_GETTIME CACHE)

        list(INSERT CMAKE_REQUIRED_LIBRARIES 0 rt)
        check_symbol_exists(clock_gettime time.h HAVE_CLOCK_GETTIME)

        # If it was found now, add it to all targets and keep it
        # in CMAKE_REQUIRED_LIBRARIES for further tests too.
        if(HAVE_CLOCK_GETTIME)
            link_libraries(rt)
        else()
            list(REMOVE_AT CMAKE_REQUIRED_LIBRARIES 0)
        endif()
    endif()
    if(HAVE_CLOCK_GETTIME)
        # Check if CLOCK_MONOTONIC is available for clock_gettime().
        check_symbol_exists(CLOCK_MONOTONIC time.h HAVE_CLOCK_MONOTONIC)

        add_compile_definitions(
            HAVE_CLOCK_GETTIME
            HAVE_CLOCK_MONOTONIC
        )
    endif()
endif()

# Options for new enough GCC or Clang on any arch or operating system:
if(CMAKE_C_COMPILER_ID MATCHES GNU|Clang)
    # configure.ac has a long list but it won't be copied here:
    add_compile_options(-Wall -Wextra)
endif()

#############################################################################
# liblzma
#############################################################################

option(BUILD_SHARED_LIBS "Build liblzma as a shared library instead of static")

add_library(liblzma
    src/common/mythread.h
    src/common/sysdefs.h
    src/common/tuklib_common.h
    src/common/tuklib_config.h
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
    src/liblzma/check/crc_macros.h
    src/liblzma/common/block_util.c
    src/liblzma/common/common.c
    src/liblzma/common/common.h
    src/liblzma/common/easy_preset.c
    src/liblzma/common/easy_preset.h
    src/liblzma/common/filter_common.c
    src/liblzma/common/filter_common.h
    src/liblzma/common/hardware_physmem.c
    src/liblzma/common/index.c
    src/liblzma/common/index.h
    src/liblzma/common/memcmplen.h
    src/liblzma/common/stream_flags_common.c
    src/liblzma/common/stream_flags_common.h
    src/liblzma/common/string_conversion.c
    src/liblzma/common/vli_size.c
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


######################
# Size optimizations #
######################

option(ENABLE_SMALL "Reduce code size at expense of speed. \
This may be useful together with CMAKE_BUILD_TYPE=MinSizeRel.")

if(ENABLE_SMALL)
    add_compile_definitions(HAVE_SMALL)
endif()


##########
# Checks #
##########

set(ADDITIONAL_SUPPORTED_CHECKS crc64 sha256)

set(ADDITIONAL_CHECK_TYPES "${ADDITIONAL_SUPPORTED_CHECKS}" CACHE STRING
    "Additional check types to support (crc32 is always built)")

foreach(CHECK IN LISTS ADDITIONAL_CHECK_TYPES)
    if(NOT CHECK IN_LIST ADDITIONAL_SUPPORTED_CHECKS)
        message(SEND_ERROR "'${CHECK}' is not a supported check type")
    endif()
endforeach()

if(ENABLE_SMALL)
    target_sources(liblzma PRIVATE src/liblzma/check/crc32_small.c)
else()
    target_sources(liblzma PRIVATE
        src/liblzma/check/crc32_fast.c
        src/liblzma/check/crc32_table.c
        src/liblzma/check/crc32_table_be.h
        src/liblzma/check/crc32_table_le.h
    )
endif()

if("crc64" IN_LIST ADDITIONAL_CHECK_TYPES)
    add_compile_definitions("HAVE_CHECK_CRC64")

    if(ENABLE_SMALL)
        target_sources(liblzma PRIVATE src/liblzma/check/crc64_small.c)
    else()
        target_sources(liblzma PRIVATE
            src/liblzma/check/crc64_fast.c
            src/liblzma/check/crc64_table.c
            src/liblzma/check/crc64_table_be.h
            src/liblzma/check/crc64_table_le.h
        )
    endif()
endif()

if("sha256" IN_LIST ADDITIONAL_CHECK_TYPES)
    add_compile_definitions("HAVE_CHECK_SHA256")
    target_sources(liblzma PRIVATE src/liblzma/check/sha256.c)
endif()


#################
# Match finders #
#################

set(SUPPORTED_MATCH_FINDERS hc3 hc4 bt2 bt3 bt4)

set(MATCH_FINDERS "${SUPPORTED_MATCH_FINDERS}" CACHE STRING
    "Match finders to support (at least one is required for LZMA1 or LZMA2)")

foreach(MF IN LISTS MATCH_FINDERS)
    if(MF IN_LIST SUPPORTED_MATCH_FINDERS)
        string(TOUPPER "${MF}" MF_UPPER)
        add_compile_definitions("HAVE_MF_${MF_UPPER}")
    else()
        message(SEND_ERROR "'${MF}' is not a supported match finder")
    endif()
endforeach()


#############
# Threading #
#############

# Supported thread methods:
# ON    - autodetect the best threading method. The autodetection will
#         prefer Windows threading (win95 or vista) over posix if both are
#         available. vista threads will be used over win95 unless it is a
#         32-bit build.
# OFF   - Disable threading.
# posix - Use posix threading, or throw an error if not available.
# win95 - Use Windows win95 threading, or throw an error if not available.
# vista - Use Windows vista threading, or throw an error if not available.
set(SUPPORTED_THREAD_METHODS ON OFF posix win95 vista)

set(ENABLE_THREADS ON CACHE STRING
        "Threading method type to support. Set to 'OFF' to disable threading")

# Create dropdown in CMake GUI since only 1 threading method is possible
# to select in a build.
set_property(CACHE ENABLE_THREADS
        PROPERTY STRINGS "${SUPPORTED_THREAD_METHODS}")

if(NOT ENABLE_THREADS IN_LIST SUPPORTED_THREAD_METHODS)
    message(SEND_ERROR "'${ENABLE_THREADS}' is not a supported thread type")
endif()

if(ENABLE_THREADS)
    # Also set THREADS_PREFER_PTHREAD_FLAG since the flag has no effect
    # for Windows threading.
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    # If both Windows and posix threading are available, prefer Windows.
    if(CMAKE_USE_WIN32_THREADS_INIT AND NOT ENABLE_THREADS STREQUAL "posix")
        if(ENABLE_THREADS STREQUAL "win95"
                OR (ENABLE_THREADS STREQUAL "ON"
                AND CMAKE_SIZEOF_VOID_P EQUAL 4))
            # Use Windows 95 (and thus XP) compatible threads.
            # This avoids use of features that were added in
            # Windows Vista. This is used for 32-bit x86 builds for
            # compatibility reasons since it makes no measurable difference
            # in performance compared to Vista threads.
            #
            # The Win95 threading lacks thread-safe one-time initialization
            # function.
            if (ENABLE_SMALL)
                message(SEND_ERROR "Threading method win95 and ENABLE_SMALL "
                                   "cannot be used at the same time")
            endif()

            add_compile_definitions(MYTHREAD_WIN95)
        else()
            add_compile_definitions(MYTHREAD_VISTA)
        endif()
    elseif(CMAKE_USE_PTHREADS_INIT)
        if(ENABLE_THREADS STREQUAL "posix" OR ENABLE_THREADS STREQUAL "ON")
            # Overwrite ENABLE_THREADS in case it was set to "ON".
            # The threading library only needs to be explicitly linked
            # for posix threads, so this is needed for creating
            # liblzma-config.cmake later.
            set(ENABLE_THREADS "posix")

            target_link_libraries(liblzma Threads::Threads)
            add_compile_definitions(MYTHREAD_POSIX)

            # Check if pthread_condattr_setclock() exists to use CLOCK_MONOTONIC.
            if(HAVE_CLOCK_MONOTONIC)
                list(INSERT CMAKE_REQUIRED_LIBRARIES 0 "${CMAKE_THREAD_LIBS_INIT}")
                check_symbol_exists(pthread_condattr_setclock pthread.h
                                    HAVE_PTHREAD_CONDATTR_SETCLOCK)
                tuklib_add_definition_if(ALL HAVE_PTHREAD_CONDATTR_SETCLOCK)
            endif()
        else()
            message(SEND_ERROR
                    "Windows thread method requested, but a compatible "
                    "library could not be found")
        endif()
    else()
        message(SEND_ERROR "No supported threading library found")
    endif()

    target_sources(liblzma PRIVATE
        src/common/tuklib_cpucores.c
        src/common/tuklib_cpucores.h
        src/liblzma/common/hardware_cputhreads.c
        src/liblzma/common/outqueue.c
        src/liblzma/common/outqueue.h
    )
endif()


############
# Encoders #
############

set(SIMPLE_FILTERS
    x86
    arm
    armthumb
    arm64
    powerpc
    ia64
    sparc
)

# The SUPPORTED_FILTERS are shared between Encoders and Decoders
# since only lzip does not appear in both lists. lzip is a special
# case anyway, so it is handled separately in the Decoders section.
set(SUPPORTED_FILTERS
    lzma1
    lzma2
    delta
    "${SIMPLE_FILTERS}"
)

set(ENCODERS "${SUPPORTED_FILTERS}" CACHE STRING "Encoders to support")

# If LZMA2 is enabled, then LZMA1 must also be enabled.
if(NOT "lzma1" IN_LIST ENCODERS AND "lzma2" IN_LIST ENCODERS)
    message(SEND_ERROR "LZMA2 encoder requires that LZMA1 is also enabled")
endif()

# If LZMA1 is enabled, then at least one match finder must be enabled.
if(MATCH_FINDERS STREQUAL "" AND "lzma1" IN_LIST ENCODERS)
    message(SEND_ERROR "At least 1 match finder is required for an "
                       "LZ-based encoder")
endif()

set(HAVE_DELTA_CODER OFF)
set(SIMPLE_ENCODERS OFF)
set(HAVE_ENCODERS OFF)

foreach(ENCODER IN LISTS ENCODERS)
    if(ENCODER IN_LIST SUPPORTED_FILTERS)
        set(HAVE_ENCODERS ON)

        if(NOT SIMPLE_ENCODERS AND ENCODER IN_LIST SIMPLE_FILTERS)
            set(SIMPLE_ENCODERS ON)
        endif()

        string(TOUPPER "${ENCODER}" ENCODER_UPPER)
        add_compile_definitions("HAVE_ENCODER_${ENCODER_UPPER}")
    else()
        message(SEND_ERROR "'${ENCODER}' is not a supported encoder")
    endif()
endforeach()

if(HAVE_ENCODERS)
    add_compile_definitions(HAVE_ENCODERS)

    target_sources(liblzma PRIVATE
        src/liblzma/common/alone_encoder.c
        src/liblzma/common/block_buffer_encoder.c
        src/liblzma/common/block_buffer_encoder.h
        src/liblzma/common/block_encoder.c
        src/liblzma/common/block_encoder.h
        src/liblzma/common/block_header_encoder.c
        src/liblzma/common/easy_buffer_encoder.c
        src/liblzma/common/easy_encoder.c
        src/liblzma/common/easy_encoder_memusage.c
        src/liblzma/common/filter_buffer_encoder.c
        src/liblzma/common/filter_encoder.c
        src/liblzma/common/filter_encoder.h
        src/liblzma/common/filter_flags_encoder.c
        src/liblzma/common/index_encoder.c
        src/liblzma/common/index_encoder.h
        src/liblzma/common/stream_buffer_encoder.c
        src/liblzma/common/stream_encoder.c
        src/liblzma/common/stream_flags_encoder.c
        src/liblzma/common/vli_encoder.c
    )

    if(ENABLE_THREADS)
        target_sources(liblzma PRIVATE
            src/liblzma/common/stream_encoder_mt.c
        )
    endif()

    if(SIMPLE_ENCODERS)
        target_sources(liblzma PRIVATE
            src/liblzma/simple/simple_encoder.c
            src/liblzma/simple/simple_encoder.h
        )
    endif()

    if("lzma1" IN_LIST ENCODERS)
        target_sources(liblzma PRIVATE
            src/liblzma/lzma/lzma_encoder.c
            src/liblzma/lzma/lzma_encoder.h
            src/liblzma/lzma/lzma_encoder_optimum_fast.c
            src/liblzma/lzma/lzma_encoder_optimum_normal.c
            src/liblzma/lzma/lzma_encoder_private.h
            src/liblzma/lzma/fastpos.h
            src/liblzma/lz/lz_encoder.c
            src/liblzma/lz/lz_encoder.h
            src/liblzma/lz/lz_encoder_hash.h
            src/liblzma/lz/lz_encoder_hash_table.h
            src/liblzma/lz/lz_encoder_mf.c
            src/liblzma/rangecoder/price.h
            src/liblzma/rangecoder/price_table.c
            src/liblzma/rangecoder/range_encoder.h
        )

        if(NOT ENABLE_SMALL)
            target_sources(liblzma PRIVATE src/liblzma/lzma/fastpos_table.c)
        endif()
    endif()

    if("lzma2" IN_LIST ENCODERS)
        target_sources(liblzma PRIVATE
            src/liblzma/lzma/lzma2_encoder.c
            src/liblzma/lzma/lzma2_encoder.h
        )
    endif()

    if("delta" IN_LIST ENCODERS)
        set(HAVE_DELTA_CODER ON)
        target_sources(liblzma PRIVATE
            src/liblzma/delta/delta_encoder.c
            src/liblzma/delta/delta_encoder.h
        )
    endif()
endif()


############
# Decoders #
############

set(DECODERS "${SUPPORTED_FILTERS}" CACHE STRING "Decoders to support")

set(SIMPLE_DECODERS OFF)
set(HAVE_DECODERS OFF)

foreach(DECODER IN LISTS DECODERS)
    if(DECODER IN_LIST SUPPORTED_FILTERS)
        set(HAVE_DECODERS ON)

        if(NOT SIMPLE_DECODERS AND DECODER IN_LIST SIMPLE_FILTERS)
            set(SIMPLE_DECODERS ON)
        endif()

        string(TOUPPER "${DECODER}" DECODER_UPPER)
        add_compile_definitions("HAVE_DECODER_${DECODER_UPPER}")
    else()
        message(SEND_ERROR "'${DECODER}' is not a supported decoder")
    endif()
endforeach()

if(HAVE_DECODERS)
    add_compile_definitions(HAVE_DECODERS)

    target_sources(liblzma PRIVATE
        src/liblzma/common/alone_decoder.c
        src/liblzma/common/alone_decoder.h
        src/liblzma/common/auto_decoder.c
        src/liblzma/common/block_buffer_decoder.c
        src/liblzma/common/block_decoder.c
        src/liblzma/common/block_decoder.h
        src/liblzma/common/block_header_decoder.c
        src/liblzma/common/easy_decoder_memusage.c
        src/liblzma/common/file_info.c
        src/liblzma/common/filter_buffer_decoder.c
        src/liblzma/common/filter_decoder.c
        src/liblzma/common/filter_decoder.h
        src/liblzma/common/filter_flags_decoder.c
        src/liblzma/common/index_decoder.c
        src/liblzma/common/index_decoder.h
        src/liblzma/common/index_hash.c
        src/liblzma/common/stream_buffer_decoder.c
        src/liblzma/common/stream_decoder.c
        src/liblzma/common/stream_flags_decoder.c
        src/liblzma/common/stream_decoder.h
        src/liblzma/common/vli_decoder.c
    )

    if(ENABLE_THREADS)
        target_sources(liblzma PRIVATE
            src/liblzma/common/stream_decoder_mt.c
        )
    endif()

    if(SIMPLE_DECODERS)
        target_sources(liblzma PRIVATE
            src/liblzma/simple/simple_decoder.c
            src/liblzma/simple/simple_decoder.h
        )
    endif()

    if("lzma1" IN_LIST DECODERS)
        target_sources(liblzma PRIVATE
            src/liblzma/lzma/lzma_decoder.c
            src/liblzma/lzma/lzma_decoder.h
            src/liblzma/rangecoder/range_decoder.h
            src/liblzma/lz/lz_decoder.c
            src/liblzma/lz/lz_decoder.h
        )
    endif()

    if("lzma2" IN_LIST DECODERS)
        target_sources(liblzma PRIVATE
            src/liblzma/lzma/lzma2_decoder.c
            src/liblzma/lzma/lzma2_decoder.h
        )
    endif()

    if("delta" IN_LIST DECODERS)
        set(HAVE_DELTA_CODER ON)
        target_sources(liblzma PRIVATE
            src/liblzma/delta/delta_decoder.c
            src/liblzma/delta/delta_decoder.h
        )
    endif()
endif()

# Some sources must appear if the filter is configured as either
# an encoder or decoder.
if("lzma1" IN_LIST ENCODERS OR "lzma1" IN_LIST DECODERS)
    target_sources(liblzma PRIVATE
        src/liblzma/rangecoder/range_common.h
        src/liblzma/lzma/lzma_encoder_presets.c
        src/liblzma/lzma/lzma_common.h
    )
endif()

if(HAVE_DELTA_CODER)
    target_sources(liblzma PRIVATE
        src/liblzma/delta/delta_common.c
        src/liblzma/delta/delta_common.h
        src/liblzma/delta/delta_private.h
    )
endif()

if(SIMPLE_ENCODERS OR SIMPLE_DECODERS)
    target_sources(liblzma PRIVATE
        src/liblzma/simple/simple_coder.c
        src/liblzma/simple/simple_coder.h
        src/liblzma/simple/simple_private.h
    )
endif()

foreach(SIMPLE_CODER IN LISTS SIMPLE_FILTERS)
    if(SIMPLE_CODER IN_LIST ENCODERS OR SIMPLE_CODER IN_LIST DECODERS)
        target_sources(liblzma PRIVATE "src/liblzma/simple/${SIMPLE_CODER}.c")
    endif()
endforeach()


#############
# MicroLZMA #
#############

option(MICROLZMA_ENCODER
       "MicroLZMA encoder (needed by specific applications only)" ON)

option(MICROLZMA_DECODER
       "MicroLZMA decoder (needed by specific applications only)" ON)

if(MICROLZMA_ENCODER)
    if(NOT "lzma1" IN_LIST ENCODERS)
        message(SEND_ERROR "The LZMA1 encoder is required to support the "
                           "MicroLZMA encoder")
    endif()

    target_sources(liblzma PRIVATE src/liblzma/common/microlzma_encoder.c)
endif()

if(MICROLZMA_DECODER)
    if(NOT "lzma1" IN_LIST DECODERS)
        message(SEND_ERROR "The LZMA1 decoder is required to support the "
                           "MicroLZMA decoder")
    endif()

    target_sources(liblzma PRIVATE src/liblzma/common/microlzma_decoder.c)
endif()


#############################
# lzip (.lz) format support #
#############################

option(LZIP_DECODER "Support lzip decoder" ON)

if(LZIP_DECODER)
    # If lzip decoder support is requested, make sure LZMA1 decoder is enabled.
    if(NOT "lzma1" IN_LIST DECODERS)
        message(SEND_ERROR "The LZMA1 decoder is required to support the "
                           "lzip decoder")
    endif()

    add_compile_definitions(HAVE_LZIP_DECODER)

    target_sources(liblzma PRIVATE
        src/liblzma/common/lzip_decoder.c
        src/liblzma/common/lzip_decoder.h
    )
endif()

###

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
            __m128i my_clmul(__m128i a)
            {
                const __m128i b = _mm_set_epi64x(1, 2);
                return _mm_clmulepi64_si128(a, b, 0);
            }
            int main(void) { return 0; }
    "
    HAVE_USABLE_CLMUL)

    tuklib_add_definition_if(liblzma HAVE_USABLE_CLMUL)
endif()

target_compile_definitions(liblzma PRIVATE HAVE_VISIBILITY=0)
target_compile_definitions(liblzma INTERFACE LZMA_API_STATIC)