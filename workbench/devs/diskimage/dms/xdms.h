/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 */

#ifndef XDMS_H
#define XDMS_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#include <string.h>

struct xdms_data {
	UWORD PWDCRC;
	UWORD quick_text_loc;
	UWORD medium_text_loc;
	UWORD heavy_text_loc;
	UWORD deep_text_loc;
	int init_deep_tabs;
	UBYTE *text;

	UBYTE *indata;
	ULONG bitcount;
	ULONG bitbuf;

	struct {
		UWORD freq[628]; // freq[T + 1]
		UWORD prnt[941]; // prnt[T + N_CHAR]
		UWORD son[627];  // son[T];
	} u_deep;

	struct {
		UWORD left[1019];  // left[2 * NC - 1]
		UWORD right[1028]; // right[2 * NC - 1 + 9]
		UBYTE c_len[510];   // c_len[NC]
		UBYTE pt_len[20];   // pt_len[NPT]
		UWORD c_table[4096];
		UWORD pt_table[256];
		UWORD lastlen, np;
	} u_heavy;
};

/* crc_csum.c */
UWORD Calc_CheckSum (const UBYTE *mem, ULONG size);
UWORD CreateCRC (const UBYTE *mem, ULONG size);

/* getbits.c */
extern const ULONG mask_bits[];
void initbitbuf (struct xdms_data *xdms, UBYTE *in);
#define INITBITBUF(in) initbitbuf(xdms, in)
#define GETBITS(n) ((UWORD)(xdms->bitbuf >> (xdms->bitcount-(n))))
#define DROPBITS(n) {xdms->bitbuf &= mask_bits[xdms->bitcount-=(n)]; while (xdms->bitcount<16) {xdms->bitbuf = (xdms->bitbuf << 8) | *xdms->indata++;  xdms->bitcount += 8;}}

/* tables.c */
extern const UBYTE d_code[];
extern const UBYTE d_len[];

/* maketbl.c */
UWORD make_table (struct xdms_data *xdms, UWORD nchar, UBYTE bitlen[],UWORD tablebits, UWORD table[]);

/* u_init.c */
void Init_Decrunchers (struct xdms_data *xdms);

/* u_deep.c */
UWORD Unpack_DEEP (struct xdms_data *xdms, UBYTE *in, UBYTE *out, UWORD origsize);

/* u_heavy.c */
UWORD Unpack_HEAVY (struct xdms_data *xdms, UBYTE *in, UBYTE *out, UBYTE flags, UWORD origsize);

/* u_medium.c */
UWORD Unpack_MEDIUM (struct xdms_data *xdms, UBYTE *in, UBYTE *out, UWORD origsize);

/* u_quick.c */
UWORD Unpack_QUICK(struct xdms_data *xdms, UBYTE *in, UBYTE *out, UWORD origsize);

/* u_rle.c */
UWORD Unpack_RLE (struct xdms_data *xdms, UBYTE *in, UBYTE *out, UWORD origsize);

#endif
