///
#include <bela/__strings/string_cat_internal.hpp>
#include <bela/terminal.hpp>
#include <filesystem>

typedef enum {
  LZMA_OK = 0,
  /**<
   * \brief       Operation completed successfully
   */

  LZMA_STREAM_END = 1,
  /**<
   * \brief       End of stream was reached
   *
   * In encoder, LZMA_SYNC_FLUSH, LZMA_FULL_FLUSH, or
   * LZMA_FINISH was finished. In decoder, this indicates
   * that all the data was successfully decoded.
   *
   * In all cases, when LZMA_STREAM_END is returned, the last
   * output bytes should be picked from strm->next_out.
   */

  LZMA_NO_CHECK = 2,
  /**<
   * \brief       Input stream has no integrity check
   *
   * This return value can be returned only if the
   * LZMA_TELL_NO_CHECK flag was used when initializing
   * the decoder. LZMA_NO_CHECK is just a warning, and
   * the decoding can be continued normally.
   *
   * It is possible to call lzma_get_check() immediately after
   * lzma_code has returned LZMA_NO_CHECK. The result will
   * naturally be LZMA_CHECK_NONE, but the possibility to call
   * lzma_get_check() may be convenient in some applications.
   */

  LZMA_UNSUPPORTED_CHECK = 3,
  /**<
   * \brief       Cannot calculate the integrity check
   *
   * The usage of this return value is different in encoders
   * and decoders.
   *
   * Encoders can return this value only from the initialization
   * function. If initialization fails with this value, the
   * encoding cannot be done, because there's no way to produce
   * output with the correct integrity check.
   *
   * Decoders can return this value only from lzma_code() and
   * only if the LZMA_TELL_UNSUPPORTED_CHECK flag was used when
   * initializing the decoder. The decoding can still be
   * continued normally even if the check type is unsupported,
   * but naturally the check will not be validated, and possible
   * errors may go undetected.
   *
   * With decoder, it is possible to call lzma_get_check()
   * immediately after lzma_code() has returned
   * LZMA_UNSUPPORTED_CHECK. This way it is possible to find
   * out what the unsupported Check ID was.
   */

  LZMA_GET_CHECK = 4,
  /**<
   * \brief       Integrity check type is now available
   *
   * This value can be returned only by the lzma_code() function
   * and only if the decoder was initialized with the
   * LZMA_TELL_ANY_CHECK flag. LZMA_GET_CHECK tells the
   * application that it may now call lzma_get_check() to find
   * out the Check ID. This can be used, for example, to
   * implement a decoder that accepts only files that have
   * strong enough integrity check.
   */

  LZMA_MEM_ERROR = 5,
  /**<
   * \brief       Cannot allocate memory
   *
   * Memory allocation failed, or the size of the allocation
   * would be greater than SIZE_MAX.
   *
   * Due to internal implementation reasons, the coding cannot
   * be continued even if more memory were made available after
   * LZMA_MEM_ERROR.
   */

  LZMA_MEMLIMIT_ERROR = 6,
  /**
   * \brief       Memory usage limit was reached
   *
   * Decoder would need more memory than allowed by the
   * specified memory usage limit. To continue decoding,
   * the memory usage limit has to be increased with
   * lzma_memlimit_set().
   */

  LZMA_FORMAT_ERROR = 7,
  /**<
   * \brief       File format not recognized
   *
   * The decoder did not recognize the input as supported file
   * format. This error can occur, for example, when trying to
   * decode .lzma format file with lzma_stream_decoder,
   * because lzma_stream_decoder accepts only the .xz format.
   */

  LZMA_OPTIONS_ERROR = 8,
  /**<
   * \brief       Invalid or unsupported options
   *
   * Invalid or unsupported options, for example
   *  - unsupported filter(s) or filter options; or
   *  - reserved bits set in headers (decoder only).
   *
   * Rebuilding liblzma with more features enabled, or
   * upgrading to a newer version of liblzma may help.
   */

  LZMA_DATA_ERROR = 9,
  /**<
   * \brief       Data is corrupt
   *
   * The usage of this return value is different in encoders
   * and decoders. In both encoder and decoder, the coding
   * cannot continue after this error.
   *
   * Encoders return this if size limits of the target file
   * format would be exceeded. These limits are huge, thus
   * getting this error from an encoder is mostly theoretical.
   * For example, the maximum compressed and uncompressed
   * size of a .xz Stream is roughly 8 EiB (2^63 bytes).
   *
   * Decoders return this error if the input data is corrupt.
   * This can mean, for example, invalid CRC32 in headers
   * or invalid check of uncompressed data.
   */

  LZMA_BUF_ERROR = 10,
  /**<
   * \brief       No progress is possible
   *
   * This error code is returned when the coder cannot consume
   * any new input and produce any new output. The most common
   * reason for this error is that the input stream being
   * decoded is truncated or corrupt.
   *
   * This error is not fatal. Coding can be continued normally
   * by providing more input and/or more output space, if
   * possible.
   *
   * Typically the first call to lzma_code() that can do no
   * progress returns LZMA_OK instead of LZMA_BUF_ERROR. Only
   * the second consecutive call doing no progress will return
   * LZMA_BUF_ERROR. This is intentional.
   *
   * With zlib, Z_BUF_ERROR may be returned even if the
   * application is doing nothing wrong, so apps will need
   * to handle Z_BUF_ERROR specially. The above hack
   * guarantees that liblzma never returns LZMA_BUF_ERROR
   * to properly written applications unless the input file
   * is truncated or corrupt. This should simplify the
   * applications a little.
   */

  LZMA_PROG_ERROR = 11,
  /**<
   * \brief       Programming error
   *
   * This indicates that the arguments given to the function are
   * invalid or the internal state of the decoder is corrupt.
   *   - Function arguments are invalid or the structures
   *     pointed by the argument pointers are invalid
   *     e.g. if strm->next_out has been set to NULL and
   *     strm->avail_out > 0 when calling lzma_code().
   *   - lzma_* functions have been called in wrong order
   *     e.g. lzma_code() was called right after lzma_end().
   *   - If errors occur randomly, the reason might be flaky
   *     hardware.
   *
   * If you think that your code is correct, this error code
   * can be a sign of a bug in liblzma. See the documentation
   * how to report bugs.
   */
} lzma_ret;

