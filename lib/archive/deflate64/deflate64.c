///
#include <stdlib.h>
#include <string.h>
#include "deflate64.h"
#include "deflate64_internal.h"

#define WSIZE 65536UL

voidpf inline xcalloc(voidpf opaque, unsigned items, unsigned size) {
  (void)opaque;
  return (voidpf)malloc(items * size);
}

void inline xcfree(voidpf opaque, voidpf ptr) {
  (void)opaque;
  free(ptr);
}

int inflateBack9Init_(z_stream *strm, unsigned char *window,
                      const char *version, int stream_size) {
  struct inflate_state *state;

  if (version == Z_NULL || version[0] != ZLIB_VERSION[0] ||
      stream_size != (int)(sizeof(z_stream)))
    return Z_VERSION_ERROR;
  if (strm == Z_NULL || window == Z_NULL)
    return Z_STREAM_ERROR;
  strm->msg = Z_NULL; /* in case we return an error */
  if (strm->zalloc == (alloc_func)0) {
    strm->zalloc = xcalloc;
    strm->opaque = (voidpf)0;
  }
  if (strm->zfree == (free_func)0)
    strm->zfree = xcfree;
  state = (struct inflate_state *)strm->zalloc(strm->opaque, 1,
                                               sizeof(struct inflate_state));
  if (state == Z_NULL)
    return Z_MEM_ERROR;
  strm->state = (voidpf)state;
  state->window = window;
  return Z_OK;
}

/* Clear the input bit accumulator */
#define INITBITS()                                                             \
  do {                                                                         \
    hold = 0;                                                                  \
    bits = 0;                                                                  \
  } while (0)

/* Assure that some input is available.  If input is requested, but denied,
   then return a Z_BUF_ERROR from inflateBack(). */
#define PULL()                                                                 \
  do {                                                                         \
    if (have == 0) {                                                           \
      have = in(in_desc, &next);                                               \
      if (have == 0) {                                                         \
        next = Z_NULL;                                                         \
        ret = Z_BUF_ERROR;                                                     \
        goto inf_leave;                                                        \
      }                                                                        \
    }                                                                          \
  } while (0)

/* Get a byte of input into the bit accumulator, or return from inflateBack()
   with an error if there is no input available. */
#define PULLBYTE()                                                             \
  do {                                                                         \
    PULL();                                                                    \
    have--;                                                                    \
    hold += (unsigned long)(*next++) << bits;                                  \
    bits += 8;                                                                 \
  } while (0)

/* Assure that there are at least n bits in the bit accumulator.  If there is
   not enough available input to do that, then return from inflateBack() with
   an error. */
#define NEEDBITS(n)                                                            \
  do {                                                                         \
    while (bits < (unsigned)(n))                                               \
      PULLBYTE();                                                              \
  } while (0)

/* Return the low n bits of the bit accumulator (n <= 16) */
#define BITS(n) ((unsigned)hold & ((1U << (n)) - 1))

/* Remove n bits from the bit accumulator */
#define DROPBITS(n)                                                            \
  do {                                                                         \
    hold >>= (n);                                                              \
    bits -= (unsigned)(n);                                                     \
  } while (0)

/* Remove zero to seven bits as needed to go to a byte boundary */
#define BYTEBITS()                                                             \
  do {                                                                         \
    hold >>= bits & 7;                                                         \
    bits -= bits & 7;                                                          \
  } while (0)

/* Assure that some output space is available, by writing out the window
   if it's full.  If the write fails, return from inflateBack() with a
   Z_BUF_ERROR. */
#define ROOM()                                                                 \
  do {                                                                         \
    if (left == 0) {                                                           \
      put = window;                                                            \
      left = WSIZE;                                                            \
      wrap = 1;                                                                \
      if (out(out_desc, put, (unsigned)left)) {                                \
        ret = Z_BUF_ERROR;                                                     \
        goto inf_leave;                                                        \
      }                                                                        \
    }                                                                          \
  } while (0)

