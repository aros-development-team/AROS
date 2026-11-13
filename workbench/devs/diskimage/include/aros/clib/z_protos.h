/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef CLIB_Z_PROTOS_H
#define CLIB_Z_PROTOS_H

/*
**	$VER: z_protos.h 1.0 (14.02.2008)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright © 2001 Amiga, Inc.
**	    All Rights Reserved
*/

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

const char * ZlibVersion(void);
LONG DeflateInit(z_streamp strm, LONG level);
LONG Deflate(z_streamp strm, LONG flush);
LONG DeflateEnd(z_streamp strm);
LONG InflateInit(z_streamp strm);
LONG Inflate(z_streamp strm, LONG flush);
LONG InflateEnd(z_streamp strm);
LONG DeflateInit2(z_streamp strm, LONG level, LONG method, LONG windowBits, LONG memLevel, LONG strategy);
LONG DeflateSetDictionary(z_streamp strm, CONST_APTR dictionary, ULONG dictLength);
LONG DeflateCopy(z_streamp dest, z_streamp source);
LONG DeflateReset(z_streamp strm);
LONG DeflateParams(z_streamp strm, LONG level, LONG strategy);
LONG InflateInit2(z_streamp strm, LONG windowBits);
LONG InflateSetDictionary(z_streamp strm, CONST_APTR dictionary, ULONG dictLength);
LONG InflateReset(z_streamp strm);
LONG Compress(APTR dest, ULONG * destLen, CONST_APTR source, ULONG sourceLen);
LONG Uncompress(APTR dest, ULONG * destLen, CONST_APTR source, ULONG sourceLen);
ULONG Adler32(ULONG adler, CONST_APTR buf, ULONG len);
ULONG CRC32(ULONG crc, CONST_APTR buf, ULONG len);
LONG InflateSync(z_streamp strm);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_Z_PROTOS_H */
