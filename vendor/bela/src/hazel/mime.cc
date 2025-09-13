//
#include <hazel/hazel.hpp>

namespace hazel {
struct mime_value_t {
  types::hazel_types_t t;
  const wchar_t *mime;
};
// https://mediatemple.net/community/products/dv/204403964/mime-types
const wchar_t *LookupMIME(types::hazel_types_t t) {
  constexpr mime_value_t mimes[] = {
      {.t = types::ascii, .mime = L"text/plain"},
      {.t = types::utf7, .mime = L"text/plain;charset=UTF-7"},
      {.t = types::utf8, .mime = L"text/plain;charset=UTF-8"},
      {.t = types::utf8bom, .mime = L"text/plain;charset=UTF-8"},
      {.t = types::utf16le, .mime = L"text/plain;charset=UTF-16LE"},
      {.t = types::utf16be, .mime = L"text/plain;charset=UTF-16BE"},
      {.t = types::utf32le, .mime = L"text/plain;charset=UTF-32LE"},
      {.t = types::utf32be, .mime = L"text/plain;charset=UTF-32BE"},
      // text index end
      // binary
      {.t = types::bitcode, .mime = L"application/octet-stream"},           ///< Bitcode file
      {.t = types::archive, .mime = L"application/x-unix-archive"},         ///< ar style archive file
      {.t = types::elf, .mime = L"application/x-elf"},                      ///< ELF Unknown type
      {.t = types::elf_relocatable, .mime = L"application/x-relocatable"},  ///< ELF Relocatable object file
      {.t = types::elf_executable, .mime = L"application/x-executable"},    ///< ELF Executable image
      {.t = types::elf_shared_object, .mime = L"application/x-sharedlib"},  ///< ELF dynamically linked shared lib
      {.t = types::elf_core, .mime = L"application/x-coredump"},            ///< ELF core image
      {.t = types::macho_object, .mime = L"application/x-mach-binary"},     ///< Mach-O Object file
      {.t = types::macho_executable, .mime = L"application/x-mach-binary"}, ///< Mach-O Executable
      {.t = types::macho_fixed_virtual_memory_shared_lib,
       .mime = L"application/x-mach-binary"},                                       ///< Mach-O Shared Lib, FVM
      {.t = types::macho_core, .mime = L"application/x-mach-binary"},               ///< Mach-O Core File
      {.t = types::macho_preload_executable, .mime = L"application/x-mach-binary"}, ///< Mach-O Preloaded Executable
      {.t = types::macho_dynamically_linked_shared_lib,
       .mime = L"application/x-mach-binary"},                                   ///< Mach-O dynlinked shared lib
      {.t = types::macho_dynamic_linker, .mime = L"application/x-mach-binary"}, ///< The Mach-O dynamic linker
      {.t = types::macho_bundle, .mime = L"application/x-mach-binary"},         ///< Mach-O Bundle file
      {.t = types::macho_dynamically_linked_shared_lib_stub,
       .mime = L"application/x-mach-binary"},                                     ///< Mach-O Shared lib stub
      {.t = types::macho_dsym_companion, .mime = L"application/x-mach-binary"},   ///< Mach-O dSYM companion file
      {.t = types::macho_kext_bundle, .mime = L"application/x-mach-binary"},      ///< Mach-O kext bundle file
      {.t = types::macho_universal_binary, .mime = L"application/x-mach-binary"}, ///< Mach-O universal binary
      {.t = types::coff_cl_gl_object,
       .mime = L"application/vnd.microsoft.coff"}, ///< Microsoft cl.exe's intermediate code file
      {.t = types::coff_object, .mime = L"application/vnd.microsoft.coff"},         ///< COFF object file
      {.t = types::coff_import_library, .mime = L"application/vnd.microsoft.coff"}, ///< COFF import library
      {.t = types::pecoff_executable,
       .mime = L"application/vnd.microsoft.portable-executable"}, ///< PECOFF executable file
      {.t = types::windows_resource,
       .mime = L"application/vnd.microsoft.resource"},        ///< Windows compiled resource file (.res)
      {.t = types::wasm_object, .mime = L"application/wasm"}, ///< WebAssembly Object file
      {.t = types::pdb, .mime = L"application/octet-stream"}, ///< Windows PDB debug info file
      /// archive
      {.t = types::epub, .mime = L"application/epub"},
      {.t = types::zip, .mime = L"application/zip"},
      {.t = types::tar, .mime = L"application/x-tar"},
      {.t = types::rar, .mime = L"application/vnd.rar"},
      {.t = types::gz, .mime = L"application/gzip"},
      {.t = types::bz2, .mime = L"application/x-bzip2"},
      {.t = types::zstd, .mime = L"application/x-zstd"},
      {.t = types::p7z, .mime = L"application/x-7z-compressed"},
      {.t = types::xz, .mime = L"application/x-xz"},
      {.t = types::pdf, .mime = L"application/pdf"},
      {.t = types::swf, .mime = L"application/x-shockwave-flash"},
      {.t = types::rtf, .mime = L"application/rtf"},
      {.t = types::eot, .mime = L"application/octet-stream"},
      {.t = types::ps, .mime = L"application/postscript"},
      {.t = types::sqlite, .mime = L"application/vnd.sqlite3"},
      {.t = types::nes, .mime = L"application/x-nes-rom"},
      {.t = types::crx, .mime = L"application/x-google-chrome-extension"},
      {.t = types::deb, .mime = L"application/vnd.debian.binary-package"},
      {.t = types::lz, .mime = L"application/x-lzip"},
      {.t = types::rpm, .mime = L"application/x-rpm"},
      {.t = types::cab, .mime = L"application/vnd.ms-cab-compressed"},
      {.t = types::msi, .mime = L"application/x-msi"},
      {.t = types::dmg, .mime = L"application/x-apple-diskimage"},
      {.t = types::xar, .mime = L"application/x-xar"},
      {.t = types::wim, .mime = L"application/x-ms-wim"},
      {.t = types::z, .mime = L"application/x-compress"},
      {.t = types::nsis, .mime = L"application/x-nsis"},
      // image
      {.t = types::jpg, .mime = L"image/jpeg"},
      {.t = types::jp2, .mime = L"image/jp2"},
      {.t = types::png, .mime = L"image/png"},
      {.t = types::gif, .mime = L"image/gif"},
      {.t = types::webp, .mime = L"image/webp"},
      {.t = types::cr2, .mime = L"image/x-canon-cr2"},
      {.t = types::tif, .mime = L"image/tiff"},
      {.t = types::bmp, .mime = L"image/bmp"},
      {.t = types::jxr, .mime = L"image/vnd.ms-photo"},
      {.t = types::psd, .mime = L"image/vnd.adobe.photoshop"},
      {.t = types::ico, .mime = L"image/vnd.microsoft.icon"}, // image/x-icon
      {.t = types::qoi, .mime = L"image/qoi"},
      // docs
      {.t = types::doc, .mime = L"application/msword"},
      {.t = types::docx, .mime = L"application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
      {.t = types::xls, .mime = L"application/vnd.ms-excel"},
      {.t = types::xlsx, .mime = L"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
      {.t = types::ppt, .mime = L"application/vnd.ms-powerpoint"},
      {.t = types::pptx, .mime = L"application/vnd.openxmlformats-officedocument.presentationml.presentation"},
      //
      {.t = types::ofd, .mime = L"application/ofd"}, // Open Fixed layout Document
      // font
      {.t = types::woff, .mime = L"application/font-woff"},
      {.t = types::woff2, .mime = L"application/font-woff"},
      {.t = types::ttf, .mime = L"application/font-sfnt"},
      {.t = types::otf, .mime = L"application/font-sfnt"},
      // Media
      {.t = types::midi, .mime = L"audio/x-midi"},
      {.t = types::mp3, .mime = L"audio/mpeg"},
      {.t = types::m4a, .mime = L"audio/m4a"},
      {.t = types::ogg, .mime = L"audio/ogg"},
      {.t = types::flac, .mime = L"audio/flac"},
      {.t = types::wav, .mime = L"audio/wave"},
      {.t = types::amr, .mime = L"audio/3gpp"},
      {.t = types::aac, .mime = L"application/vnd.americandynamics.acc"},
      {.t = types::mp4, .mime = L"video/mp4"},
      {.t = types::m4v, .mime = L"video/x-m4v"},
      {.t = types::mkv, .mime = L"video/x-matroska"},
      {.t = types::webm, .mime = L"video/webm"},
      {.t = types::mov, .mime = L"video/quicktime"},
      {.t = types::avi, .mime = L"video/x-msvideo"},
      {.t = types::wmv, .mime = L"video/x-ms-wmv"},
      {.t = types::mpeg, .mime = L"video/mpeg"},
      {.t = types::flv, .mime = L"video/x-flv"},
      // support git
      {.t = types::gitpack, .mime = L"application/x-git-pack"},
      {.t = types::gitpkindex, .mime = L"application/x-git-pack-index"},
      {.t = types::gitmidx, .mime = L"application/x-git-pack-multi-index"},
      {.t = types::lnk, .mime = L"application/x-ms-shortcut"}, // .lnk application/x-ms-shortcut
      {.t = types::iso, .mime = L"application/x-iso9660-image"},
      {.t = types::ifc, .mime = L"application/vnd.microsoft.ifc"},
      {.t = types::goff_object, .mime = L"application/x-goff-object"}};
  //
  for (const auto &m : mimes) {
    if (m.t == t) {
      return m.mime;
    }
  }
  return L"application/octet-stream";
}

} // namespace hazel