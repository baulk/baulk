#
set(mi_sources
    mimalloc/src/stats.c
    mimalloc/src/random.c
    mimalloc/src/os.c
    mimalloc/src/bitmap.c
    mimalloc/src/arena.c
    mimalloc/src/segment-cache.c
    mimalloc/src/segment.c
    mimalloc/src/page.c
    mimalloc/src/alloc.c
    mimalloc/src/alloc-aligned.c
    mimalloc/src/alloc-posix.c
    mimalloc/src/heap.c
    mimalloc/src/options.c
    mimalloc/src/init.c)

list(APPEND mi_defines MI_SECURE=4)
set_source_files_properties(${mi_sources} PROPERTIES LANGUAGE CXX)
add_library(baulk.mem STATIC ${mi_sources} mem.cc)
set_property(TARGET baulk.mem PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(baulk.mem PRIVATE ${mi_defines} MI_STATIC_LIB)
target_include_directories(baulk.mem PRIVATE mimalloc/include)
target_link_libraries(
  baulk.mem
  psapi
  shell32
  user32
  advapi32
  bcrypt)
