///
#ifndef INQUISITIVE_DOCS_HPP
#define INQUISITIVE_DOCS_HPP
#include <cstdint>

namespace hazel::internal {
// Microsoft word/ppt/xls
// WordDocument
// https://msdn.microsoft.com/en-us/library/office/dd926131(v=office.12).aspx
// https://msdn.microsoft.com/en-us/library/office/dd949344(v=office.12).aspx
// https://docs.microsoft.com/zh-cn/previous-versions/office/gg615596(v=office.14)
// https://msdn.microsoft.com/en-us/library/cc313154(v=office.12).aspx

struct oleheader_t {
  uint32_t id[2]; // D0CF11E0 A1B11AE1
  uint32_t clid[4];
  uint16_t verminor; // 0x3e
  uint16_t verdll;   // 0x03
  uint16_t byteorder;
  uint16_t lsectorB;
  uint16_t lssectorB;

  uint16_t reserved1;
  uint32_t reserved2;
  uint32_t reserved3;

  uint32_t cfat; // count full sectors
  uint32_t dirstart;

  uint32_t reserved4;

  uint32_t sectorcutoff; // min size of a standard stream ; if less than this
                         // then it uses short-streams
  uint32_t sfatstart;    // first short-sector or EOC
  uint32_t csfat;        // count short sectors
  uint32_t difstart;     // first sector master sector table or EOC
  uint32_t cdif;         // total count
  uint32_t MSAT[109];    // First 109 MSAT
};

// https://www.oasis-open.org/standards#opendocumentv1.2
} // namespace hazel::internal

#endif
