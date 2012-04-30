#ifndef CLIB_Z_PROTOS_H
#define CLIB_Z_PROTOS_H


/*
**	$VER: z_protos.h 1.0 (23.11.2011)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright © 2011 
**	All Rights Reserved
*/

#ifndef  EXEC_TYPES_H
#include <exec/types.h>
#endif

const char * ZlibVersion(void);
LONG DeflateInit(LONG strm, LONG level);
LONG Deflate(LONG strm, LONG flush);
LONG DeflateEnd(LONG strm);
LONG InflateInit(LONG strm);
LONG Inflate(LONG strm, LONG flush);
LONG InflateEnd(LONG strm);
LONG DeflateInit2(LONG strm, LONG level, LONG method, LONG windowBits, LONG memLevel,
	LONG strategy);
LONG DeflateSetDictionary(LONG strm, LONG dictionary, ULONG dictLength);
LONG DeflateCopy(LONG dest, LONG source);
LONG DeflateReset(LONG strm);
LONG DeflateParams(LONG strm, LONG level, LONG strategy);
LONG InflateInit2(LONG strm, LONG windowBits);
LONG InflateSetDictionary(LONG strm, LONG dictionary, ULONG dictLength);
LONG InflateReset(LONG strm);
LONG Compress(APTR dest, ULONG * destLen, LONG source, ULONG sourceLen);
LONG Uncompress(APTR dest, ULONG * destLen, LONG source, ULONG sourceLen);
ULONG Adler32(ULONG adler, LONG buf, ULONG len);
ULONG CRC32(ULONG crc, LONG buf, ULONG len);
LONG InflateSync(LONG strm);

#endif	/*  CLIB_Z_PROTOS_H  */
