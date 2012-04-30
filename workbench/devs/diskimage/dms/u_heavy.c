/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *     Lempel-Ziv-Huffman decompression functions used in Heavy 1 & 2 
 *     compression modes. Based on LZH decompression functions from
 *     UNIX LHA made by Masaru Oki
 *
 */

#include "xdms.h"

#define NC 510
#define NPT 20
#define N1 510
#define OFFSET 253

#if 0
UWORD left[2 * NC - 1], right[2 * NC - 1 + 9];
static UBYTE c_len[NC], pt_len[NPT];
static UWORD c_table[4096], pt_table[256];
static UWORD lastlen, np;
#endif

static UWORD read_tree_c (struct xdms_data *xdms);
static UWORD read_tree_p (struct xdms_data *xdms);
static inline UWORD decode_c (struct xdms_data *xdms);
static inline UWORD decode_p (struct xdms_data *xdms);

UWORD Unpack_HEAVY (struct xdms_data *xdms, UBYTE *in, UBYTE *out, UBYTE flags, UWORD origsize) {
	UWORD heavy_text_loc = xdms->heavy_text_loc;
	UBYTE *text = xdms->text;
	UWORD j, i, c, bitmask;
	UBYTE *outend;

	/*  Heavy 1 uses a 4Kb dictionary,  Heavy 2 uses 8Kb  */

	if (flags & 8) {
		xdms->u_heavy.np = 15;
		bitmask = 0x1fff;
	} else {
		xdms->u_heavy.np = 14;
		bitmask = 0x0fff;
	}

	INITBITBUF(in);

	if (flags & 2) {
		if (read_tree_c(xdms)) return 1;
		if (read_tree_p(xdms)) return 2;
	}

	outend = out+origsize;

	while (out<outend) {
		c = decode_c(xdms);
		if (c < 256) {
			*out++ = text[heavy_text_loc++ & bitmask] = (UBYTE)c;
		} else {
			j = (UWORD)(c - OFFSET);
			i = (UWORD)(heavy_text_loc - decode_p(xdms) - 1);
			while(j--) *out++ = text[heavy_text_loc++ & bitmask] = text[i++ & bitmask];
		}
	}

	xdms->heavy_text_loc = heavy_text_loc;

	return 0;
}

static UWORD decode_c (struct xdms_data *xdms) {
	const UWORD * const left = xdms->u_heavy.left;
	const UWORD * const right = xdms->u_heavy.right;
	UWORD * const c_table = xdms->u_heavy.c_table;
	UBYTE * const c_len = xdms->u_heavy.c_len;
	UWORD i, j, m;

	j = c_table[GETBITS(12)];
	if (j < N1) {
		DROPBITS(c_len[j]);
	} else {
		DROPBITS(12);
		i = GETBITS(16);
		m = 0x8000;
		do {
			if (i & m) j = right[j];
			else j = left [j];
			m >>= 1;
		} while (j >= N1);
		DROPBITS(c_len[j] - 12);
	}
	return j;
}

static UWORD decode_p (struct xdms_data *xdms) {
	const UWORD * const left = xdms->u_heavy.left;
	const UWORD * const right = xdms->u_heavy.right;
	UWORD * const pt_table = xdms->u_heavy.pt_table;
	UBYTE * const pt_len = xdms->u_heavy.pt_len;
	const UWORD np = xdms->u_heavy.np;
	UWORD i, j, m;

	j = pt_table[GETBITS(8)];
	if (j < np) {
		DROPBITS(pt_len[j]);
	} else {
		DROPBITS(8);
		i = GETBITS(16);
		m = 0x8000;
		do {
			if (i & m) j = right[j];
			else j = left [j];
			m >>= 1;
		} while (j >= np);
		DROPBITS(pt_len[j] - 8);
	}

	if (j != np-1) {
		if (j > 0) {
			j = (UWORD)(GETBITS(i=(UWORD)(j-1)) | (1U << (j-1)));
			DROPBITS(i);
		}
		xdms->u_heavy.lastlen=j;
	}

	return xdms->u_heavy.lastlen;

}

static UWORD read_tree_c (struct xdms_data *xdms) {
	UWORD * const c_table = xdms->u_heavy.c_table;
	UBYTE * const c_len = xdms->u_heavy.c_len;
	UWORD i,n;

	n = GETBITS(9);
	DROPBITS(9);
	if (n>0){
		for (i=0; i<n; i++) {
			c_len[i] = (UBYTE)GETBITS(5);
			DROPBITS(5);
		}
		for (i=n; i<510; i++) c_len[i] = 0;
		if (make_table(xdms,510,c_len,12,c_table)) return 1;
	} else {
		n = GETBITS(9);
		DROPBITS(9);
		for (i=0; i<510; i++) c_len[i] = 0;
		for (i=0; i<4096; i++) c_table[i] = n;
	}
	return 0;
}

static UWORD read_tree_p (struct xdms_data *xdms) {
	UWORD * const pt_table = xdms->u_heavy.pt_table;
	UBYTE * const pt_len = xdms->u_heavy.pt_len;
	const UWORD np = xdms->u_heavy.np;
	UWORD i,n;

	n = GETBITS(5);
	DROPBITS(5);
	if (n>0){
		for (i=0; i<n; i++) {
			pt_len[i] = (UBYTE)GETBITS(4);
			DROPBITS(4);
		}
		for (i=n; i<np; i++) pt_len[i] = 0;
		if (make_table(xdms,np,pt_len,8,pt_table)) return 1;
	} else {
		n = GETBITS(5);
		DROPBITS(5);
		for (i=0; i<np; i++) pt_len[i] = 0;
		for (i=0; i<256; i++) pt_table[i] = n;
	}
	return 0;
}
