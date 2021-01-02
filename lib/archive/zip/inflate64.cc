///
#include <zip.hpp>

namespace baulk::archive::zip {
// https://github.com/dotnet/runtime/blob/master/src/libraries/System.IO.Compression/src/System/IO/Compression/DeflateManaged
constexpr const uint8_t s_extraLengthBits[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                               2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 16};
constexpr const int s_lengthBase[] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                                      31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 3};
constexpr const int s_distanceBasePosition[] = {1,    2,    3,    4,    5,    7,     9,     13,    17,    25,   33,
                                                49,   65,   97,   129,  193,  257,   385,   513,   769,   1025, 1537,
                                                2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 32769, 49153};
constexpr const uint8_t s_codeOrder[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
constexpr const uint8_t s_staticDistanceTreeTable[] = {0x00, 0x10, 0x08, 0x18, 0x04, 0x14, 0x0c, 0x1c, 0x02, 0x12, 0x0a,
                                                       0x1a, 0x06, 0x16, 0x0e, 0x1e, 0x01, 0x11, 0x09, 0x19, 0x05, 0x15,
                                                       0x0d, 0x1d, 0x03, 0x13, 0x0b, 0x1b, 0x07, 0x17, 0x0f, 0x1f};

constexpr int WindowSize = 262144;
constexpr int WindowMask = 262143;

enum inflate_state_t : int {
  ReadingHeader = 0, // Only applies to GZIP

  ReadingBFinal = 2, // About to read bfinal bit
  ReadingBType = 3,  // About to read blockType bits

  ReadingNumLitCodes = 4,        // About to read # literal codes
  ReadingNumDistCodes = 5,       // About to read # dist codes
  ReadingNumCodeLengthCodes = 6, // About to read # code length codes
  ReadingCodeLengthCodes = 7,    // In the middle of reading the code length codes
  ReadingTreeCodesBefore = 8,    // In the middle of reading tree codes (loop top)
  ReadingTreeCodesAfter = 9,     // In the middle of reading tree codes (extension; code > 15)

  DecodeTop = 10,         // About to decode a literal (char/match) in a compressed block
  HaveInitialLength = 11, // Decoding a match, have the literal code (base length)
  HaveFullLength = 12,    // Ditto, now have the full match length (incl. extra length bits)
  HaveDistCode = 13,      // Ditto, now have the distance code also, need extra dist bits

  /* uncompressed blocks */
  UncompressedAligning = 15,
  UncompressedByte1 = 16,
  UncompressedByte2 = 17,
  UncompressedByte3 = 18,
  UncompressedByte4 = 19,
  DecodingUncompressed = 20,

  // These three apply only to GZIP
  StartReadingFooter = 21, // (Initialisation for reading footer)
  ReadingFooter = 22,
  VerifyingFooter = 23,

  Done = 24 // Finished
};

} // namespace baulk::archive::zip