/*
   strm provides the memory allocation functions and window buffer on input,
   and provides information on the unused input on return.  For Z_DATA_ERROR
   returns, strm will also provide an error message.

   in() and out() are the call-back input and output functions.  When
   inflateBack() needs more input, it calls in().  When inflateBack() has
   filled the window with output, or when it completes with data in the
   window, it calls out() to write out the data.  The application must not
   change the provided input until in() is called again or inflateBack()
   returns.  The application must not change the window/output buffer until
   inflateBack() returns.

   in() and out() are called with a descriptor parameter provided in the
   inflateBack() call.  This parameter can be a structure that provides the
   information required to do the read or write, as well as accumulated
   information on the input and output such as totals and check values.

   in() should return zero on failure.  out() should return non-zero on
   failure.  If either in() or out() fails, than inflateBack() returns a
   Z_BUF_ERROR.  strm->next_in can be checked for Z_NULL to see whether it
   was in() or out() that caused in the error.  Otherwise,  inflateBack()
   returns Z_STREAM_END on success, Z_DATA_ERROR for an deflate format
   error, or Z_MEM_ERROR if it could not allocate memory for the state.
   inflateBack() can also return Z_STREAM_ERROR if the input parameters
   are not correct, i.e. strm is Z_NULL or the state was not initialized.
 */

