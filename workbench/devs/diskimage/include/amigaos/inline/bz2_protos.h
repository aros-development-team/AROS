#ifndef _VBCCINLINE_BZ2_H
#define _VBCCINLINE_BZ2_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

const char * __BZ2_bzlibVersion(__reg("a6") struct Library *)="\tjsr\t-30(a6)";
#define BZ2_bzlibVersion() __BZ2_bzlibVersion(BZ2Base)

LONG __BZ2_bzCompressInit(__reg("a6") struct Library *, __reg("a0") bz_stream * strm, __reg("d0") LONG blockSize100k, __reg("d1") LONG verbosity, __reg("d2") LONG workFactor)="\tjsr\t-36(a6)";
#define BZ2_bzCompressInit(strm, blockSize100k, verbosity, workFactor) __BZ2_bzCompressInit(BZ2Base, (strm), (blockSize100k), (verbosity), (workFactor))

LONG __BZ2_bzCompress(__reg("a6") struct Library *, __reg("a0") bz_stream * strm, __reg("d0") LONG action)="\tjsr\t-42(a6)";
#define BZ2_bzCompress(strm, action) __BZ2_bzCompress(BZ2Base, (strm), (action))

LONG __BZ2_bzCompressEnd(__reg("a6") struct Library *, __reg("a0") bz_stream * strm)="\tjsr\t-48(a6)";
#define BZ2_bzCompressEnd(strm) __BZ2_bzCompressEnd(BZ2Base, (strm))

LONG __BZ2_bzDecompressInit(__reg("a6") struct Library *, __reg("a0") bz_stream * strm, __reg("d0") LONG verbosity, __reg("d1") LONG small)="\tjsr\t-54(a6)";
#define BZ2_bzDecompressInit(strm, verbosity, small) __BZ2_bzDecompressInit(BZ2Base, (strm), (verbosity), (small))

LONG __BZ2_bzDecompress(__reg("a6") struct Library *, __reg("a0") bz_stream * strm)="\tjsr\t-60(a6)";
#define BZ2_bzDecompress(strm) __BZ2_bzDecompress(BZ2Base, (strm))

LONG __BZ2_bzDecompressEnd(__reg("a6") struct Library *, __reg("a0") bz_stream * strm)="\tjsr\t-66(a6)";
#define BZ2_bzDecompressEnd(strm) __BZ2_bzDecompressEnd(BZ2Base, (strm))

LONG __BZ2_bzBuffToBuffCompress(__reg("a6") struct Library *, __reg("a0") APTR dest, __reg("a1") ULONG * destLen, __reg("a2") void * source, __reg("d0") ULONG sourceLen, __reg("d1") LONG blockSize100k, __reg("d2") LONG verbosity, __reg("d3") LONG workFactor)="\tjsr\t-72(a6)";
#define BZ2_bzBuffToBuffCompress(dest, destLen, source, sourceLen, blockSize100k, verbosity, workFactor) __BZ2_bzBuffToBuffCompress(BZ2Base, (dest), (destLen), (void *)(source), (sourceLen), (blockSize100k), (verbosity), (workFactor))

LONG __BZ2_bzBuffToBuffDecompress(__reg("a6") struct Library *, __reg("a0") APTR dest, __reg("a1") ULONG * destLen, __reg("a2") void * source, __reg("d0") ULONG sourceLen, __reg("d1") LONG small, __reg("d2") LONG verbosity)="\tjsr\t-78(a6)";
#define BZ2_bzBuffToBuffDecompress(dest, destLen, source, sourceLen, small, verbosity) __BZ2_bzBuffToBuffDecompress(BZ2Base, (dest), (destLen), (void *)(source), (sourceLen), (small), (verbosity))

#endif /*  _VBCCINLINE_BZ2_H  */
