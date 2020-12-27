# WinZip AES specifies


1. Encryption-Decryption w/ AES-CTR (128, 192, or 256 bits)


2. Key generation with PBKDF2-HMAC-SHA1 (1000 iteration count) that
generates a master key broken into the following:

    a.   First m bytes is for the encryption key
    b.   Next n bytes is for the authentication key
    c.   Last 2 bytes is the password verification value.


3.   Following salt lengths are used w/ password during keygen:
 
|AES Key Size|Salt Size|
|---|---|
|128bit(16bytes) | 8 bytes|
|192bit(24bytes) | 12 bytes|
|256bit(32bytes) | 16 bytes|


4.   Master key len = AESKeyLen + AuthKeyLen + PWVLen:

    a.   AES 128 = 16 + 16 + 2 = 34 bytes of key material
    b.   AES 192 = 24 + 24 + 2 = 50 bytes of key material
    c.   AES 256 = 32 + 32 + 2 = 66 bytes of key material

5.   Authentication Key is same size as AES key.

6.   Authentication with HMAC-SHA1-80 (truncated to 80bits).

7.   A new master key is generated for every file.

8.   The file header and directory header compression method will
be 99 (decimal) indicating Winzip AES encryption. The actual
compression method will be in the extra's payload at the end
of the headers.

9.   A extra field will be added to the file header and directory
header identified by the ID 0x9901 and contains the following info:

    a.   Header ID (2 bytes)
    b.   Data Size (2 bytes)
    c.   Vendor Version (2 bytes)
    d.   Vendor ID (2 bytes)
    e.   AES Strength (1 byte)
    f.   Compression Method (2 bytes)

|Offset|Size(bytes)|Content|
|---|---|---|
|0|2|Extra field header ID (0x9901)|
|2|2|Data size (currently 7, but subject to possible increase in the future)|
|4|2|Integer version number specific to the zip vendor|
|6|2|2-character vendor ID|
|8|1|Integer mode value indicating AES encryption strength|
|9|2|The actual compression method used to compress the file|

10.   The Data Size is always 7.

11.   The Vendor Version can be either 0x0001 (AE-1) or
0x0002 (AE-2).

12.   Vendor ID is ASCII "AE"

13.   AES Strength:

    a.   0x01 - AES-128
    b.   0x02 - AES-192
    c.   0x03 - AES-256

14.   Compression Method is the actual compression method
used that was replaced by the encryption process mentioned in #8.
15. AE-1 keeps the CRC and should be verified after decompression.
AE-2 removes the CRC and shouldn't be verified after decompression.
Refer to http://www.winzip.com/aes_info.htm#winzip11 for the reasoning.

16. Storage Format (file data payload totals CompressedSize64 bytes):

    a.   Salt - 8, 12, or 16 bytes depending on keysize
    b.   Password Verification Value - 2 bytes
    c.   Encrypted Data - compressed size - salt - pwv - auth code lengths
    d.   Authentication code - 10 bytes