///
#ifndef DEFLATE64_INTERNAL_H
#define DEFLATE64_INTERNAL_H
#include <stdint.h>
#include <zlib.h>

/* Structure for decoding tables.  Each entry provides either the
   information needed to do the operation requested by the code that
   indexed that table entry, or it provides a pointer to another
   table that indexes more bits of the code.  op indicates whether
   the entry is a pointer to another table, a literal, a length or
   distance, an end-of-block, or an invalid code.  For a table
   pointer, the low four bits of op is the number of index bits of
   that table.  For a length or distance, the low four bits of op
   is the number of extra bits to get after the code.  bits is
   the number of bits in this code or part of the code to drop off
   of the bit buffer.  val is the actual byte to output in the case
   of a literal, the base length or distance, or the offset from
   the current table to the next table.  Each entry is four bytes. */
typedef struct {
  unsigned char op;   /* operation, extra bits, table bits */
  unsigned char bits; /* bits in this part of the code */
  unsigned short val; /* offset in table or code value */
} code;

/* op values as set by inflate_table():
    00000000 - literal
    0000tttt - table link, tttt != 0 is the number of table index bits
    100eeeee - length or distance, eeee is the number of extra bits
    01100000 - end of block
    01000000 - invalid code
 */

/* Maximum size of the dynamic table.  The maximum number of code structures is
   1446, which is the sum of 852 for literal/length codes and 594 for distance
   codes.  These values were found by exhaustive searches using the program
   examples/enough.c found in the zlib distribtution.  The arguments to that
   program are the number of symbols, the initial root table size, and the
   maximum bit length of a code.  "enough 286 9 15" for literal/length codes
   returns returns 852, and "enough 32 6 15" for distance codes returns 594.
   The initial root table size (9 or 6) is found in the fifth argument of the
   inflate_table() calls in infback9.c.  If the root table size is changed,
   then these maximum sizes would be need to be recalculated and updated. */
#define ENOUGH_LENS 852
#define ENOUGH_DISTS 594
#define ENOUGH (ENOUGH_LENS + ENOUGH_DISTS)

/* Type of code to build for inflate_table9() */
typedef enum { CODES, LENS, DISTS } codetype;

extern int inflate_table9(codetype type, unsigned short *lens, unsigned codes,
                          code **table, unsigned *bits, unsigned short *work);

/* Possible inflate modes between inflate() calls */
typedef enum {
  TYPE,   /* i: waiting for type bits, including last-flag bit */
  STORED, /* i: waiting for stored size (length and complement) */
  TABLE,  /* i: waiting for dynamic block table lengths */
  LEN,    /* i: waiting for length/lit code */
  DONE,   /* finished check, done -- remain here until reset */
  BAD     /* got a data error -- remain here until reset */
} inflate_mode;

/*
    State transitions between above modes -

    (most modes can go to the BAD mode -- not shown for clarity)

    Read deflate blocks:
            TYPE -> STORED or TABLE or LEN or DONE
            STORED -> TYPE
            TABLE -> LENLENS -> CODELENS -> LEN
    Read deflate codes:
                LEN -> LEN or TYPE
 */

/* state maintained between inflate() calls.  Approximately 7K bytes. */
struct inflate_state {
  /* sliding window */
  unsigned char *window;    /* allocated sliding window, if needed */
                            /* dynamic table building */
  unsigned ncode;           /* number of code length code lengths */
  unsigned nlen;            /* number of length code lengths */
  unsigned ndist;           /* number of distance code lengths */
  unsigned have;            /* number of code lengths in lens[] */
  code *next;               /* next available space in codes[] */
  unsigned short lens[320]; /* temporary storage for code lengths */
  unsigned short work[288]; /* work area for code table building */
  code codes[ENOUGH];       /* space for code tables */
};

#endif
