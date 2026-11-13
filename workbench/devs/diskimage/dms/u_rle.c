/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *     Run Length Encoding decompression function used in most
 *     modes after decompression by other algorithm
 *
 */

#include "xdms.h"

UWORD Unpack_RLE (struct xdms_data *xdms, UBYTE *in, UBYTE *out, UWORD origsize) {
	UWORD n;
	UBYTE a,b, *outend;

	outend = out+origsize;
	while (out<outend){
		if ((a = *in++) != 0x90)
			*out++ = a;
		else if (!(b = *in++))
			*out++ = a;
		else {
			a = *in++;
			if (b == 0xff) {
				n = *in++;
				n = (UWORD)((n<<8) + *in++);
			} else
				n = b;
			if (out+n > outend) return 1;
			memset(out,a,(size_t) n);
			out += n;
		}
	}
	return 0;
}
