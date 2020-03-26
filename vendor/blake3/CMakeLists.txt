if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64"
   OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64"
   OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86")
  set(BLAKE3_SIMDSRC blake3_sse41.c blake3_avx2.c blake3_avx512.c)
  # SIMD please
  set_source_files_properties(blake3_avx512.c PROPERTIES COMPILE_FLAGS "-arch:AVX512")
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ARM64")
  set(BLAKE3_SIMDSRC blake3_neon.c)
endif()

add_library(blake3 STATIC blake3.c blake3_dispatch.c blake3_portable.c ${BLAKE3_SIMDSRC})
