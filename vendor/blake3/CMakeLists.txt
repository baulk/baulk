# blake3
if("${COMPILER_ARCH_ID}" STREQUAL "x86_64"
   OR "${COMPILER_ARCH_ID}" STREQUAL "amd64"
   OR "${COMPILER_ARCH_ID}" STREQUAL "x64"
   OR "${COMPILER_ARCH_ID}" STREQUAL "x86")
  set(BLAKE3_SIMDSRC blake3_sse41.c blake3_avx2.c blake3_avx512.c)
  # SIMD please
  set_source_files_properties(blake3_avx512.c PROPERTIES COMPILE_FLAGS "-arch:AVX512")
elseif("${COMPILER_ARCH_ID}" STREQUAL "arm64")
  set(BLAKE3_SIMDSRC blake3_neon.c)
endif()
message(STATUS "lookup CMAKE_C_COMPILER_ARCHITECTURE_ID: ${COMPILER_ARCH_ID}")
add_library(blake3 STATIC blake3.c blake3_dispatch.c blake3_portable.c ${BLAKE3_SIMDSRC})
