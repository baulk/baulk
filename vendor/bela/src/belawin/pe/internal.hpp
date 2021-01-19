///
#ifndef BELA_PE_INTERNAL_HPP
#define BELA_PE_INTERNAL_HPP
#include <bela/pe.hpp>

namespace bela::pe {
constexpr int DataDirExportTable = 0;            // Export table.
constexpr int DataDirImportTable = 1;            // Import table.
constexpr int DataDirResourceTable = 2;          // Resource table.
constexpr int DataDirExceptionTable = 3;         // Exception table.
constexpr int DataDirCertificateTable = 4;       // Certificate table.
constexpr int DataDirBaseRelocationTable = 5;    // Base relocation table.
constexpr int DataDirDebug = 6;                  // Debugging information.
constexpr int DataDirArchitecture = 7;           // Architecture-specific data.
constexpr int DataDirGlobalPtr = 8;              // Global pointer register.
constexpr int DataDirTLSTable = 9;               // Thread local storage (TLS) table.
constexpr int DataDirLoadConfigTable = 10;       // Load configuration table.
constexpr int DataDirBoundImport = 11;           // Bound import table.
constexpr int DataDirIAT = 12;                   // Import address table.
constexpr int DataDirDelayImportDescriptor = 13; // Delay import descriptor.
constexpr int DataDirCLRHeader = 14;             // CLR header.
constexpr int DataDirReserved = 15;              // Reserved.

inline std::string_view cstring_view(const char *data, size_t len) {
  std::string_view sv{data, len};
  if (auto p = sv.find('\0'); p != std::string_view::npos) {
    return sv.substr(0, p);
  }
  return sv;
}

inline std::string_view cstring_view(const uint8_t *data, size_t len) {
  return cstring_view(reinterpret_cast<const char *>(data), len);
}

// getString extracts a string from symbol string table.
inline std::string getString(std::vector<char> &section, int start) {
  if (start < 0 || static_cast<size_t>(start) >= section.size()) {
    return "";
  }
  for (auto end = static_cast<size_t>(start); end < section.size(); end++) {
    if (section[end] == 0) {
      return std::string(section.data() + start, end - start);
    }
  }
  return "";
}

struct ImportDirectory {
  uint32_t OriginalFirstThunk;
  uint32_t TimeDateStamp;
  uint32_t ForwarderChain;
  uint32_t Name;
  uint32_t FirstThunk;

  std::string DllName;
};

struct ImportDelayDirectory {
  uint32_t Attributes;
  uint32_t DllNameRVA;
  uint32_t ModuleHandleRVA;
  uint32_t ImportAddressTableRVA;
  uint32_t ImportNameTableRVA;
  uint32_t BoundImportAddressTableRVA;
  uint32_t UnloadInformationTableRVA;
  uint32_t TimeDateStamp;

  std::string DllName;
};
} // namespace bela::pe

#endif