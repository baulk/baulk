//
#ifndef HAZEL_TYPES_HPP
#define HAZEL_TYPES_HPP
#include <cstdint>

namespace hazel::types {

typedef enum hazel_types_e : uint32_t {
  none,
  // text index begin
  ascii,
  utf7,
  utf8,
  utf8bom,
  utf16le,
  utf16be,
  utf32le,
  utf32be,
  // text index end
  // binary
  bitcode,                                  ///< Bitcode file
  archive,                                  ///< ar style archive file
  elf,                                      ///< ELF Unknown type
  elf_relocatable,                          ///< ELF Relocatable object file
  elf_executable,                           ///< ELF Executable image
  elf_shared_object,                        ///< ELF dynamically linked shared lib
  elf_core,                                 ///< ELF core image
  macho_object,                             ///< Mach-O Object file
  macho_executable,                         ///< Mach-O Executable
  macho_fixed_virtual_memory_shared_lib,    ///< Mach-O Shared Lib, FVM
  macho_core,                               ///< Mach-O Core File
  macho_preload_executable,                 ///< Mach-O Preloaded Executable
  macho_dynamically_linked_shared_lib,      ///< Mach-O dynlinked shared lib
  macho_dynamic_linker,                     ///< The Mach-O dynamic linker
  macho_bundle,                             ///< Mach-O Bundle file
  macho_dynamically_linked_shared_lib_stub, ///< Mach-O Shared lib stub
  macho_dsym_companion,                     ///< Mach-O dSYM companion file
  macho_kext_bundle,                        ///< Mach-O kext bundle file
  macho_universal_binary,                   ///< Mach-O universal binary
  minidump,                                 ///< Windows minidump file
  coff_cl_gl_object,                        ///< Microsoft cl.exe's intermediate code file
  coff_object,                              ///< COFF object file
  coff_import_library,                      ///< COFF import library
  pecoff_executable,                        ///< PECOFF executable file
  windows_resource,                         ///< Windows compiled resource file (.res)
  xcoff_object_32,                          ///< 32-bit XCOFF object file
  xcoff_object_64,                          ///< 64-bit XCOFF object file
  wasm_object,                              ///< WebAssembly Object file
  pdb,                                      ///< Windows PDB debug info file
  tapi_file,                                ///< Text-based Dynamic Library Stub file
  /// archive
  epub,
  zip,
  tar,
  rar,
  gz,
  bz2,
  zstd,
  p7z,
  xz,
  pdf,
  swf,
  rtf,
  eot,
  ps,
  sqlite,
  nes,
  crx,
  deb,
  lz,
  rpm,
  cab,
  msi,
  dmg,
  xar,
  wim,
  z,
  // image
  jpg,
  jp2,
  png,
  gif,
  webp,
  cr2,
  tif,
  bmp,
  jxr,
  psd,
  ico,
  // new images
  mif1,
  msf1,
  heic,
  heix,
  hevc,
  hevx,
  heim,
  heis,
  avic,
  hevm,
  hevs,
  avcs,
  avif,
  avis,
  // docs
  doc,
  docx,
  xls,
  xlsx,
  ppt,
  pptx,
  //
  ofd, // Open Fixed layout Document GB/T 33190-2016
  // font
  woff,
  woff2,
  ttf,
  otf,
  // Media
  midi,
  mp3,
  m4a,
  ogg,
  flac,
  wav,
  amr,
  aac,
  mp4,
  m4v,
  mkv,
  webm,
  mov,
  avi,
  wmv,
  mpeg,
  flv,
  // support git
  gitpack,
  gitpkindex,
  gitmidx,
  //
  shelllink, // Windows shelllink
  //
  iso,
  jar,
} hazel_types_t;

}

#endif