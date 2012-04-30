/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef _INLINE_BZ2_H
#define _INLINE_BZ2_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef BZ2_BASE_NAME
#define BZ2_BASE_NAME BZ2Base
#endif /* !BZ2_BASE_NAME */

#define BZ2_bzlibVersion() \
	AROS_LC0(const char *, BZ2_bzlibVersion, \
	struct Library *, BZ2_BASE_NAME, 5, Bz2)

#define BZ2_bzCompressInit(___strm, ___blockSize100k, ___verbosity, ___workFactor) \
	AROS_LC4(LONG, BZ2_bzCompressInit, \
	AROS_LCA(bz_stream *, (___strm), A0), \
	AROS_LCA(LONG, (___blockSize100k), D0), \
	AROS_LCA(LONG, (___verbosity), D1), \
	AROS_LCA(LONG, (___workFactor), D2), \
	struct Library *, BZ2_BASE_NAME, 6, Bz2)

#define BZ2_bzCompress(___strm, ___action) \
	AROS_LC2(LONG, BZ2_bzCompress, \
	AROS_LCA(bz_stream *, (___strm), A0), \
	AROS_LCA(LONG, (___action), D0), \
	struct Library *, BZ2_BASE_NAME, 7, Bz2)

#define BZ2_bzCompressEnd(___strm) \
	AROS_LC1(LONG, BZ2_bzCompressEnd, \
	AROS_LCA(bz_stream *, (___strm), A0), \
	struct Library *, BZ2_BASE_NAME, 8, Bz2)

#define BZ2_bzDecompressInit(___strm, ___verbosity, ___small) \
	AROS_LC3(LONG, BZ2_bzDecompressInit, \
	AROS_LCA(bz_stream *, (___strm), A0), \
	AROS_LCA(LONG, (___verbosity), D0), \
	AROS_LCA(LONG, (___small), D1), \
	struct Library *, BZ2_BASE_NAME, 9, Bz2)

#define BZ2_bzDecompress(___strm) \
	AROS_LC1(LONG, BZ2_bzDecompress, \
	AROS_LCA(bz_stream *, (___strm), A0), \
	struct Library *, BZ2_BASE_NAME, 10, Bz2)

#define BZ2_bzDecompressEnd(___strm) \
	AROS_LC1(LONG, BZ2_bzDecompressEnd, \
	AROS_LCA(bz_stream *, (___strm), A0), \
	struct Library *, BZ2_BASE_NAME, 11, Bz2)

#define BZ2_bzBuffToBuffCompress(___dest, ___destLen, ___source, ___sourceLen, ___blockSize100k, ___verbosity, ___workFactor) \
	AROS_LC7(LONG, BZ2_bzBuffToBuffCompress, \
	AROS_LCA(APTR, (___dest), A0), \
	AROS_LCA(ULONG *, (___destLen), A1), \
	AROS_LCA(CONST_APTR, (___source), A2), \
	AROS_LCA(ULONG, (___sourceLen), D0), \
	AROS_LCA(LONG, (___blockSize100k), D1), \
	AROS_LCA(LONG, (___verbosity), D2), \
	AROS_LCA(LONG, (___workFactor), D3), \
	struct Library *, BZ2_BASE_NAME, 12, Bz2)

#define BZ2_bzBuffToBuffDecompress(___dest, ___destLen, ___source, ___sourceLen, ___small, ___verbosity) \
	AROS_LC6(LONG, BZ2_bzBuffToBuffDecompress, \
	AROS_LCA(APTR, (___dest), A0), \
	AROS_LCA(ULONG *, (___destLen), A1), \
	AROS_LCA(CONST_APTR, (___source), A2), \
	AROS_LCA(ULONG, (___sourceLen), D0), \
	AROS_LCA(LONG, (___small), D1), \
	AROS_LCA(LONG, (___verbosity), D2), \
	struct Library *, BZ2_BASE_NAME, 13, Bz2)

#endif /* !_INLINE_BZ2_H */
