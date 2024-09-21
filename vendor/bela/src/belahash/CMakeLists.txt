# bela::hash https://docs.microsoft.com/en-us/cpp/intrinsics/arm64-intrinsics
# https://static.docs.arm.com/ihi0073/c/IHI0073C_arm_neon_intrinsics_ref.pdf

# architecture lists for which to enable assembly / SIMD sources
set(BLAKE3_AMD64_NAMES amd64 AMD64 x86_64)
set(BLAKE3_X86_NAMES i686 x86 X86)
set(BLAKE3_ARMv8_NAMES aarch64 AArch64 arm64 ARM64 armv8 armv8a)
# default SIMD compiler flag configuration (can be overriden by toolchains or CLI)
if(MSVC)
  set(BLAKE3_CFLAGS_SSE2 "/arch:SSE2" CACHE STRING "the compiler flags to enable SSE2")
  # MSVC has no dedicated sse4.1 flag (see https://learn.microsoft.com/en-us/cpp/build/reference/arch-x86?view=msvc-170)
  set(BLAKE3_CFLAGS_SSE4.1 "/arch:AVX" CACHE STRING "the compiler flags to enable SSE4.1")
  set(BLAKE3_CFLAGS_AVX2 "/arch:AVX2" CACHE STRING "the compiler flags to enable AVX2")
  set(BLAKE3_CFLAGS_AVX512 "/arch:AVX512" CACHE STRING "the compiler flags to enable AVX512")

  set(BLAKE3_AMD64_ASM_SOURCES
    blake3/blake3_avx2_x86-64_windows_msvc.asm
    blake3/blake3_avx512_x86-64_windows_msvc.asm
    blake3/blake3_sse2_x86-64_windows_msvc.asm
    blake3/blake3_sse41_x86-64_windows_msvc.asm
  )

elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU"
       OR CMAKE_C_COMPILER_ID STREQUAL "Clang"
       OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
  set(BLAKE3_CFLAGS_SSE2 "-msse2" CACHE STRING "the compiler flags to enable SSE2")
  set(BLAKE3_CFLAGS_SSE4.1 "-msse4.1" CACHE STRING "the compiler flags to enable SSE4.1")
  set(BLAKE3_CFLAGS_AVX2 "-mavx2" CACHE STRING "the compiler flags to enable AVX2")
  set(BLAKE3_CFLAGS_AVX512 "-mavx512f -mavx512vl" CACHE STRING "the compiler flags to enable AVX512")

  if (WIN32)
    set(BLAKE3_AMD64_ASM_SOURCES
      blake3/blake3_avx2_x86-64_windows_gnu.S
      blake3/blake3_avx512_x86-64_windows_gnu.S
      blake3/blake3_sse2_x86-64_windows_gnu.S
      blake3/blake3_sse41_x86-64_windows_gnu.S
    )

  elseif(UNIX)
    set(BLAKE3_AMD64_ASM_SOURCES
      blake3/blake3_avx2_x86-64_unix.S
      blake3/blake3_avx512_x86-64_unix.S
      blake3/blake3_sse2_x86-64_unix.S
      blake3/blake3_sse41_x86-64_unix.S
    )
  endif()

  if (CMAKE_SYSTEM_PROCESSOR IN_LIST BLAKE3_ARMv8_NAMES
      AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 32-bit ARMv8 needs NEON to be enabled explicitly
    set(BLAKE3_CFLAGS_NEON "-mfpu=neon" CACHE STRING "the compiler flags to enable NEON")
  endif()
endif()

mark_as_advanced(BLAKE3_CFLAGS_SSE2 BLAKE3_CFLAGS_SSE4.1 BLAKE3_CFLAGS_AVX2 BLAKE3_CFLAGS_AVX512 BLAKE3_CFLAGS_NEON)
mark_as_advanced(BLAKE3_AMD64_ASM_SOURCES)

message(STATUS "BLAKE3 SIMD configuration: ${CMAKE_C_COMPILER_ARCHITECTURE_ID}")
if(MSVC AND DEFINED CMAKE_C_COMPILER_ARCHITECTURE_ID)
  if(CMAKE_C_COMPILER_ARCHITECTURE_ID MATCHES "[Xx]86")
    set(BLAKE3_SIMD_TYPE "x86-intrinsics" CACHE STRING "the SIMD acceleration type to use")

  elseif(CMAKE_C_COMPILER_ARCHITECTURE_ID MATCHES "[Xx]64")
    set(BLAKE3_SIMD_TYPE "amd64-asm" CACHE STRING "the SIMD acceleration type to use")

  elseif(CMAKE_C_COMPILER_ARCHITECTURE_ID MATCHES "[Aa][Rr][Mm]64")
    set(BLAKE3_SIMD_TYPE "neon-intrinsics" CACHE STRING "the SIMD acceleration type to use")

  else()
    set(BLAKE3_SIMD_TYPE "none" CACHE STRING "the SIMD acceleration type to use")
  endif()

elseif(CMAKE_SYSTEM_PROCESSOR IN_LIST BLAKE3_AMD64_NAMES)
  set(BLAKE3_SIMD_TYPE "amd64-asm" CACHE STRING "the SIMD acceleration type to use")

elseif(CMAKE_SYSTEM_PROCESSOR IN_LIST BLAKE3_X86_NAMES
       AND DEFINED BLAKE3_CFLAGS_SSE2
       AND DEFINED BLAKE3_CFLAGS_SSE4.1
       AND DEFINED BLAKE3_CFLAGS_AVX2
       AND DEFINED BLAKE3_CFLAGS_AVX512)
  set(BLAKE3_SIMD_TYPE "x86-intrinsics" CACHE STRING "the SIMD acceleration type to use")

elseif((CMAKE_SYSTEM_PROCESSOR IN_LIST BLAKE3_ARMv8_NAMES
          OR ANDROID_ABI STREQUAL "armeabi-v7a"
          OR BLAKE3_USE_NEON_INTRINSICS)
        AND (DEFINED BLAKE3_CFLAGS_NEON
          OR CMAKE_SIZEOF_VOID_P EQUAL 8))
  set(BLAKE3_SIMD_TYPE "neon-intrinsics" CACHE STRING "the SIMD acceleration type to use")

else()
  set(BLAKE3_SIMD_TYPE "none" CACHE STRING "the SIMD acceleration type to use")
endif()

mark_as_advanced(BLAKE3_SIMD_TYPE)

add_library(
  belahash STATIC
  sha256.cc
  sha512.cc
  sha3.cc
  sm3.cc
  blake3/blake3.c
  blake3/blake3_dispatch.c
  blake3/blake3_portable.c)

# optional SIMD sources
if(BLAKE3_SIMD_TYPE STREQUAL "amd64-asm")
  if (NOT DEFINED BLAKE3_AMD64_ASM_SOURCES)
    message(FATAL_ERROR "BLAKE3_SIMD_TYPE is set to 'amd64-asm' but no assembly sources are available for the target architecture.")
  endif()
  set(BLAKE3_SIMD_AMD64_ASM ON)

  if(MSVC)
    enable_language(ASM_MASM)
  endif()

  target_sources(belahash PRIVATE ${BLAKE3_AMD64_ASM_SOURCES})

elseif(BLAKE3_SIMD_TYPE STREQUAL "x86-intrinsics")
  if (NOT DEFINED BLAKE3_CFLAGS_SSE2
      OR NOT DEFINED BLAKE3_CFLAGS_SSE4.1
      OR NOT DEFINED BLAKE3_CFLAGS_AVX2
      OR NOT DEFINED BLAKE3_CFLAGS_AVX512)
    message(FATAL_ERROR "BLAKE3_SIMD_TYPE is set to 'x86-intrinsics' but no compiler flags are available for the target architecture.")
  endif()
  set(BLAKE3_SIMD_X86_INTRINSICS ON)

  target_sources(belahash PRIVATE
    blake3/blake3_avx2.c
    blake3/blake3_avx512.c
    blake3/blake3_sse2.c
    blake3/blake3_sse41.c
  )
  set_source_files_properties(blake3/blake3_avx2.c PROPERTIES COMPILE_FLAGS "${BLAKE3_CFLAGS_AVX2}")
  set_source_files_properties(blake3/blake3_avx512.c PROPERTIES COMPILE_FLAGS "${BLAKE3_CFLAGS_AVX512}")
  set_source_files_properties(blake3/blake3_sse2.c PROPERTIES COMPILE_FLAGS "${BLAKE3_CFLAGS_SSE2}")
  set_source_files_properties(blake3/blake3_sse41.c PROPERTIES COMPILE_FLAGS "${BLAKE3_CFLAGS_SSE4.1}")

elseif(BLAKE3_SIMD_TYPE STREQUAL "neon-intrinsics")
  set(BLAKE3_SIMD_NEON_INTRINSICS ON)

  target_sources(belahash PRIVATE
    blake3/blake3_neon.c
  )
  target_compile_definitions(belahash PRIVATE
    BLAKE3_USE_NEON=1
  )

  if (DEFINED BLAKE3_CFLAGS_NEON)
    set_source_files_properties(blake3/blake3_neon.c PROPERTIES COMPILE_FLAGS "${BLAKE3_CFLAGS_NEON}")
  endif()

elseif(BLAKE3_SIMD_TYPE STREQUAL "none")
  target_compile_definitions(belahash PRIVATE
    BLAKE3_USE_NEON=0
    BLAKE3_NO_SSE2
    BLAKE3_NO_SSE41
    BLAKE3_NO_AVX2
    BLAKE3_NO_AVX512
  )

else()
  message(FATAL_ERROR "BLAKE3_SIMD_TYPE is set to an unknown value: '${BLAKE3_SIMD_TYPE}'")
endif()

target_link_libraries(belahash bela)

if(BELA_ENABLE_LTO)
  set_property(TARGET belahash PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
