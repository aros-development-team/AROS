#ifndef _VBCCINLINE_Z_H
#define _VBCCINLINE_Z_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

const char * __ZlibVersion(__reg("a6") struct Library *)="\tjsr\t-30(a6)";
#define ZlibVersion() __ZlibVersion(ZBase)

LONG __DeflateInit(__reg("a6") struct Library *, __reg("a0") void * strm, __reg("d0") LONG level)="\tjsr\t-36(a6)";
#define DeflateInit(strm, level) __DeflateInit(ZBase, (void *)(strm), (level))

LONG __Deflate(__reg("a6") struct Library *, __reg("a0") void * strm, __reg("d0") LONG flush)="\tjsr\t-42(a6)";
#define Deflate(strm, flush) __Deflate(ZBase, (void *)(strm), (flush))

LONG __DeflateEnd(__reg("a6") struct Library *, __reg("a0") void * strm)="\tjsr\t-48(a6)";
#define DeflateEnd(strm) __DeflateEnd(ZBase, (void *)(strm))

LONG __InflateInit(__reg("a6") struct Library *, __reg("a0") void * strm)="\tjsr\t-54(a6)";
#define InflateInit(strm) __InflateInit(ZBase, (void *)(strm))

LONG __Inflate(__reg("a6") struct Library *, __reg("a0") void * strm, __reg("d0") LONG flush)="\tjsr\t-60(a6)";
#define Inflate(strm, flush) __Inflate(ZBase, (void *)(strm), (flush))

LONG __InflateEnd(__reg("a6") struct Library *, __reg("a0") void * strm)="\tjsr\t-66(a6)";
#define InflateEnd(strm) __InflateEnd(ZBase, (void *)(strm))

LONG __DeflateInit2(__reg("a6") struct Library *, __reg("a0") void * strm, __reg("d0") LONG level, __reg("d1") LONG method, __reg("d2") LONG windowBits, __reg("d3") LONG memLevel, __reg("d4") LONG strategy)="\tjsr\t-72(a6)";
#define DeflateInit2(strm, level, method, windowBits, memLevel, strategy) __DeflateInit2(ZBase, (void *)(strm), (level), (method), (windowBits), (memLevel), (strategy))

LONG __DeflateSetDictionary(__reg("a6") struct Library *, __reg("a0") void * strm, __reg("a1") void * dictionary, __reg("d0") ULONG dictLength)="\tjsr\t-78(a6)";
#define DeflateSetDictionary(strm, dictionary, dictLength) __DeflateSetDictionary(ZBase, (void *)(strm), (void *)(dictionary), (dictLength))

LONG __DeflateCopy(__reg("a6") struct Library *, __reg("a0") void * dest, __reg("a1") void * source)="\tjsr\t-84(a6)";
#define DeflateCopy(dest, source) __DeflateCopy(ZBase, (void *)(dest), (void *)(source))

LONG __DeflateReset(__reg("a6") struct Library *, __reg("a0") void * strm)="\tjsr\t-90(a6)";
#define DeflateReset(strm) __DeflateReset(ZBase, (void *)(strm))

LONG __DeflateParams(__reg("a6") struct Library *, __reg("a0") void * strm, __reg("d0") LONG level, __reg("d1") LONG strategy)="\tjsr\t-96(a6)";
#define DeflateParams(strm, level, strategy) __DeflateParams(ZBase, (void *)(strm), (level), (strategy))

LONG __InflateInit2(__reg("a6") struct Library *, __reg("a0") void * strm, __reg("d0") LONG windowBits)="\tjsr\t-102(a6)";
#define InflateInit2(strm, windowBits) __InflateInit2(ZBase, (void *)(strm), (windowBits))

LONG __InflateSetDictionary(__reg("a6") struct Library *, __reg("a0") void * strm, __reg("a1") void * dictionary, __reg("d0") ULONG dictLength)="\tjsr\t-108(a6)";
#define InflateSetDictionary(strm, dictionary, dictLength) __InflateSetDictionary(ZBase, (void *)(strm), (void *)(dictionary), (dictLength))

LONG __InflateReset(__reg("a6") struct Library *, __reg("a0") void * strm)="\tjsr\t-114(a6)";
#define InflateReset(strm) __InflateReset(ZBase, (void *)(strm))

LONG __Compress(__reg("a6") struct Library *, __reg("a0") APTR dest, __reg("a1") ULONG * destLen, __reg("a2") void * source, __reg("d0") ULONG sourceLen)="\tjsr\t-120(a6)";
#define Compress(dest, destLen, source, sourceLen) __Compress(ZBase, (dest), (destLen), (void *)(source), (sourceLen))

LONG __Uncompress(__reg("a6") struct Library *, __reg("a0") APTR dest, __reg("a1") ULONG * destLen, __reg("a2") void * source, __reg("d0") ULONG sourceLen)="\tjsr\t-126(a6)";
#define Uncompress(dest, destLen, source, sourceLen) __Uncompress(ZBase, (dest), (destLen), (void *)(source), (sourceLen))

ULONG __Adler32(__reg("a6") struct Library *, __reg("d0") ULONG adler, __reg("a0") void * buf, __reg("d1") ULONG len)="\tjsr\t-132(a6)";
#define Adler32(adler, buf, len) __Adler32(ZBase, (adler), (void *)(buf), (len))

ULONG __CRC32(__reg("a6") struct Library *, __reg("d0") ULONG crc, __reg("a0") void * buf, __reg("d1") ULONG len)="\tjsr\t-138(a6)";
#define CRC32(crc, buf, len) __CRC32(ZBase, (crc), (void *)(buf), (len))

LONG __InflateSync(__reg("a6") struct Library *, __reg("a0") void * strm)="\tjsr\t-144(a6)";
#define InflateSync(strm) __InflateSync(ZBase, (void *)(strm))

#endif /*  _VBCCINLINE_Z_H  */