int inflateBack9(z_stream *strm, in_func in, void *in_desc, out_func out,
                 void *out_desc) {
  struct inflate_state *state;
  z_const unsigned char *next; /* next input */
  unsigned char *put;          /* next output */
  unsigned have;               /* available input */
  unsigned long left;          /* available output */
  inflate_mode mode;           /* current inflate mode */
  int lastblock;               /* true if processing last block */
  int wrap;                    /* true if the window has wrapped */
  unsigned char *window;       /* allocated sliding window, if needed */
  unsigned long hold;          /* bit buffer */
  unsigned bits;               /* bits in bit buffer */
  unsigned extra;              /* extra bits needed */
  unsigned long length;        /* literal or length of data to copy */
  unsigned long offset;        /* distance back to copy string from */
  unsigned long copy;          /* number of stored or match bytes to copy */
  unsigned char *from;         /* where to copy match bytes from */
  code const *lencode;         /* starting table for length/literal codes */
  code const *distcode;        /* starting table for distance codes */
  unsigned lenbits;            /* index bits for lencode */
  unsigned distbits;           /* index bits for distcode */
  code here;                   /* current decoding table entry */
  code last;                   /* parent table entry */
  unsigned len;                /* length to copy for repeats, bits to drop */
  int ret;                     /* return code */
  static const unsigned short order[19] = /* permutation of code lengths */
      {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

  static const code lenfix[512] = {
      {96, 7, 0},    {0, 8, 80},    {0, 8, 16},    {132, 8, 115}, {130, 7, 31},
      {0, 8, 112},   {0, 8, 48},    {0, 9, 192},   {128, 7, 10},  {0, 8, 96},
      {0, 8, 32},    {0, 9, 160},   {0, 8, 0},     {0, 8, 128},   {0, 8, 64},
      {0, 9, 224},   {128, 7, 6},   {0, 8, 88},    {0, 8, 24},    {0, 9, 144},
      {131, 7, 59},  {0, 8, 120},   {0, 8, 56},    {0, 9, 208},   {129, 7, 17},
      {0, 8, 104},   {0, 8, 40},    {0, 9, 176},   {0, 8, 8},     {0, 8, 136},
      {0, 8, 72},    {0, 9, 240},   {128, 7, 4},   {0, 8, 84},    {0, 8, 20},
      {133, 8, 227}, {131, 7, 43},  {0, 8, 116},   {0, 8, 52},    {0, 9, 200},
      {129, 7, 13},  {0, 8, 100},   {0, 8, 36},    {0, 9, 168},   {0, 8, 4},
      {0, 8, 132},   {0, 8, 68},    {0, 9, 232},   {128, 7, 8},   {0, 8, 92},
      {0, 8, 28},    {0, 9, 152},   {132, 7, 83},  {0, 8, 124},   {0, 8, 60},
      {0, 9, 216},   {130, 7, 23},  {0, 8, 108},   {0, 8, 44},    {0, 9, 184},
      {0, 8, 12},    {0, 8, 140},   {0, 8, 76},    {0, 9, 248},   {128, 7, 3},
      {0, 8, 82},    {0, 8, 18},    {133, 8, 163}, {131, 7, 35},  {0, 8, 114},
      {0, 8, 50},    {0, 9, 196},   {129, 7, 11},  {0, 8, 98},    {0, 8, 34},
      {0, 9, 164},   {0, 8, 2},     {0, 8, 130},   {0, 8, 66},    {0, 9, 228},
      {128, 7, 7},   {0, 8, 90},    {0, 8, 26},    {0, 9, 148},   {132, 7, 67},
      {0, 8, 122},   {0, 8, 58},    {0, 9, 212},   {130, 7, 19},  {0, 8, 106},
      {0, 8, 42},    {0, 9, 180},   {0, 8, 10},    {0, 8, 138},   {0, 8, 74},
      {0, 9, 244},   {128, 7, 5},   {0, 8, 86},    {0, 8, 22},    {65, 8, 0},
      {131, 7, 51},  {0, 8, 118},   {0, 8, 54},    {0, 9, 204},   {129, 7, 15},
      {0, 8, 102},   {0, 8, 38},    {0, 9, 172},   {0, 8, 6},     {0, 8, 134},
      {0, 8, 70},    {0, 9, 236},   {128, 7, 9},   {0, 8, 94},    {0, 8, 30},
      {0, 9, 156},   {132, 7, 99},  {0, 8, 126},   {0, 8, 62},    {0, 9, 220},
      {130, 7, 27},  {0, 8, 110},   {0, 8, 46},    {0, 9, 188},   {0, 8, 14},
      {0, 8, 142},   {0, 8, 78},    {0, 9, 252},   {96, 7, 0},    {0, 8, 81},
      {0, 8, 17},    {133, 8, 131}, {130, 7, 31},  {0, 8, 113},   {0, 8, 49},
      {0, 9, 194},   {128, 7, 10},  {0, 8, 97},    {0, 8, 33},    {0, 9, 162},
      {0, 8, 1},     {0, 8, 129},   {0, 8, 65},    {0, 9, 226},   {128, 7, 6},
      {0, 8, 89},    {0, 8, 25},    {0, 9, 146},   {131, 7, 59},  {0, 8, 121},
      {0, 8, 57},    {0, 9, 210},   {129, 7, 17},  {0, 8, 105},   {0, 8, 41},
      {0, 9, 178},   {0, 8, 9},     {0, 8, 137},   {0, 8, 73},    {0, 9, 242},
      {128, 7, 4},   {0, 8, 85},    {0, 8, 21},    {144, 8, 3},   {131, 7, 43},
      {0, 8, 117},   {0, 8, 53},    {0, 9, 202},   {129, 7, 13},  {0, 8, 101},
      {0, 8, 37},    {0, 9, 170},   {0, 8, 5},     {0, 8, 133},   {0, 8, 69},
      {0, 9, 234},   {128, 7, 8},   {0, 8, 93},    {0, 8, 29},    {0, 9, 154},
      {132, 7, 83},  {0, 8, 125},   {0, 8, 61},    {0, 9, 218},   {130, 7, 23},
      {0, 8, 109},   {0, 8, 45},    {0, 9, 186},   {0, 8, 13},    {0, 8, 141},
      {0, 8, 77},    {0, 9, 250},   {128, 7, 3},   {0, 8, 83},    {0, 8, 19},
      {133, 8, 195}, {131, 7, 35},  {0, 8, 115},   {0, 8, 51},    {0, 9, 198},
      {129, 7, 11},  {0, 8, 99},    {0, 8, 35},    {0, 9, 166},   {0, 8, 3},
      {0, 8, 131},   {0, 8, 67},    {0, 9, 230},   {128, 7, 7},   {0, 8, 91},
      {0, 8, 27},    {0, 9, 150},   {132, 7, 67},  {0, 8, 123},   {0, 8, 59},
      {0, 9, 214},   {130, 7, 19},  {0, 8, 107},   {0, 8, 43},    {0, 9, 182},
      {0, 8, 11},    {0, 8, 139},   {0, 8, 75},    {0, 9, 246},   {128, 7, 5},
      {0, 8, 87},    {0, 8, 23},    {77, 8, 0},    {131, 7, 51},  {0, 8, 119},
      {0, 8, 55},    {0, 9, 206},   {129, 7, 15},  {0, 8, 103},   {0, 8, 39},
      {0, 9, 174},   {0, 8, 7},     {0, 8, 135},   {0, 8, 71},    {0, 9, 238},
      {128, 7, 9},   {0, 8, 95},    {0, 8, 31},    {0, 9, 158},   {132, 7, 99},
      {0, 8, 127},   {0, 8, 63},    {0, 9, 222},   {130, 7, 27},  {0, 8, 111},
      {0, 8, 47},    {0, 9, 190},   {0, 8, 15},    {0, 8, 143},   {0, 8, 79},
      {0, 9, 254},   {96, 7, 0},    {0, 8, 80},    {0, 8, 16},    {132, 8, 115},
      {130, 7, 31},  {0, 8, 112},   {0, 8, 48},    {0, 9, 193},   {128, 7, 10},
      {0, 8, 96},    {0, 8, 32},    {0, 9, 161},   {0, 8, 0},     {0, 8, 128},
      {0, 8, 64},    {0, 9, 225},   {128, 7, 6},   {0, 8, 88},    {0, 8, 24},
      {0, 9, 145},   {131, 7, 59},  {0, 8, 120},   {0, 8, 56},    {0, 9, 209},
      {129, 7, 17},  {0, 8, 104},   {0, 8, 40},    {0, 9, 177},   {0, 8, 8},
      {0, 8, 136},   {0, 8, 72},    {0, 9, 241},   {128, 7, 4},   {0, 8, 84},
      {0, 8, 20},    {133, 8, 227}, {131, 7, 43},  {0, 8, 116},   {0, 8, 52},
      {0, 9, 201},   {129, 7, 13},  {0, 8, 100},   {0, 8, 36},    {0, 9, 169},
      {0, 8, 4},     {0, 8, 132},   {0, 8, 68},    {0, 9, 233},   {128, 7, 8},
      {0, 8, 92},    {0, 8, 28},    {0, 9, 153},   {132, 7, 83},  {0, 8, 124},
      {0, 8, 60},    {0, 9, 217},   {130, 7, 23},  {0, 8, 108},   {0, 8, 44},
      {0, 9, 185},   {0, 8, 12},    {0, 8, 140},   {0, 8, 76},    {0, 9, 249},
      {128, 7, 3},   {0, 8, 82},    {0, 8, 18},    {133, 8, 163}, {131, 7, 35},
      {0, 8, 114},   {0, 8, 50},    {0, 9, 197},   {129, 7, 11},  {0, 8, 98},
      {0, 8, 34},    {0, 9, 165},   {0, 8, 2},     {0, 8, 130},   {0, 8, 66},
      {0, 9, 229},   {128, 7, 7},   {0, 8, 90},    {0, 8, 26},    {0, 9, 149},
      {132, 7, 67},  {0, 8, 122},   {0, 8, 58},    {0, 9, 213},   {130, 7, 19},
      {0, 8, 106},   {0, 8, 42},    {0, 9, 181},   {0, 8, 10},    {0, 8, 138},
      {0, 8, 74},    {0, 9, 245},   {128, 7, 5},   {0, 8, 86},    {0, 8, 22},
      {65, 8, 0},    {131, 7, 51},  {0, 8, 118},   {0, 8, 54},    {0, 9, 205},
      {129, 7, 15},  {0, 8, 102},   {0, 8, 38},    {0, 9, 173},   {0, 8, 6},
      {0, 8, 134},   {0, 8, 70},    {0, 9, 237},   {128, 7, 9},   {0, 8, 94},
      {0, 8, 30},    {0, 9, 157},   {132, 7, 99},  {0, 8, 126},   {0, 8, 62},
      {0, 9, 221},   {130, 7, 27},  {0, 8, 110},   {0, 8, 46},    {0, 9, 189},
      {0, 8, 14},    {0, 8, 142},   {0, 8, 78},    {0, 9, 253},   {96, 7, 0},
      {0, 8, 81},    {0, 8, 17},    {133, 8, 131}, {130, 7, 31},  {0, 8, 113},
      {0, 8, 49},    {0, 9, 195},   {128, 7, 10},  {0, 8, 97},    {0, 8, 33},
      {0, 9, 163},   {0, 8, 1},     {0, 8, 129},   {0, 8, 65},    {0, 9, 227},
      {128, 7, 6},   {0, 8, 89},    {0, 8, 25},    {0, 9, 147},   {131, 7, 59},
      {0, 8, 121},   {0, 8, 57},    {0, 9, 211},   {129, 7, 17},  {0, 8, 105},
      {0, 8, 41},    {0, 9, 179},   {0, 8, 9},     {0, 8, 137},   {0, 8, 73},
      {0, 9, 243},   {128, 7, 4},   {0, 8, 85},    {0, 8, 21},    {144, 8, 3},
      {131, 7, 43},  {0, 8, 117},   {0, 8, 53},    {0, 9, 203},   {129, 7, 13},
      {0, 8, 101},   {0, 8, 37},    {0, 9, 171},   {0, 8, 5},     {0, 8, 133},
      {0, 8, 69},    {0, 9, 235},   {128, 7, 8},   {0, 8, 93},    {0, 8, 29},
      {0, 9, 155},   {132, 7, 83},  {0, 8, 125},   {0, 8, 61},    {0, 9, 219},
      {130, 7, 23},  {0, 8, 109},   {0, 8, 45},    {0, 9, 187},   {0, 8, 13},
      {0, 8, 141},   {0, 8, 77},    {0, 9, 251},   {128, 7, 3},   {0, 8, 83},
      {0, 8, 19},    {133, 8, 195}, {131, 7, 35},  {0, 8, 115},   {0, 8, 51},
      {0, 9, 199},   {129, 7, 11},  {0, 8, 99},    {0, 8, 35},    {0, 9, 167},
      {0, 8, 3},     {0, 8, 131},   {0, 8, 67},    {0, 9, 231},   {128, 7, 7},
      {0, 8, 91},    {0, 8, 27},    {0, 9, 151},   {132, 7, 67},  {0, 8, 123},
      {0, 8, 59},    {0, 9, 215},   {130, 7, 19},  {0, 8, 107},   {0, 8, 43},
      {0, 9, 183},   {0, 8, 11},    {0, 8, 139},   {0, 8, 75},    {0, 9, 247},
      {128, 7, 5},   {0, 8, 87},    {0, 8, 23},    {77, 8, 0},    {131, 7, 51},
      {0, 8, 119},   {0, 8, 55},    {0, 9, 207},   {129, 7, 15},  {0, 8, 103},
      {0, 8, 39},    {0, 9, 175},   {0, 8, 7},     {0, 8, 135},   {0, 8, 71},
      {0, 9, 239},   {128, 7, 9},   {0, 8, 95},    {0, 8, 31},    {0, 9, 159},
      {132, 7, 99},  {0, 8, 127},   {0, 8, 63},    {0, 9, 223},   {130, 7, 27},
      {0, 8, 111},   {0, 8, 47},    {0, 9, 191},   {0, 8, 15},    {0, 8, 143},
      {0, 8, 79},    {0, 9, 255}};

  static const code distfix[32] = {
      {128, 5, 1},  {135, 5, 257},  {131, 5, 17},  {139, 5, 4097},
      {129, 5, 5},  {137, 5, 1025}, {133, 5, 65},  {141, 5, 16385},
      {128, 5, 3},  {136, 5, 513},  {132, 5, 33},  {140, 5, 8193},
      {130, 5, 9},  {138, 5, 2049}, {134, 5, 129}, {142, 5, 32769},
      {128, 5, 2},  {135, 5, 385},  {131, 5, 25},  {139, 5, 6145},
      {129, 5, 7},  {137, 5, 1537}, {133, 5, 97},  {141, 5, 24577},
      {128, 5, 4},  {136, 5, 769},  {132, 5, 49},  {140, 5, 12289},
      {130, 5, 13}, {138, 5, 3073}, {134, 5, 193}, {142, 5, 49153}};
  /* Check that the strm exists and that the state was initialized */
  if (strm == Z_NULL || strm->state == Z_NULL)
    return Z_STREAM_ERROR;
  state = (struct inflate_state *)strm->state;

  /* Reset the state */
  strm->msg = Z_NULL;
  mode = TYPE;
  lastblock = 0;
  wrap = 0;
  window = state->window;
  next = strm->next_in;
  have = next != Z_NULL ? strm->avail_in : 0;
  hold = 0;
  bits = 0;
  put = window;
  left = WSIZE;
  lencode = Z_NULL;
  distcode = Z_NULL;

  /* Inflate until end of block marked as last */
  for (;;)
    switch (mode) {
    case TYPE:
      /* determine and dispatch block type */
      if (lastblock) {
        BYTEBITS();
        mode = DONE;
        break;
      }
      NEEDBITS(3);
      lastblock = BITS(1);
      DROPBITS(1);
      switch (BITS(2)) {
      case 0: /* stored block */
        mode = STORED;
        break;
      case 1: /* fixed block */
        lencode = lenfix;
        lenbits = 9;
        distcode = distfix;
        distbits = 5;
        mode = LEN; /* decode codes */
        break;
      case 2: /* dynamic block */
        mode = TABLE;
        break;
      case 3:
        strm->msg = (char *)"invalid block type";
        mode = BAD;
      }
      DROPBITS(2);
      break;

    case STORED:
      /* get and verify stored block length */
      BYTEBITS(); /* go to byte boundary */
      NEEDBITS(32);
      if ((hold & 0xffff) != ((hold >> 16) ^ 0xffff)) {
        strm->msg = (char *)"invalid stored block lengths";
        mode = BAD;
        break;
      }
      length = (unsigned)hold & 0xffff;
      INITBITS();

      /* copy stored block from input to output */
      while (length != 0) {
        copy = length;
        PULL();
        ROOM();
        if (copy > have)
          copy = have;
        if (copy > left)
          copy = left;
        memcpy(put, next, copy);
        have -= copy;
        next += copy;
        left -= copy;
        put += copy;
        length -= copy;
      }
      mode = TYPE;
      break;

    case TABLE:
      /* get dynamic table entries descriptor */
      NEEDBITS(14);
      state->nlen = BITS(5) + 257;
      DROPBITS(5);
      state->ndist = BITS(5) + 1;
      DROPBITS(5);
      state->ncode = BITS(4) + 4;
      DROPBITS(4);
      if (state->nlen > 286) {
        strm->msg = (char *)"too many length symbols";
        mode = BAD;
        break;
      }

      /* get code length code lengths (not a typo) */
      state->have = 0;
      while (state->have < state->ncode) {
        NEEDBITS(3);
        state->lens[order[state->have++]] = (unsigned short)BITS(3);
        DROPBITS(3);
      }
      while (state->have < 19)
        state->lens[order[state->have++]] = 0;
      state->next = state->codes;
      lencode = (code const *)(state->next);
      lenbits = 7;
      ret = inflate_table9(CODES, state->lens, 19, &(state->next), &(lenbits),
                           state->work);
      if (ret) {
        strm->msg = (char *)"invalid code lengths set";
        mode = BAD;
        break;
      }

      /* get length and distance code code lengths */
      state->have = 0;
      while (state->have < state->nlen + state->ndist) {
        for (;;) {
          here = lencode[BITS(lenbits)];
          if ((unsigned)(here.bits) <= bits)
            break;
          PULLBYTE();
        }
        if (here.val < 16) {
          NEEDBITS(here.bits);
          DROPBITS(here.bits);
          state->lens[state->have++] = here.val;
        } else {
          if (here.val == 16) {
            NEEDBITS(here.bits + 2);
            DROPBITS(here.bits);
            if (state->have == 0) {
              strm->msg = (char *)"invalid bit length repeat";
              mode = BAD;
              break;
            }
            len = (unsigned)(state->lens[state->have - 1]);
            copy = 3 + BITS(2);
            DROPBITS(2);
          } else if (here.val == 17) {
            NEEDBITS(here.bits + 3);
            DROPBITS(here.bits);
            len = 0;
            copy = 3 + BITS(3);
            DROPBITS(3);
          } else {
            NEEDBITS(here.bits + 7);
            DROPBITS(here.bits);
            len = 0;
            copy = 11 + BITS(7);
            DROPBITS(7);
          }
          if (state->have + copy > state->nlen + state->ndist) {
            strm->msg = (char *)"invalid bit length repeat";
            mode = BAD;
            break;
          }
          while (copy--)
            state->lens[state->have++] = (unsigned short)len;
        }
      }

      /* handle error breaks in while */
      if (mode == BAD)
        break;

      /* check for end-of-block code (better have one) */
      if (state->lens[256] == 0) {
        strm->msg = (char *)"invalid code -- missing end-of-block";
        mode = BAD;
        break;
      }

      /* build code tables -- note: do not change the lenbits or distbits
         values here (9 and 6) without reading the comments in inftree9.h
         concerning the ENOUGH constants, which depend on those values */
      state->next = state->codes;
      lencode = (code const *)(state->next);
      lenbits = 9;
      ret = inflate_table9(LENS, state->lens, state->nlen, &(state->next),
                           &(lenbits), state->work);
      if (ret) {
        strm->msg = (char *)"invalid literal/lengths set";
        mode = BAD;
        break;
      }
      distcode = (code const *)(state->next);
      distbits = 6;
      ret = inflate_table9(DISTS, state->lens + state->nlen, state->ndist,
                           &(state->next), &(distbits), state->work);
      if (ret) {
        strm->msg = (char *)"invalid distances set";
        mode = BAD;
        break;
      }
      mode = LEN;

    case LEN:
      /* get a literal, length, or end-of-block code */
      for (;;) {
        here = lencode[BITS(lenbits)];
        if ((unsigned)(here.bits) <= bits)
          break;
        PULLBYTE();
      }
      if (here.op && (here.op & 0xf0) == 0) {
        last = here;
        for (;;) {
          here = lencode[last.val + (BITS(last.bits + last.op) >> last.bits)];
          if ((unsigned)(last.bits + here.bits) <= bits)
            break;
          PULLBYTE();
        }
        DROPBITS(last.bits);
      }
      DROPBITS(here.bits);
      length = (unsigned)here.val;

      /* process literal */
      if (here.op == 0) {
        ROOM();
        *put++ = (unsigned char)(length);
        left--;
        mode = LEN;
        break;
      }

      /* process end of block */
      if (here.op & 32) {
        mode = TYPE;
        break;
      }

      /* invalid code */
      if (here.op & 64) {
        strm->msg = (char *)"invalid literal/length code";
        mode = BAD;
        break;
      }

      /* length code -- get extra bits, if any */
      extra = (unsigned)(here.op) & 31;
      if (extra != 0) {
        NEEDBITS(extra);
        length += BITS(extra);
        DROPBITS(extra);
      }

      /* get distance code */
      for (;;) {
        here = distcode[BITS(distbits)];
        if ((unsigned)(here.bits) <= bits)
          break;
        PULLBYTE();
      }
      if ((here.op & 0xf0) == 0) {
        last = here;
        for (;;) {
          here = distcode[last.val + (BITS(last.bits + last.op) >> last.bits)];
          if ((unsigned)(last.bits + here.bits) <= bits)
            break;
          PULLBYTE();
        }
        DROPBITS(last.bits);
      }
      DROPBITS(here.bits);
      if (here.op & 64) {
        strm->msg = (char *)"invalid distance code";
        mode = BAD;
        break;
      }
      offset = (unsigned)here.val;

      /* get distance extra bits, if any */
      extra = (unsigned)(here.op) & 15;
      if (extra != 0) {
        NEEDBITS(extra);
        offset += BITS(extra);
        DROPBITS(extra);
      }
      if (offset > WSIZE - (wrap ? 0 : left)) {
        strm->msg = (char *)"invalid distance too far back";
        mode = BAD;
        break;
      }

      /* copy match from window to output */
      do {
        ROOM();
        copy = WSIZE - offset;
        if (copy < left) {
          from = put + copy;
          copy = left - copy;
        } else {
          from = put - offset;
          copy = left;
        }
        if (copy > length)
          copy = length;
        length -= copy;
        left -= copy;
        do {
          *put++ = *from++;
        } while (--copy);
      } while (length != 0);
      break;

    case DONE:
      /* inflate stream terminated properly -- write leftover output */
      ret = Z_STREAM_END;
      if (left < WSIZE) {
        if (out(out_desc, window, (unsigned)(WSIZE - left)))
          ret = Z_BUF_ERROR;
      }
      goto inf_leave;

    case BAD:
      ret = Z_DATA_ERROR;
      goto inf_leave;

    default: /* can't happen, but makes compilers happy */
      ret = Z_STREAM_ERROR;
      goto inf_leave;
    }

  /* Return unused input */
inf_leave:
  strm->next_in = next;
  strm->avail_in = have;
  return ret;
}

// int inflate9(z_streamp *strm, int flush) {
//   //
//   return 0;
// }

int inflateBack9End(z_stream *strm) {
  if (strm == Z_NULL || strm->state == Z_NULL || strm->zfree == (free_func)0)
    return Z_STREAM_ERROR;
  strm->zfree(strm->opaque, strm->state);
  strm->state = Z_NULL;
  return Z_OK;
}
