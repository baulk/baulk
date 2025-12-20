# SPDX-License-Identifier: 0BSD

#############################################################################
#
# CMake support for building XZ Utils
#
# Requirements:
#
#   - CMake 3.20 or later
#
#   - To get translated messages, install GNU gettext tools (the command
#     msgfmt is needed). Alternatively disable translations by setting
#     XZ_NLS=OFF.
#
#   - If building from xz.git instead of a release tarball: To generate
#     translated man pages, run po4a/update-po which requires the po4a
#     tool. The build works without this step too.
#
# About CMAKE_BUILD_TYPE:
#
#   - CMake's standard choices are fine to use for production builds,
#     including "Release" and "RelWithDebInfo".
#
#     NOTE: While "Release" uses -O3 by default with some compilers,
#     this file overrides -O3 to -O2 for "Release" builds if
#     CMAKE_C_FLAGS_RELEASE is not defined by the user. At least
#     with GCC and Clang/LLVM, -O3 doesn't seem useful for this
#     package as it can result in bigger binaries without any
#     improvement in speed compared to -O2.
#
#   - Empty value (the default) is handled slightly specially: It
#     adds -DNDEBUG to disable debugging code (assert() and a few
#     other things). No optimization flags are added so an empty
#     CMAKE_BUILD_TYPE is an easy way to build with whatever
#     optimization flags one wants, and so this method is also
#     suitable for production builds.
#
#     If debugging is wanted when using empty CMAKE_BUILD_TYPE,
#     include -UNDEBUG in the CFLAGS environment variable or
#     in the CMAKE_C_FLAGS CMake variable to override -DNDEBUG.
#     With empty CMAKE_BUILD_TYPE, the -UNDEBUG option will go
#     after the -DNDEBUG option on the compiler command line and
#     thus NDEBUG will be undefined.
#
#   - Non-standard build types like "None" aren't treated specially
#     and thus won't have -DNEBUG. Such non-standard build types
#     SHOULD BE AVOIDED FOR PRODUCTION BUILDS. Or at least one
#     should remember to add -DNDEBUG.
#
# This file provides the following installation components (if you only
# need liblzma, install only its components!):
#   - liblzma_Runtime (shared library only)
#   - liblzma_Development
#   - liblzma_Documentation (examples and Doxygen-generated API docs as HTML)
#   - xz_Runtime (xz, the symlinks, and possibly translation files)
#   - xz_Documentation (xz man pages and the symlinks)
#   - xzdec_Runtime
#   - xzdec_Documentation (xzdec *and* lzmadec man pages)
#   - lzmadec_Runtime
#   - lzmainfo_Runtime
#   - lzmainfo_Documentation (lzmainfo man pages)
#   - scripts_Runtime (xzdiff, xzgrep, xzless, xzmore)
#   - scripts_Documentation (their man pages)
#   - Documentation (generic docs like README and licenses)
#
# To find the target liblzma::liblzma from other packages, use the CONFIG
# option with find_package() to avoid a conflict with the FindLibLZMA module
# with case-insensitive file systems. For example, to require liblzma 5.2.5
# or a newer compatible version:
#
#     find_package(liblzma 5.2.5 REQUIRED CONFIG)
#     target_link_libraries(my_application liblzma::liblzma)
#
#############################################################################
#
# Author: Lasse Collin
#
#############################################################################


include(CMakePushCheckState)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CheckCSourceCompiles)
include(CheckCCompilerFlag)
include(CheckLinkerFlag)
include(cmake/tuklib_large_file_support.cmake)
include(cmake/tuklib_integer.cmake)
include(cmake/tuklib_cpucores.cmake)
include(cmake/tuklib_physmem.cmake)
include(cmake/tuklib_progname.cmake)
include(cmake/tuklib_mbstr.cmake)

file(READ src/liblzma/api/lzma/version.h PACKAGE_VERSION)
string(REGEX REPLACE
"^.*\n\
#define LZMA_VERSION_MAJOR ([0-9]+)\n\
.*\
#define LZMA_VERSION_MINOR ([0-9]+)\n\
.*\
#define LZMA_VERSION_PATCH ([0-9]+)\n\
.*$"
       "\\1.\\2.\\3" PACKAGE_VERSION_SHORT "${PACKAGE_VERSION}")

