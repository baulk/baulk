//
#ifndef BAULK_ZIP_INFLATE64_HPP
#define BAULK_ZIP_INFLATE64_HPP
#include <cstdint>

namespace baulk::archive::zip::inflate64 {
constexpr uint32_t kNumHuffmanBits = 15;
constexpr uint32_t kHistorySize32 = (1 << 15);
constexpr uint32_t kHistorySize64 = (1 << 16);

constexpr uint32_t kDistTableSize32 = 30;
constexpr uint32_t kDistTableSize64 = 32;

constexpr uint32_t kNumLenSymbols32 = 256;
constexpr uint32_t kNumLenSymbols64 = 255; // don't change it. It must be <= 255.
constexpr uint32_t kNumLenSymbolsMax = kNumLenSymbols32;

constexpr uint32_t kNumLenSlots = 29;

constexpr uint32_t kFixedDistTableSize = 32;
constexpr uint32_t kFixedLenTableSize = 31;

constexpr uint32_t kSymbolEndOfBlock = 0x100;
constexpr uint32_t kSymbolMatch = kSymbolEndOfBlock + 1;

constexpr uint32_t kMainTableSize = kSymbolMatch + kNumLenSlots;
constexpr uint32_t kFixedMainTableSize = kSymbolMatch + kFixedLenTableSize;

constexpr uint32_t kLevelTableSize = 19;

constexpr uint32_t kTableDirectLevels = 16;
constexpr uint32_t kTableLevelRepNumber = kTableDirectLevels;
constexpr uint32_t kTableLevel0Number = kTableLevelRepNumber + 1;
constexpr uint32_t kTableLevel0Number2 = kTableLevel0Number + 1;

constexpr uint32_t kLevelMask = 0xF;

constexpr uint8_t kLenStart32[kFixedLenTableSize] = {0,  1,   2,   3,   4,   5,   6,   7,  8,  10, 12,
                                                     14, 16,  20,  24,  28,  32,  40,  48, 56, 64, 80,
                                                     96, 112, 128, 160, 192, 224, 255, 0,  0};
constexpr uint8_t kLenStart64[kFixedLenTableSize] = {0,  1,  2,  3,  4,  5,  6,  7,   8,   10,  12,  14,  16, 20, 24, 28,
                                                  32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 0,  0,  0};

constexpr uint8_t kLenDirectBits32[kFixedLenTableSize] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                                                          3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0};
constexpr uint8_t kLenDirectBits64[kFixedLenTableSize] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2,  2, 2, 2,
                                                          3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 16, 0, 0};

constexpr uint32_t kDistStart[kDistTableSize64] = {0,    1,    2,    3,    4,    6,     8,     12,    16,    24,   32,
                                                   48,   64,   96,   128,  192,  256,   384,   512,   768,   1024, 1536,
                                                   2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576, 32768, 49152};
constexpr uint8_t kDistDirectBits[kDistTableSize64] = {0, 0, 0, 0, 1, 1, 2,  2,  3,  3,  4,  4,  5,  5,  6,  6,
                                                       7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14};

constexpr uint8_t kLevelDirectBits[3] = {2, 3, 7};

constexpr uint8_t kCodeLengthAlphabetOrder[kLevelTableSize] = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                                               11, 4,  12, 3, 13, 2, 14, 1, 15};

constexpr uint32_t kMatchMinLen = 3;
constexpr uint32_t kMatchMaxLen32 = kNumLenSymbols32 + kMatchMinLen - 1; // 256 + 2
constexpr uint32_t kMatchMaxLen64 = kNumLenSymbols64 + kMatchMinLen - 1; // 255 + 2
constexpr uint32_t kMatchMaxLen = kMatchMaxLen32;

constexpr uint32_t kFinalBlockFieldSize = 1;
} // namespace baulk::archive::zip::inflate64
#endif