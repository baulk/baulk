# bela win libaray

add_library(
  belawin STATIC
  env.cc
  io.cc
  fs.cc
  path.cc
  pe/clr.cc
  pe/file.cc
  pe/overlay.cc
  pe/resource.cc
  pe/rva.cc
  pe/searcher.cc
  pe/section.cc
  pe/string.cc
  pe/symbol.cc
  pe/version.cc
  process.cc
  realpath.cc
  simulator.cc)

target_link_libraries(belawin bela version)

if(BELA_ENABLE_LTO)
  set_property(TARGET belawin PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
