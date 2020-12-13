# bela hazel library

add_library(
  hazel STATIC
  ina/archive.cc
  ina/binexeobj.cc
  ina/docs.cc
  ina/font.cc
  ina/git.cc
  ina/image.cc
  ina/media.cc
  ina/shebang.cc
  ina/shl.cc
  ina/text.cc
  zip/zip.cc
  elf/dynamic.cc
  elf/elf.cc
  elf/elfdata.cc
  elf/gnu.cc
  elf/symbol.cc
  macho/macho.cc
  macho/fat.cc
  fs.cc
  hazel.cc
  mime.cc)

target_link_libraries(hazel bela)

if(BELA_ENABLE_LTO)
  set_property(TARGET hazel PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
