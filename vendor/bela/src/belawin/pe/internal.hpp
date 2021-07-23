///
#ifndef BELA_PE_INTERNAL_HPP
#define BELA_PE_INTERNAL_HPP
#include <bela/pe.hpp>

namespace bela::pe {

struct image_import_descriptor {
  uint32_t OriginalFirstThunk;
  uint32_t TimeDateStamp;
  uint32_t ForwarderChain;
  uint32_t Name;
  uint32_t FirstThunk;
};

struct image_delayload_descriptor {
  uint32_t Attributes;
  uint32_t DllNameRVA;
  uint32_t ModuleHandleRVA;
  uint32_t ImportAddressTableRVA;
  uint32_t ImportNameTableRVA;
  uint32_t BoundImportAddressTableRVA;
  uint32_t UnloadInformationTableRVA;
  uint32_t TimeDateStamp;
};
} // namespace bela::pe

#endif