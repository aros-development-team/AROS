/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *
 */

#include "xdms.h"

#define QBITMASK 0xff

UWORD Unpack_QUICK(struct xdms_data *xdms, UBYTE *in, UBYTE *out, UWORD origsize) {
	UWORD quick_text_loc = xdms->quick_text_loc;
	UBYTE *text = xdms->text;
	UWORD i, j;
	UBYTE *outend;

	INITBITBUF(in);

	outend = out+origsize;
	while (out < outend) {
		if (GETBITS(1)!=0) {
			DROPBITS(1);
			*out++ = text[quick_text_loc++ & QBITMASK] = (UBYTE)GETBITS(8);  DROPBITS(8);
		} else {
			DROPBITS(1);
			j = (UWORD)(GETBITS(2)+2);  DROPBITS(2);
			i = (UWORD)(quick_text_loc - GETBITS(8) - 1);  DROPBITS(8);
			while(j--) {
				*out++ = text[quick_text_loc++ & QBITMASK] = text[i++ & QBITMASK];
			}
		}
	}
	xdms->quick_text_loc = (UWORD)((quick_text_loc+5) & QBITMASK);

	return 0;
}