void u8stringshow() {
  bela::FPrintF(stderr, L"char8_t: %v\n", bela::string_cat<char8_t>(u8"args = 1"));
  bela::FPrintF(stderr, L"char8_t: %v\n", bela::string_cat<char8_t>(u8"args = ", 2));
  bela::FPrintF(stderr, L"char8_t: %v\n", bela::string_cat<char8_t>(u8"args ", u8"= ", 3));
  bela::FPrintF(stderr, L"char8_t: %v\n", bela::string_cat<char8_t>(u8"args ", u8"= ", 3, u8",", 5));
  constexpr char32_t em = 0x1F603; // 😃 U+1F603
  bela::FPrintF(stderr, L"char8_t: %s\n",
                bela::string_cat<char8_t>(u8"Look emoji --> ", em, u8" see: U+", bela::Hex(em)));
  bela::FPrintF(stderr, L"char8_t: %v\n",
                bela::string_cat<char8_t>(u8"Look emoji --> ", em, u8" see: U+", bela::Hex(em), u8" jacksome"));
}

void stringshow() {
  bela::FPrintF(stderr, L"char: %v\n", bela::string_cat<char>(u8"args = 1"));
  bela::FPrintF(stderr, L"char: %v\n", bela::string_cat<char>(u8"args = ", 2));
  bela::FPrintF(stderr, L"char: %v\n", bela::string_cat<char>(u8"args ", u8"= ", 3));
  bela::FPrintF(stderr, L"char: %v\n", bela::string_cat<char>(u8"args ", u8"= ", 3, u8",", 5));
  constexpr char32_t em = 0x1F603; // 😃 U+1F603
  bela::FPrintF(stderr, L"char: %s\n", bela::string_cat<char>(u8"Look emoji --> ", em, u8" see: U+", bela::Hex(em)));
  bela::FPrintF(stderr, L"char: %v\n",
                bela::string_cat<char>(u8"Look emoji --> ", em, " see: U+", bela::Hex(em), u8" jacksome"));
}

