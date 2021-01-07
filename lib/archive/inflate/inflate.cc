////
#include "inflate.hpp"

namespace baulk::archive::inflate {
// https://github.com/dotnet/runtime/blob/master/src/libraries/System.IO.Compression/src/System/IO/Compression/DeflateManaged/InflaterManaged.cs

// Extra bits for length code 257 - 285.
constexpr const static byte s_extraLengthBits[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                                   2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 16};

// The base length for length code 257 - 285.
// The formula to get the real length for a length code is lengthBase[code - 257] + (value stored in extraBits)
constexpr const static int s_lengthBase[] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                                             31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 3};

// The base distance for distance code 0 - 31
// The real distance for a distance code is  distanceBasePosition[code] + (value stored in extraBits)
constexpr const static int s_distanceBasePosition[] = {
    1,   2,   3,   4,   5,    7,    9,    13,   17,   25,   33,   49,    65,    97,    129,   193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 32769, 49153};

// code lengths for code length alphabet is stored in following order
constexpr const static byte s_codeOrder[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

constexpr const static byte s_staticDistanceTreeTable[] = {
    0x00, 0x10, 0x08, 0x18, 0x04, 0x14, 0x0c, 0x1c, 0x02, 0x12, 0x0a, 0x1a, 0x06, 0x16, 0x0e, 0x1e,
    0x01, 0x11, 0x09, 0x19, 0x05, 0x15, 0x0d, 0x1d, 0x03, 0x13, 0x0b, 0x1b, 0x07, 0x17, 0x0f, 0x1f};
} // namespace baulk::archive::inflate