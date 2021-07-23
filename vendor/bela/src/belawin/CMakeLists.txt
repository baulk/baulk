# bela win libaray

set(PE_FILES
  pe/delayimports.cc
  pe/dotnet.cc
  pe/exports.cc
  pe/file.cc
  pe/imports.cc
  pe/overlay.cc
  pe/resource.cc
  pe/rva.cc
  pe/searcher.cc
  pe/section.cc
  pe/string.cc
  pe/symbol.cc
  pe/version.cc
)

add_library(
  belawin STATIC
  env.cc
  io.cc
  fs.cc
  path.cc
  process.cc
  realpath.cc
  simulator.cc
  ${PE_FILES})

target_link_libraries(belawin bela version)

if(BELA_ENABLE_LTO)
  set_property(TARGET belawin PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
