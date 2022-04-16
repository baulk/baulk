# bela::hash https://docs.microsoft.com/en-us/cpp/intrinsics/arm64-intrinsics
# https://static.docs.arm.com/ihi0073/c/IHI0073C_arm_neon_intrinsics_ref.pdf

set(BELA_BLAKE3_SOURCES blake3/blake3.c blake3/blake3_dispatch.c blake3/blake3_portable.c)

# FIXME CMAKE CMAKE_MSVC_RUNTIME_LIBRARY_DEFAULT
if(BELA_ARCHITECTURE_64BIT)
  if(BELA_ENABLE_ASSEMBLY_FILES)
    if(MSVC)
      enable_language(ASM_MASM)
      list(
        APPEND
        BELA_BLAKE3_SOURCES
        blake3/blake3_sse2_x86-64_windows_msvc.asm
        blake3/blake3_sse41_x86-64_windows_msvc.asm
        blake3/blake3_avx2_x86-64_windows_msvc.asm
        blake3/blake3_avx512_x86-64_windows_msvc.asm)
    elseif(WIN32 OR CYGWIN)
      list(
        APPEND
        BELA_BLAKE3_SOURCES
        blake3/blake3_sse2_x86-64_windows_gnu.S
        blake3/blake3_sse41_x86-64_windows_gnu.S
        blake3/blake3_avx2_x86-64_windows_gnu.S
        blake3/blake3_avx512_x86-64_windows_gnu.S)
    else()
      list(
        APPEND
        BELA_BLAKE3_SOURCES
        blake3/blake3_sse2_x86-64_unix.S
        blake3/blake3_sse41_x86-64_unix.S
        blake3/blake3_avx2_x86-64_unix.S
        blake3/blake3_avx512_x86-64_unix.S)
    endif()
  else()
    list(
      APPEND
      BELA_BLAKE3_SOURCES
      blake3/blake3_sse2.c
      blake3/blake3_sse41.c
      blake3/blake3_avx2.c
      blake3/blake3_avx512.c)
  endif(BELA_ENABLE_ASSEMBLY_FILES)
elseif(BELA_ARCHITECTURE_ARM64)
  list(APPEND BELA_BLAKE3_SOURCES blake3/blake3_neon.c)
elseif(BELA_ARCHITECTURE_32BIT)
  list(
    APPEND
    BELA_BLAKE3_SOURCES
    blake3/blake3_sse2.c
    blake3/blake3_sse41.c
    blake3/blake3_avx2.c
    blake3/blake3_avx512.c)
endif()

add_library(
  belahash STATIC
  sha256.cc
  sha512.cc
  sha3.cc
  sm3.cc
  ${BELA_BLAKE3_SOURCES})

target_link_libraries(belahash bela)

if(BELA_ENABLE_LTO)
  set_property(TARGET belahash PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
