/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *     Main decompression functions used in MEDIUM mode
 *
 */

#include "xdms.h"

#define MBITMASK 0x3fff

UWORD Unpack_MEDIUM (struct xdms_data *xdms, UBYTE *in, UBYTE *out, UWORD origsize) {
	UWORD medium_text_loc = xdms->medium_text_loc;
	UBYTE *text = xdms->text;
	UWORD i, j, c;
	UBYTE u, *outend;

	INITBITBUF(in);

	outend = out+origsize;
	while (out < outend) {
		if (GETBITS(1)!=0) {
			DROPBITS(1);
			*out++ = text[medium_text_loc++ & MBITMASK] = (UBYTE)GETBITS(8);
			DROPBITS(8);
		} else {
			DROPBITS(1);
			c = GETBITS(8);  DROPBITS(8);
			j = (UWORD)(d_code[c]+3);
			u = d_len[c];
			c = (UWORD)(((c << u) | GETBITS(u)) & 0xff);  DROPBITS(u);
			u = d_len[c];
			c = (UWORD)((d_code[c] << 8) | (((c << u) | GETBITS(u)) & 0xff));  DROPBITS(u);
			i = (UWORD)(medium_text_loc - c - 1);

			while(j--) *out++ = text[medium_text_loc++ & MBITMASK] = text[i++ & MBITMASK];
			
		}
	}
	xdms->medium_text_loc = (UWORD)((medium_text_loc+66) & MBITMASK);

	return 0;
}