void wstringshow() {
  bela::FPrintF(stderr, L"wchar_t: %v\n", bela::string_cat<wchar_t>(L"args = 1"));
  bela::FPrintF(stderr, L"wchar_t: %v\n", bela::string_cat<wchar_t>(L"args = ", 2));
  bela::FPrintF(stderr, L"wchar_t: %v\n", bela::string_cat<wchar_t>(L"args ", L"= ", 3));
  bela::FPrintF(stderr, L"wchar_t: %v\n", bela::string_cat<wchar_t>(L"args ", L"= ", 3, L",", 5));
  constexpr char32_t em = 0x1F603; // 😃 U+1F603
  bela::FPrintF(stderr, L"wchar_t: %s\n",
                bela::string_cat<wchar_t>(L"Look emoji --> ", em, L" see: U+", bela::Hex(em)));
  bela::FPrintF(stderr, L"wchar_t: %v\n",
                bela::string_cat<wchar_t>(L"Look emoji --> ", em, L" see: U+", bela::Hex(em), L" jacksome"));
}

int wmain(int argc, wchar_t **argv) {
  std::error_code ec;
  constexpr char32_t blueheart = U'💙';
  auto arg0 = std::filesystem::absolute(argv[0], ec);
  bela::basic_alphanum<wchar_t> was[] = {(std::numeric_limits<uint64_t>::max)(),
                                         (std::numeric_limits<uint64_t>::min)(),
                                         (std::numeric_limits<int64_t>::max)(),
                                         (std::numeric_limits<int64_t>::min)(),
                                         (std::numeric_limits<uint32_t>::max)(),
                                         (std::numeric_limits<uint32_t>::min)(),
                                         (std::numeric_limits<int32_t>::max)(),
                                         (std::numeric_limits<int32_t>::min)(),
                                         (std::numeric_limits<uint16_t>::max)(),
                                         (std::numeric_limits<uint16_t>::min)(),
                                         (std::numeric_limits<int16_t>::max)(),
                                         (std::numeric_limits<int16_t>::min)(),
                                         1,
                                         1.2f,
                                         L"hello world",
                                         u"hello world",
                                         u'\u00a9',
                                         L'我',
                                         L"这是一段简单的文字",
                                         blueheart,
                                         arg0,
                                         bela::Hex((std::numeric_limits<int32_t>::max)(), bela::kZeroPad20),
                                         bela::Hex((std::numeric_limits<int32_t>::min)(), bela::kZeroPad20),
                                         bela::Dec((std::numeric_limits<int32_t>::max)(), bela::kZeroPad20),
                                         bela::Dec((std::numeric_limits<int32_t>::min)(), bela::kZeroPad20)};
  for (const auto &a : was) {
    bela::FPrintF(stderr, L"wchar_t: %v\n", a.Piece());
  }
  bela::basic_alphanum<char16_t> uas[] = {(std::numeric_limits<uint64_t>::max)(),
                                          (std::numeric_limits<uint64_t>::min)(),
                                          (std::numeric_limits<int64_t>::max)(),
                                          (std::numeric_limits<int64_t>::min)(),
                                          (std::numeric_limits<uint32_t>::max)(),
                                          (std::numeric_limits<uint32_t>::min)(),
                                          (std::numeric_limits<int32_t>::max)(),
                                          (std::numeric_limits<int32_t>::min)(),
                                          (std::numeric_limits<uint16_t>::max)(),
                                          (std::numeric_limits<uint16_t>::min)(),
                                          (std::numeric_limits<int16_t>::max)(),
                                          (std::numeric_limits<int16_t>::min)(),
                                          1,
                                          1.2f,
                                          L"hello world",
                                          u"hello world",
                                          u'\u00a9',
                                          L'我',
                                          L"这是一段简单的文字",
                                          blueheart,
                                          arg0,
                                          bela::Hex((std::numeric_limits<int32_t>::max)(), bela::kZeroPad20),
                                          bela::Hex((std::numeric_limits<int32_t>::min)(), bela::kZeroPad20),
                                          bela::Dec((std::numeric_limits<int32_t>::max)(), bela::kZeroPad20),
                                          bela::Dec((std::numeric_limits<int32_t>::min)(), bela::kZeroPad20)};
  for (const auto &a : uas) {
    bela::FPrintF(stderr, L"char16_t: %v\n", a.Piece());
  }
  bela::basic_alphanum<char8_t> u8as[] = {(std::numeric_limits<uint64_t>::max)(),
                                          (std::numeric_limits<uint64_t>::min)(),
                                          (std::numeric_limits<int64_t>::max)(),
                                          (std::numeric_limits<int64_t>::min)(),
                                          (std::numeric_limits<uint32_t>::max)(),
                                          (std::numeric_limits<uint32_t>::min)(),
                                          (std::numeric_limits<int32_t>::max)(),
                                          (std::numeric_limits<int32_t>::min)(),
                                          (std::numeric_limits<uint16_t>::max)(),
                                          (std::numeric_limits<uint16_t>::min)(),
                                          (std::numeric_limits<int16_t>::max)(),
                                          (std::numeric_limits<int16_t>::min)(),
                                          1,
                                          1.2f,
                                          u8"hello world",
                                          "hello world",
                                          u8'U',
                                          'x',
                                          U'我',
                                          u8"这是一段简单的文字",
                                          blueheart,
                                          bela::Hex((std::numeric_limits<int32_t>::max)(), bela::kZeroPad20),
                                          bela::Hex((std::numeric_limits<int32_t>::min)(), bela::kZeroPad20),
                                          bela::Dec((std::numeric_limits<int32_t>::max)(), bela::kZeroPad20),
                                          bela::Dec((std::numeric_limits<int32_t>::min)(), bela::kZeroPad20)};
  for (const auto &a : u8as) {
    bela::FPrintF(stderr, L"char8_t: %v\n", a.Piece());
  }
  bela::basic_alphanum<char> cas[] = {(std::numeric_limits<uint64_t>::max)(),
                                      (std::numeric_limits<uint64_t>::min)(),
                                      (std::numeric_limits<int64_t>::max)(),
                                      (std::numeric_limits<int64_t>::min)(),
                                      (std::numeric_limits<uint32_t>::max)(),
                                      (std::numeric_limits<uint32_t>::min)(),
                                      (std::numeric_limits<int32_t>::max)(),
                                      (std::numeric_limits<int32_t>::min)(),
                                      (std::numeric_limits<uint16_t>::max)(),
                                      (std::numeric_limits<uint16_t>::min)(),
                                      (std::numeric_limits<int16_t>::max)(),
                                      (std::numeric_limits<int16_t>::min)(),
                                      1,
                                      1.97821741f,
                                      u8"hello world",
                                      "hello world",
                                      u8'U',
                                      'x',
                                      U'我',
                                      "这是一段简单的文字",
                                      blueheart,
                                      bela::Hex((std::numeric_limits<int32_t>::max)(), bela::kZeroPad20),
                                      bela::Hex((std::numeric_limits<int32_t>::min)(), bela::kZeroPad20),
                                      bela::Dec((std::numeric_limits<int32_t>::max)(), bela::kZeroPad20),
                                      bela::Dec((std::numeric_limits<int32_t>::min)(), bela::kZeroPad20)};
  for (const auto &a : cas) {
    bela::FPrintF(stderr, L"char: %v\n", a.Piece());
  }
  u8stringshow();
  stringshow();
  wstringshow();
  bela::FPrintF(stderr, L"LZMA_BUF_ERROR %v\n", bela::string_cat<char>(LZMA_BUF_ERROR));
  bela::FPrintF(stderr, L"LZMA_BUF_ERROR %v\n", bela::string_cat<wchar_t>(LZMA_BUF_ERROR));
  bela::FPrintF(stderr, L"LZMA_BUF_ERROR %v\n", bela::string_cat<char8_t>(LZMA_BUF_ERROR));
  bela::FPrintF(stderr, L"LZMA_BUF_ERROR %v\n", bela::string_cat<char16_t>(LZMA_BUF_ERROR));
  return 0;
}