# Definitions common to all targets:
add_compile_definitions(
    # Package info:
    PACKAGE_NAME="${PACKAGE_NAME_DEFINITION}"
    PACKAGE_BUGREPORT="${PACKAGE_BUGREPORT}"
    PACKAGE_URL="${PACKAGE_URL}"

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
tuklib_use_system_extensions()

# Check for large file support. It's required on some 32-bit platforms and
# even on 64-bit MinGW-w64 to get 64-bit off_t. This can be forced off on
# the CMake command line if needed: -DLARGE_FILE_SUPPORT=OFF
tuklib_large_file_support(ALL)

# This is needed by liblzma and xz.
tuklib_integer(ALL)

# Check for clock_gettime(). Do this before checking for threading so
# that we know there if CLOCK_MONOTONIC is available.
check_symbol_exists(clock_gettime time.h HAVE_CLOCK_GETTIME)

if(NOT HAVE_CLOCK_GETTIME)
    # With glibc <= 2.17 or Solaris 10 this needs librt.
    # Add librt for the next check for HAVE_CLOCK_GETTIME. If it is
    # found after including the library, we know that librt is required.
    list(PREPEND CMAKE_REQUIRED_LIBRARIES rt)
    check_symbol_exists(clock_gettime time.h HAVE_CLOCK_GETTIME_LIBRT)

    # If it was found now, add librt to all targets and keep it in
    # CMAKE_REQUIRED_LIBRARIES for further tests too.
    if(HAVE_CLOCK_GETTIME_LIBRT)
        link_libraries(rt)
        set(LIBS "-lrt ${LIBS}") # For liblzma.pc
    else()
        list(POP_FRONT CMAKE_REQUIRED_LIBRARIES)
    endif()
endif()

if(HAVE_CLOCK_GETTIME OR HAVE_CLOCK_GETTIME_LIBRT)
    add_compile_definitions(HAVE_CLOCK_GETTIME)

    # Check if CLOCK_MONOTONIC is available for clock_gettime().
    check_symbol_exists(CLOCK_MONOTONIC time.h HAVE_CLOCK_MONOTONIC)
    tuklib_add_definition_if(ALL HAVE_CLOCK_MONOTONIC)
endif()


# Options for new enough GCC or Clang on any arch or operating system:
if(CMAKE_C_COMPILER_ID MATCHES GNU|Clang)
    # configure.ac has a long list but it won't be copied here:
    add_compile_options(-Wall -Wextra)
endif()

#############################################################################
# liblzma
#############################################################################

set(LIBLZMA_API_HEADERS
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
)

add_library(liblzma
    src/common/mythread.h
    src/common/sysdefs.h
    src/common/tuklib_common.h
    src/common/tuklib_config.h
    src/common/tuklib_integer.h
    src/common/tuklib_physmem.c
    src/common/tuklib_physmem.h
    ${LIBLZMA_API_HEADERS}
    src/liblzma/check/check.c
    src/liblzma/check/check.h
    src/liblzma/check/crc_common.h
    src/liblzma/check/crc_x86_clmul.h
    src/liblzma/check/crc32_arm64.h
    src/liblzma/check/crc32_loongarch.h
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


#############
# Threading #
#############

# Supported threading methods:
# yes   - Autodetect the best threading method. The autodetection will
#         prefer Windows threading (win95 or vista) over posix if both are
#         available. vista threads will be used over win95 unless it is a
#         32-bit build. Configuration fails if no threading support is found;
#         threading won't be silently disabled.
# no    - Disable threading.
# posix - Use posix threading (pthreads), or throw an error if not available.
# win95 - Use Windows win95 threading, or throw an error if not available.
# vista - Use Windows vista threading, or throw an error if not available.
set(SUPPORTED_THREADING_METHODS yes no posix win95 vista)

set(XZ_THREADS yes CACHE STRING "Threading method: \
'yes' to autodetect, 'no' to disable, 'posix' (pthreads), \
'win95' (WinXP compatible), 'vista' (needs Windows Vista or later)")

# Create dropdown in CMake GUI since only 1 threading method is possible
# to select in a build.
set_property(CACHE XZ_THREADS
             PROPERTY STRINGS "${SUPPORTED_THREADING_METHODS}")

# This is a flag variable set when win95 threads are used. We must ensure
# that the combination of XZ_SMALL and win95 threads is only used with a
# compiler that supports the __constructor__ attribute.
set(USE_WIN95_THREADS OFF)

# This is a flag variable set when posix threading (pthreads) is used.
# It's needed when creating liblzma-config.cmake where dependency on
# Threads::Threads is only needed with pthreads.
set(USE_POSIX_THREADS OFF)

if(NOT XZ_THREADS IN_LIST SUPPORTED_THREADING_METHODS)
    message(FATAL_ERROR "'${XZ_THREADS}' is not a supported threading method")
endif()

if(XZ_THREADS)
    # Also set THREADS_PREFER_PTHREAD_FLAG since the flag has no effect
    # for Windows threading.
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    # If both Windows and posix threading are available, prefer Windows.
    # Note that on Cygwin, CMAKE_USE_WIN32_THREADS_INIT is false.
    if(CMAKE_USE_WIN32_THREADS_INIT AND NOT XZ_THREADS STREQUAL "posix")
        if(XZ_THREADS STREQUAL "win95"
                OR (XZ_THREADS STREQUAL "yes" AND CMAKE_SIZEOF_VOID_P EQUAL 4))
            # Use Windows 95 (and thus XP) compatible threads.
            # This avoids use of features that were added in
            # Windows Vista. This is used for 32-bit x86 builds for
            # compatibility reasons since it makes no measurable difference
            # in performance compared to Vista threads.
            set(USE_WIN95_THREADS ON)
            add_compile_definitions(MYTHREAD_WIN95)
        else()
            add_compile_definitions(MYTHREAD_VISTA)
        endif()
    elseif(CMAKE_USE_PTHREADS_INIT)
        if(XZ_THREADS MATCHES "^posix$|^yes$")
            # The threading library only needs to be explicitly linked
            # for posix threads, so this is needed for creating
            # liblzma-config.cmake later.
            set(USE_POSIX_THREADS ON)

            target_link_libraries(liblzma PRIVATE Threads::Threads)
            add_compile_definitions(MYTHREAD_POSIX)

            # Make the thread libs available in later checks. In practice
            # only pthread_condattr_setclock check should need this.
            list(PREPEND CMAKE_REQUIRED_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")

            # Check if pthread_condattr_setclock() exists to
            # use CLOCK_MONOTONIC.
            if(HAVE_CLOCK_MONOTONIC)
                check_symbol_exists(pthread_condattr_setclock pthread.h
                                    HAVE_PTHREAD_CONDATTR_SETCLOCK)
                tuklib_add_definition_if(ALL HAVE_PTHREAD_CONDATTR_SETCLOCK)
            endif()
        else()
            message(SEND_ERROR
                    "Windows threading method was requested but a compatible "
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


######################
# Size optimizations #
######################

option(XZ_SMALL "Reduce code size at expense of speed. \
This may be useful together with CMAKE_BUILD_TYPE=MinSizeRel.")

if(XZ_SMALL)
    add_compile_definitions(HAVE_SMALL)
endif()


##########
# Checks #
##########

set(SUPPORTED_CHECKS crc32 crc64 sha256)

set(XZ_CHECKS "${SUPPORTED_CHECKS}" CACHE STRING
    "Check types to support (crc32 is always built)")

foreach(CHECK IN LISTS XZ_CHECKS)
    if(NOT CHECK IN_LIST SUPPORTED_CHECKS)
        message(FATAL_ERROR "'${CHECK}' is not a supported check type")
    endif()
endforeach()

if(XZ_SMALL)
    target_sources(liblzma PRIVATE src/liblzma/check/crc32_small.c)
else()
    target_sources(liblzma PRIVATE
        src/liblzma/check/crc32_fast.c
        src/liblzma/check/crc32_table_be.h
        src/liblzma/check/crc32_table_le.h
    )

    if(XZ_ASM_I386)
        target_sources(liblzma PRIVATE src/liblzma/check/crc32_x86.S)
        target_compile_definitions(liblzma PRIVATE HAVE_CRC_X86_ASM)
    endif()
endif()

if("crc64" IN_LIST XZ_CHECKS)
    add_compile_definitions("HAVE_CHECK_CRC64")

    if(XZ_SMALL)
        target_sources(liblzma PRIVATE src/liblzma/check/crc64_small.c)
    else()
        target_sources(liblzma PRIVATE
            src/liblzma/check/crc64_fast.c
            src/liblzma/check/crc64_table_be.h
            src/liblzma/check/crc64_table_le.h
        )

        if(XZ_ASM_I386)
            target_sources(liblzma PRIVATE src/liblzma/check/crc64_x86.S)
            # Adding #define HAVE_CRC_X86_ASM was already handled in
            # the CRC32 case a few lines above. CRC32 is always built.
        endif()
    endif()
endif()

# External SHA-256
#
# At least the following implementations are supported:
#
# OS       Headers                     Library  Type           Function
# FreeBSD  sys/types.h + sha256.h      libmd    SHA256_CTX     SHA256_Init
# NetBSD   sys/types.h + sha2.h                 SHA256_CTX     SHA256_Init
# OpenBSD  sys/types.h + sha2.h                 SHA2_CTX       SHA256Init
# Solaris  sys/types.h + sha2.h        libmd    SHA256_CTX     SHA256Init
# MINIX 3  sys/types.h + sha2.h                 SHA256_CTX     SHA256_Init
# Darwin   CommonCrypto/CommonDigest.h          CC_SHA256_CTX  CC_SHA256_Init
#
# Note that Darwin's CC_SHA256_Update takes buffer size as uint32_t instead
# of size_t.
#
# This is disabled by default because it used to conflict with OpenSSL
# on some platforms and in some cases the builtin code in liblzma was faster.
# See INSTALL and the commit message ac398c3bafa6e4c80e20571373a96947db863b3d.
option(XZ_EXTERNAL_SHA256 "Use SHA-256 code from the operating system \
if possible. See INSTALL for possible subtle problems." OFF)

if("sha256" IN_LIST XZ_CHECKS)
    add_compile_definitions("HAVE_CHECK_SHA256")

    # Assume that external code won't be used. We need this to know
    # if the internal sha256.c should be built.
    set(USE_INTERNAL_SHA256 ON)

    if(XZ_EXTERNAL_SHA256)
        # Find the header.
        set(SHA256_HEADER OFF)

        foreach(X CommonCrypto/CommonDigest.h sha256.h sha2.h)
            string(TOUPPER "HAVE_${X}" HAVE_X)
            string(REGEX REPLACE "[/.]" "_" HAVE_X "${HAVE_X}")
            check_include_file("${X}" "${HAVE_X}")
            if(${HAVE_X})
                target_compile_definitions(liblzma PRIVATE "${HAVE_X}")
                set(SHA256_HEADER "${X}")
                break()
            endif()
        endforeach()

        if(SHA256_HEADER)
            # Find the type to hold the SHA-256 state.
            set(SHA256_TYPE OFF)

            foreach(X CC_SHA256_CTX SHA256_CTX SHA2_CTX)
                string(TOUPPER "HAVE_${X}" HAVE_X)

                # configure.ac uses <sys/types.h> conditionally but it's
                # required on all cases except Darwin where it exists too.
                # So just use it unconditionally here.
                set(SOURCE
                        "#include <sys/types.h>
                        #include <${SHA256_HEADER}>
                        int main(void)
                        {
                            ${X} ctx;
                            return 0;
                        }")
                check_c_source_compiles("${SOURCE}" "${HAVE_X}")
                if(${HAVE_X})
                    target_compile_definitions(liblzma PRIVATE "${HAVE_X}")
                    set(SHA256_TYPE "${X}")
                    break()
                endif()
            endforeach()

            if(SHA256_TYPE)
                # Find the initialization function. It might required libmd.
                foreach(X CC_SHA256_Init SHA256Init SHA256_Init)
                    string(TOUPPER "HAVE_${X}" HAVE_X)

                    # On FreeBSD, <sha256.h> defines the SHA-256 functions as
                    # macros to rename them for namespace reasons. Avoid
                    # check_symbol_exists as that would accept macros without
                    # checking if the program links.
                    set(SOURCE
                            "#include <sys/types.h>
                            #include <${SHA256_HEADER}>
                            int main(void)
                            {
                                ${SHA256_TYPE} ctx;
                                ${X}(&ctx);
                                return 0;
                            }")

                    check_c_source_compiles("${SOURCE}" "${HAVE_X}")
                    if(${HAVE_X})
                        target_compile_definitions(liblzma PRIVATE "${HAVE_X}")
                        set(USE_INTERNAL_SHA256 OFF)
                        break()
                    else()
                        # Try with libmd. Other checks don't need it so we
                        # don't need to leave it into CMAKE_REQUIRED_LIBRARIES.
                        list(PREPEND CMAKE_REQUIRED_LIBRARIES md)
                        check_c_source_compiles("${SOURCE}" "${HAVE_X}_LIBMD")
                        list(POP_FRONT CMAKE_REQUIRED_LIBRARIES)
                        if(${HAVE_X}_LIBMD)
                            # NOTE: Just "${HAVE_X}", not "${HAVE_X}_LIBMD":
                            target_compile_definitions(liblzma PRIVATE
                                                       "${HAVE_X}")
                            target_link_libraries(liblzma PRIVATE md)
                            set(LIBS "-lmd ${LIBS}") # For liblzma.pc
                            set(USE_INTERNAL_SHA256 OFF)
                            break()
                        endif()
                    endif()
                endforeach()
            endif()
        endif()
    endif()

    if(USE_INTERNAL_SHA256)
        target_sources(liblzma PRIVATE src/liblzma/check/sha256.c)
    endif()
endif()


#################
# Match finders #
#################

set(SUPPORTED_MATCH_FINDERS hc3 hc4 bt2 bt3 bt4)

set(XZ_MATCH_FINDERS "${SUPPORTED_MATCH_FINDERS}" CACHE STRING
    "Match finders to support (at least one is required for LZMA1 or LZMA2)")

foreach(MF IN LISTS XZ_MATCH_FINDERS)
    if(MF IN_LIST SUPPORTED_MATCH_FINDERS)
        string(TOUPPER "${MF}" MF_UPPER)
        add_compile_definitions("HAVE_MF_${MF_UPPER}")
    else()
        message(FATAL_ERROR "'${MF}' is not a supported match finder")
    endif()
endforeach()


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
    riscv
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

set(XZ_ENCODERS "${SUPPORTED_FILTERS}" CACHE STRING "Encoders to support")

# If LZMA2 is enabled, then LZMA1 must also be enabled.
if(NOT "lzma1" IN_LIST XZ_ENCODERS AND "lzma2" IN_LIST XZ_ENCODERS)
    message(FATAL_ERROR "LZMA2 encoder requires that LZMA1 is also enabled")
endif()

# If LZMA1 is enabled, then at least one match finder must be enabled.
if(XZ_MATCH_FINDERS STREQUAL "" AND "lzma1" IN_LIST XZ_ENCODERS)
    message(FATAL_ERROR "At least 1 match finder is required for an "
                        "LZ-based encoder")
endif()

set(HAVE_DELTA_CODER OFF)
set(SIMPLE_ENCODERS OFF)
set(HAVE_ENCODERS OFF)

foreach(ENCODER IN LISTS XZ_ENCODERS)
    if(ENCODER IN_LIST SUPPORTED_FILTERS)
        set(HAVE_ENCODERS ON)

        if(NOT SIMPLE_ENCODERS AND ENCODER IN_LIST SIMPLE_FILTERS)
            set(SIMPLE_ENCODERS ON)
        endif()

        string(TOUPPER "${ENCODER}" ENCODER_UPPER)
        add_compile_definitions("HAVE_ENCODER_${ENCODER_UPPER}")
    else()
        message(FATAL_ERROR "'${ENCODER}' is not a supported encoder")
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

    if(XZ_THREADS)
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

    if("lzma1" IN_LIST XZ_ENCODERS)
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

        if(NOT XZ_SMALL)
            target_sources(liblzma PRIVATE src/liblzma/lzma/fastpos_table.c)
        endif()
    endif()

    if("lzma2" IN_LIST XZ_ENCODERS)
        target_sources(liblzma PRIVATE
            src/liblzma/lzma/lzma2_encoder.c
            src/liblzma/lzma/lzma2_encoder.h
        )
    endif()

    if("delta" IN_LIST XZ_ENCODERS)
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

set(XZ_DECODERS "${SUPPORTED_FILTERS}" CACHE STRING "Decoders to support")

set(SIMPLE_DECODERS OFF)
set(HAVE_DECODERS OFF)

foreach(DECODER IN LISTS XZ_DECODERS)
    if(DECODER IN_LIST SUPPORTED_FILTERS)
        set(HAVE_DECODERS ON)

        if(NOT SIMPLE_DECODERS AND DECODER IN_LIST SIMPLE_FILTERS)
            set(SIMPLE_DECODERS ON)
        endif()

        string(TOUPPER "${DECODER}" DECODER_UPPER)
        add_compile_definitions("HAVE_DECODER_${DECODER_UPPER}")
    else()
        message(FATAL_ERROR "'${DECODER}' is not a supported decoder")
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

    if(XZ_THREADS)
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

    if("lzma1" IN_LIST XZ_DECODERS)
        target_sources(liblzma PRIVATE
            src/liblzma/lzma/lzma_decoder.c
            src/liblzma/lzma/lzma_decoder.h
            src/liblzma/rangecoder/range_decoder.h
            src/liblzma/lz/lz_decoder.c
            src/liblzma/lz/lz_decoder.h
        )
    endif()

    if("lzma2" IN_LIST XZ_DECODERS)
        target_sources(liblzma PRIVATE
            src/liblzma/lzma/lzma2_decoder.c
            src/liblzma/lzma/lzma2_decoder.h
        )
    endif()

    if("delta" IN_LIST XZ_DECODERS)
        set(HAVE_DELTA_CODER ON)
        target_sources(liblzma PRIVATE
            src/liblzma/delta/delta_decoder.c
            src/liblzma/delta/delta_decoder.h
        )
    endif()
endif()

# Some sources must appear if the filter is configured as either
# an encoder or decoder.
if("lzma1" IN_LIST XZ_ENCODERS OR "lzma1" IN_LIST XZ_DECODERS)
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
    if(SIMPLE_CODER IN_LIST XZ_ENCODERS OR SIMPLE_CODER IN_LIST XZ_DECODERS)
        target_sources(liblzma PRIVATE "src/liblzma/simple/${SIMPLE_CODER}.c")
    endif()
endforeach()


#############
# MicroLZMA #
#############

option(XZ_MICROLZMA_ENCODER
       "MicroLZMA encoder (needed by specific applications only)" ON)

option(XZ_MICROLZMA_DECODER
       "MicroLZMA decoder (needed by specific applications only)" ON)

if(XZ_MICROLZMA_ENCODER)
    if(NOT "lzma1" IN_LIST XZ_ENCODERS)
        message(FATAL_ERROR "The LZMA1 encoder is required to support the "
                            "MicroLZMA encoder")
    endif()

    target_sources(liblzma PRIVATE src/liblzma/common/microlzma_encoder.c)
endif()

if(XZ_MICROLZMA_DECODER)
    if(NOT "lzma1" IN_LIST XZ_DECODERS)
        message(FATAL_ERROR "The LZMA1 decoder is required to support the "
                            "MicroLZMA decoder")
    endif()

    target_sources(liblzma PRIVATE src/liblzma/common/microlzma_decoder.c)
endif()


#############################
# lzip (.lz) format support #
#############################

option(XZ_LZIP_DECODER "Support lzip decoder" ON)

if(XZ_LZIP_DECODER)
    # If lzip decoder support is requested, make sure LZMA1 decoder is enabled.
    if(NOT "lzma1" IN_LIST XZ_DECODERS)
        message(FATAL_ERROR "The LZMA1 decoder is required to support the "
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

# The Win95 threading lacks a thread-safe one-time initialization function.
# The one-time initialization is needed for crc32_small.c and crc64_small.c
# create the CRC tables. So if small mode is enabled, the threading mode is
# win95, and the compiler does not support attribute constructor, then we
# would end up with a multithreaded build that is thread-unsafe. As a
# result this configuration is not allowed.
if(USE_WIN95_THREADS AND XZ_SMALL AND NOT HAVE_FUNC_ATTRIBUTE_CONSTRUCTOR)
    message(SEND_ERROR "Threading method win95 and XZ_SMALL "
                       "cannot be used at the same time because the compiler "
                       "doesn't support __attribute__((__constructor__))")
endif()


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
    option(XZ_CLMUL_CRC "Use carryless multiplication for CRC \
calculation (with runtime detection) if supported by the compiler" ON)

    if(XZ_CLMUL_CRC)
        check_c_source_compiles("
                #include <immintrin.h>
                #if defined(__e2k__) && __iset__ < 6
                #   error
                #endif
                #if (defined(__GNUC__) || defined(__clang__)) \
                        && !defined(__EDG__)
                __attribute__((__target__(\"ssse3,sse4.1,pclmul\")))
                #endif
                int main(void)
                {
                    __m128i a = _mm_set_epi64x(1, 2);
                    a = _mm_clmulepi64_si128(a, a, 0);
                    return 0;
                }
            "
            HAVE_USABLE_CLMUL)
        tuklib_add_definition_if(liblzma HAVE_USABLE_CLMUL)
    endif()
endif()

# ARM64 C Language Extensions define CRC32 functions in arm_acle.h.
# These are supported by at least GCC and Clang which both need
# __attribute__((__target__("+crc"))), unless the needed compiler flags
# are used to support the CRC instruction.
option(XZ_ARM64_CRC32 "Use ARM64 CRC32 instructions (with runtime detection) \
if supported by the compiler" ON)

if(XZ_ARM64_CRC32)
    check_c_source_compiles("
            #include <stdint.h>

            #ifndef _MSC_VER
            #include <arm_acle.h>
            #endif

            #if (defined(__GNUC__) || defined(__clang__)) && !defined(__EDG__)
            __attribute__((__target__(\"+crc\")))
            #endif
            int main(void)
            {
                return __crc32d(1, 2) != 0;
            }
        "
        HAVE_ARM64_CRC32)

    if(HAVE_ARM64_CRC32)
        target_compile_definitions(liblzma PRIVATE HAVE_ARM64_CRC32)

        # Check for ARM64 CRC32 instruction runtime detection.
        # getauxval() is supported on Linux.
        check_symbol_exists(getauxval sys/auxv.h HAVE_GETAUXVAL)
        tuklib_add_definition_if(liblzma HAVE_GETAUXVAL)

        # With getauxval() we also need HWCAP_CRC32 which was
        # added in glibc 2.24.
        if(HAVE_GETAUXVAL)
            check_symbol_exists(HWCAP_CRC32 sys/auxv.h HAVE_HWCAP_CRC32)
            tuklib_add_definition_if(liblzma HAVE_HWCAP_CRC32)
        endif()

        # elf_aux_info() is supported on FreeBSD and OpenBSD >= 7.6.
        check_symbol_exists(elf_aux_info sys/auxv.h HAVE_ELF_AUX_INFO)
        tuklib_add_definition_if(liblzma HAVE_ELF_AUX_INFO)

        # sysctlbyname("hw.optional.armv8_crc32", ...) is supported on Darwin
        # (macOS, iOS, etc.). Note that sysctlbyname() is supported on FreeBSD,
        # NetBSD, and possibly others too but the string is specific to
        # Apple OSes. The C code is responsible for checking
        # defined(__APPLE__) before using
        # sysctlbyname("hw.optional.armv8_crc32", ...).
        check_symbol_exists(sysctlbyname sys/sysctl.h HAVE_SYSCTLBYNAME)
        tuklib_add_definition_if(liblzma HAVE_SYSCTLBYNAME)
    endif()
endif()

option(XZ_LOONGARCH_CRC32
       "Use LoongArch CRC32 instructions if supported by the compiler" ON)

if(XZ_LOONGARCH_CRC32)
    # LoongArch CRC32 intrinsics are in larchintrin.h.
    # These are supported by at least GCC and Clang.
    #
    # Only 64-bit LoongArch is currently supported.
    # It doesn't need runtime detection.
    check_c_source_compiles("
            #if !(defined(__loongarch__) && __loongarch_grlen >= 64)
            #   error
            #endif

            #include <larchintrin.h>
            int main(void)
            {
                return __crc_w_w_w(1, 2);
            }
        "
        HAVE_LOONGARCH_CRC32)
    tuklib_add_definition_if(liblzma HAVE_LOONGARCH_CRC32)
endif()

target_compile_definitions(liblzma PRIVATE HAVE_VISIBILITY=0)
target_compile_definitions(liblzma INTERFACE LZMA_API_STATIC)