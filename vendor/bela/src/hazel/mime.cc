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
      {types::ascii, L"text/plain"},
      {types::utf7, L"text/plain;charset=UTF-7"},
      {types::utf8, L"text/plain;charset=UTF-8"},
      {types::utf8bom, L"text/plain;charset=UTF-8"},
      {types::utf16le, L"text/plain;charset=UTF-16LE"},
      {types::utf16be, L"text/plain;charset=UTF-16BE"},
      {types::utf32le, L"text/plain;charset=UTF-32LE"},
      {types::utf32be, L"text/plain;charset=UTF-32BE"},
      // text index end
      // binary
      {types::bitcode, L"application/octet-stream"},           ///< Bitcode file
      {types::archive, L"application/x-unix-archive"},         ///< ar style archive file
      {types::elf, L"application/x-elf"},                      ///< ELF Unknown type
      {types::elf_relocatable, L"application/x-relocatable"},  ///< ELF Relocatable object file
      {types::elf_executable, L"application/x-executable"},    ///< ELF Executable image
      {types::elf_shared_object, L"application/x-sharedlib"},  ///< ELF dynamically linked shared lib
      {types::elf_core, L"application/x-coredump"},            ///< ELF core image
      {types::macho_object, L"application/x-mach-binary"},     ///< Mach-O Object file
      {types::macho_executable, L"application/x-mach-binary"}, ///< Mach-O Executable
      {types::macho_fixed_virtual_memory_shared_lib, L"application/x-mach-binary"},    ///< Mach-O Shared Lib, FVM
      {types::macho_core, L"application/x-mach-binary"},                               ///< Mach-O Core File
      {types::macho_preload_executable, L"application/x-mach-binary"},                 ///< Mach-O Preloaded Executable
      {types::macho_dynamically_linked_shared_lib, L"application/x-mach-binary"},      ///< Mach-O dynlinked shared lib
      {types::macho_dynamic_linker, L"application/x-mach-binary"},                     ///< The Mach-O dynamic linker
      {types::macho_bundle, L"application/x-mach-binary"},                             ///< Mach-O Bundle file
      {types::macho_dynamically_linked_shared_lib_stub, L"application/x-mach-binary"}, ///< Mach-O Shared lib stub
      {types::macho_dsym_companion, L"application/x-mach-binary"},                     ///< Mach-O dSYM companion file
      {types::macho_kext_bundle, L"application/x-mach-binary"},                        ///< Mach-O kext bundle file
      {types::macho_universal_binary, L"application/x-mach-binary"},                   ///< Mach-O universal binary
      {types::coff_cl_gl_object, L"application/vnd.microsoft.coff"},   ///< Microsoft cl.exe's intermediate code file
      {types::coff_object, L"application/vnd.microsoft.coff"},         ///< COFF object file
      {types::coff_import_library, L"application/vnd.microsoft.coff"}, ///< COFF import library
      {types::pecoff_executable, L"application/vnd.microsoft.portable-executable"}, ///< PECOFF executable file
      {types::windows_resource, L"application/vnd.microsoft.resource"}, ///< Windows compiled resource file (.res)
      {types::wasm_object, L"application/wasm"},                        ///< WebAssembly Object file
      {types::pdb, L"application/octet-stream"},                        ///< Windows PDB debug info file
      /// archive
      {types::epub, L"application/epub"},
      {types::zip, L"application/zip"},
      {types::tar, L"application/x-tar"},
      {types::rar, L"application/vnd.rar"},
      {types::gz, L"application/gzip"},
      {types::bz2, L"application/x-bzip2"},
      {types::p7z, L"application/x-7z-compressed"},
      {types::xz, L"application/x-xz"},
      {types::pdf, L"application/pdf"},
      {types::swf, L"application/x-shockwave-flash"},
      {types::rtf, L"application/rtf"},
      {types::eot, L"application/octet-stream"},
      {types::ps, L"application/postscript"},
      {types::sqlite, L"application/vnd.sqlite3"},
      {types::nes, L"application/x-nintendo-nes-rom"},
      {types::crx, L"application/x-google-chrome-extension"},
      {types::deb, L"application/vnd.debian.binary-package"},
      {types::lz, L"application/x-lzip"},
      {types::rpm, L"application/x-rpm"},
      {types::cab, L"application/vnd.ms-cab-compressed"},
      {types::msi, L"application/x-msi"},
      {types::dmg, L"application/x-apple-diskimage"},
      {types::xar, L"application/x-xar"},
      {types::wim, L"application/x-ms-wim"},
      {types::z, L"application/x-compress"},
      // image
      {types::jpg, L"image/jpeg"},
      {types::jp2, L"image/jp2"},
      {types::png, L"image/png"},
      {types::gif, L"image/gif"},
      {types::webp, L"image/webp"},
      {types::cr2, L"image/x-canon-cr2"},
      {types::tif, L"image/tiff"},
      {types::bmp, L"image/bmp"},
      {types::jxr, L"image/vnd.ms-photo"},
      {types::psd, L"image/vnd.adobe.photoshop"},
      {types::ico, L"image/vnd.microsoft.icon"}, // image/x-icon
      // docs
      {types::doc, L"application/msword"},
      {types::docx, L"application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
      {types::xls, L"application/vnd.ms-excel"},
      {types::xlsx, L"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
      {types::ppt, L"application/vnd.ms-powerpoint"},
      {types::pptx, L"application/vnd.openxmlformats-officedocument.presentationml.presentation"},
      //
      {types::ofd, L"application/ofd"}, // Open Fixed layout Document
      // font
      {types::woff, L"application/font-woff"},
      {types::woff2, L"application/font-woff"},
      {types::ttf, L"application/font-sfnt"},
      {types::otf, L"application/font-sfnt"},
      // Media
      {types::midi, L"audio/x-midi"},
      {types::mp3, L"audio/mpeg"},
      {types::m4a, L"audio/m4a"},
      {types::ogg, L"audio/ogg"},
      {types::flac, L"audio/flac"},
      {types::wav, L"audio/wave"},
      {types::amr, L"audio/3gpp"},
      {types::aac, L"application/vnd.americandynamics.acc"},
      {types::mp4, L"video/mp4"},
      {types::m4v, L"video/x-m4v"},
      {types::mkv, L"video/x-matroska"},
      {types::webm, L"video/webm"},
      {types::mov, L"video/quicktime"},
      {types::avi, L"video/x-msvideo"},
      {types::wmv, L"video/x-ms-wmv"},
      {types::mpeg, L"video/mpeg"},
      {types::flv, L"video/x-flv"},
      // support git
      {types::gitpack, L"application/x-git-pack"},
      {types::gitpkindex, L"application/x-git-pack-index"},
      {types::gitmidx, L"application/x-git-pack-multi-index"},
      {types::shelllink, L"application/vnd.microsoft.shelllink"}, // Windows shelllink
      {types::iso, L"application/x-iso9660-image"},
  };
  //
  for (const auto &m : mimes) {
    if (m.t == t) {
      return m.mime;
    }
  }
  return L"application/octet-stream";
}

} // namespace hazel