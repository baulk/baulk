# bela::hash https://docs.microsoft.com/en-us/cpp/intrinsics/arm64-intrinsics
# https://static.docs.arm.com/ihi0073/c/IHI0073C_arm_neon_intrinsics_ref.pdf
string(TOLOWER "${CMAKE_C_COMPILER_ARCHITECTURE_ID}" BELA_COMPILER_ARCH_ID)
message(STATUS "CMAKE_CXX_COMPILER_ID ${CMAKE_CXX_COMPILER_ID}")
# blake3
if("${BELA_COMPILER_ARCH_ID}" STREQUAL "x86_64"
   OR "${BELA_COMPILER_ARCH_ID}" STREQUAL "amd64"
   OR "${BELA_COMPILER_ARCH_ID}" STREQUAL "x64"
   OR "${BELA_COMPILER_ARCH_ID}" STREQUAL "x86")
  # support clang-cl
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(BLAKE3_SIMDSRC blake3/blake3_sse2_x86-64_windows_gnu.S blake3/blake3_sse41_x86-64_windows_gnu.S
                       blake3/blake3_avx2_x86-64_windows_gnu.S blake3/blake3_avx512_x86-64_windows_gnu.S)
  else()
    # msvc
    set(BLAKE3_SIMDSRC blake3/blake3_sse2.c blake3/blake3_sse41.c blake3/blake3_avx2.c blake3/blake3_avx512.c)
    set_source_files_properties(blake3_avx512.c PROPERTIES COMPILE_FLAGS "-arch:AVX512")
  endif()
elseif("${BELA_COMPILER_ARCH_ID}" STREQUAL "arm64")
  set(BLAKE3_SIMDSRC blake3/blake3_neon.c)
else()
  message(FATAL "unsupport target")
endif()

add_library(
  belahash STATIC
  sha256.cc
  sha512.cc
  sha3.cc
  sm3.cc
  blake3/blake3.c
  blake3/blake3_dispatch.c
  blake3/blake3_portable.c
  ${BLAKE3_SIMDSRC})

target_link_libraries(belahash bela)

if(BELA_ENABLE_LTO)
  set_property(TARGET belahash PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
