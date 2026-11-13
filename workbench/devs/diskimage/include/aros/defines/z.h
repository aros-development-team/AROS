/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef _INLINE_Z_H
#define _INLINE_Z_H

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

#ifndef Z_BASE_NAME
#define Z_BASE_NAME ZBase
#endif /* !Z_BASE_NAME */

#define ZlibVersion() \
	AROS_LC0(const char *, ZlibVersion, \
	struct Library *, Z_BASE_NAME, 5, Z)

#define DeflateInit(___strm, ___level) \
	AROS_LC2(LONG, DeflateInit, \
	AROS_LCA(z_streamp, (___strm), A0), \
	AROS_LCA(LONG, (___level), D0), \
	struct Library *, Z_BASE_NAME, 6, Z)

#define Deflate(___strm, ___flush) \
	AROS_LC2(LONG, Deflate, \
	AROS_LCA(z_streamp, (___strm), A0), \
	AROS_LCA(LONG, (___flush), D0), \
	struct Library *, Z_BASE_NAME, 7, Z)

#define DeflateEnd(___strm) \
	AROS_LC1(LONG, DeflateEnd, \
	AROS_LCA(z_streamp, (___strm), A0), \
	struct Library *, Z_BASE_NAME, 8, Z)

#define InflateInit(___strm) \
	AROS_LC1(LONG, InflateInit, \
	AROS_LCA(z_streamp, (___strm), A0), \
	struct Library *, Z_BASE_NAME, 9, Z)

#define Inflate(___strm, ___flush) \
	AROS_LC2(LONG, Inflate, \
	AROS_LCA(z_streamp, (___strm), A0), \
	AROS_LCA(LONG, (___flush), D0), \
	struct Library *, Z_BASE_NAME, 10, Z)

#define InflateEnd(___strm) \
	AROS_LC1(LONG, InflateEnd, \
	AROS_LCA(z_streamp, (___strm), A0), \
	struct Library *, Z_BASE_NAME, 11, Z)

#define DeflateInit2(___strm, ___level, ___method, ___windowBits, ___memLevel, ___strategy) \
	AROS_LC6(LONG, DeflateInit2, \
	AROS_LCA(z_streamp, (___strm), A0), \
	AROS_LCA(LONG, (___level), D0), \
	AROS_LCA(LONG, (___method), D1), \
	AROS_LCA(LONG, (___windowBits), D2), \
	AROS_LCA(LONG, (___memLevel), D3), \
	AROS_LCA(LONG, (___strategy), D4), \
	struct Library *, Z_BASE_NAME, 12, Z)

#define DeflateSetDictionary(___strm, ___dictionary, ___dictLength) \
	AROS_LC3(LONG, DeflateSetDictionary, \
	AROS_LCA(z_streamp, (___strm), A0), \
	AROS_LCA(CONST_APTR, (___dictionary), A1), \
	AROS_LCA(ULONG, (___dictLength), D0), \
	struct Library *, Z_BASE_NAME, 13, Z)

#define DeflateCopy(___dest, ___source) \
	AROS_LC2(LONG, DeflateCopy, \
	AROS_LCA(z_streamp, (___dest), A0), \
	AROS_LCA(z_streamp, (___source), A1), \
	struct Library *, Z_BASE_NAME, 14, Z)

#define DeflateReset(___strm) \
	AROS_LC1(LONG, DeflateReset, \
	AROS_LCA(z_streamp, (___strm), A0), \
	struct Library *, Z_BASE_NAME, 15, Z)

#define DeflateParams(___strm, ___level, ___strategy) \
	AROS_LC3(LONG, DeflateParams, \
	AROS_LCA(z_streamp, (___strm), A0), \
	AROS_LCA(LONG, (___level), D0), \
	AROS_LCA(LONG, (___strategy), D1), \
	struct Library *, Z_BASE_NAME, 16, Z)

#define InflateInit2(___strm, ___windowBits) \
	AROS_LC2(LONG, InflateInit2, \
	AROS_LCA(z_streamp, (___strm), A0), \
	AROS_LCA(LONG, (___windowBits), D0), \
	struct Library *, Z_BASE_NAME, 17, Z)

#define InflateSetDictionary(___strm, ___dictionary, ___dictLength) \
	AROS_LC3(LONG, InflateSetDictionary, \
	AROS_LCA(z_streamp, (___strm), A0), \
	AROS_LCA(CONST_APTR, (___dictionary), A1), \
	AROS_LCA(ULONG, (___dictLength), D0), \
	struct Library *, Z_BASE_NAME, 18, Z)

#define InflateReset(___strm) \
	AROS_LC1(LONG, InflateReset, \
	AROS_LCA(z_streamp, (___strm), A0), \
	struct Library *, Z_BASE_NAME, 19, Z)

#define Compress(___dest, ___destLen, ___source, ___sourceLen) \
	AROS_LC4(LONG, Compress, \
	AROS_LCA(APTR, (___dest), A0), \
	AROS_LCA(ULONG *, (___destLen), A1), \
	AROS_LCA(CONST_APTR, (___source), A2), \
	AROS_LCA(ULONG, (___sourceLen), D0), \
	struct Library *, Z_BASE_NAME, 20, Z)

#define Uncompress(___dest, ___destLen, ___source, ___sourceLen) \
	AROS_LC4(LONG, Uncompress, \
	AROS_LCA(APTR, (___dest), A0), \
	AROS_LCA(ULONG *, (___destLen), A1), \
	AROS_LCA(CONST_APTR, (___source), A2), \
	AROS_LCA(ULONG, (___sourceLen), D0), \
	struct Library *, Z_BASE_NAME, 21, Z)

#define Adler32(___adler, ___buf, ___len) \
	AROS_LC3(ULONG, Adler32, \
	AROS_LCA(ULONG, (___adler), D0), \
	AROS_LCA(CONST_APTR, (___buf), A0), \
	AROS_LCA(ULONG, (___len), D1), \
	struct Library *, Z_BASE_NAME, 22, Z)

#define CRC32(___crc, ___buf, ___len) \
	AROS_LC3(ULONG, CRC32, \
	AROS_LCA(ULONG, (___crc), D0), \
	AROS_LCA(CONST_APTR, (___buf), A0), \
	AROS_LCA(ULONG, (___len), D1), \
	struct Library *, Z_BASE_NAME, 23, Z)

#define InflateSync(___strm) \
	AROS_LC1(LONG, InflateSync, \
	AROS_LCA(z_streamp, (___strm), A0), \
	struct Library *, Z_BASE_NAME, 24, Z)

#endif /* !_INLINE_Z_H */
