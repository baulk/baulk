#include "zipinternal.hpp"


// File format
// |Size (bytes)|Content|
// |------------|-----------|
// |Variable    |Salt value|
// |2           |Password verification value|
// |Variable    | Encrypted file data|
// |10	        |Authentication code|
// Key ----
// Key size	Salt size
// 128 bits	8 bytes
// 192 bits	12 bytes
// 256 bits	16 bytes