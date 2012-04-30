/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#include <exec/exec.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <zlib.h>
#include "support.h"
#include <SDI_compiler.h>
#include "z.library_rev.h"

#define LIBNAME "z.library"
const char USED_VAR verstag[] = VERSTAG;

#ifndef __AROS__
#define IPTR ULONG
#endif

struct ExecBase *SysBase;

struct ZBase {
	struct Library libNode;
	UWORD pad;
	BPTR seglist;
};

int malloc_init(void);
void malloc_exit(void);

#ifdef __AROS__
static AROS_UFP3(struct ZBase *, LibInit,
	AROS_UFPA(struct ZBase *, libBase, D0),
	AROS_UFPA(BPTR, seglist, A0),
	AROS_UFPA(struct ExecBase *, exec_base, A6)
);
static AROS_LD1(struct ZBase *, LibOpen,
	AROS_LPA(ULONG, version, D0),
	struct ZBase *, libBase, 1, Zlib
);
static AROS_LD0(BPTR, LibClose,
	struct ZBase *, libBase, 2, Zlib
);
static AROS_LD0(BPTR, LibExpunge,
	struct ZBase *, libBase, 3, Zlib
);
static AROS_LD0(APTR, LibReserved,
	struct ZBase *, libBase, 4, Zlib
);
static AROS_LD0(const char *, ZlibVersion,
	struct ZBase *, libBase, 5, Zlib
);
static AROS_LD2(LONG, DeflateInit,
	AROS_LPA(z_streamp, strm, A0),
	AROS_LPA(LONG, level, D0),
	struct ZBase *, libBase, 6, Zlib
);
static AROS_LD2(LONG, Deflate,
	AROS_LPA(z_streamp, strm, A0),
	AROS_LPA(LONG, flush, D0),
	struct ZBase *, libBase, 7, Zlib
);
static AROS_LD1(LONG, DeflateEnd,
	AROS_LPA(z_streamp, strm, A0),
	struct ZBase *, libBase, 8, Zlib
);
static AROS_LD1(LONG, InflateInit,
	AROS_LPA(z_streamp, strm, A0),
	struct ZBase *, libBase, 9, Zlib
);
static AROS_LD2(LONG, Inflate,
	AROS_LPA(z_streamp, strm, A0),
	AROS_LPA(LONG, flush, D0),
	struct ZBase *, libBase, 10, Zlib
);
static AROS_LD1(LONG, InflateEnd,
	AROS_LPA(z_streamp, strm, A0),
	struct ZBase *, libBase, 11, Zlib
);
static AROS_LD6(LONG, DeflateInit2,
	AROS_LPA(z_streamp, strm, A0),
	AROS_LPA(LONG, level, D0),
	AROS_LPA(LONG, method, D1),
	AROS_LPA(LONG, windowBits, D2),
	AROS_LPA(LONG, memLevel, D3),
	AROS_LPA(LONG, strategy, D4),
	struct ZBase *, libBase, 12, Zlib
);
static AROS_LD3(LONG, DeflateSetDictionary,
	AROS_LPA(z_streamp, strm, A0),
	AROS_LPA(APTR, dictionary, A1),
	AROS_LPA(ULONG, dictLength, D0),
	struct ZBase *, libBase, 13, Zlib
);
static AROS_LD2(LONG, DeflateCopy,
	AROS_LPA(z_streamp, dest, A0),
	AROS_LPA(z_streamp, source, A1),
	struct ZBase *, libBase, 14, Zlib
);
static AROS_LD1(LONG, DeflateReset,
	AROS_LPA(z_streamp, strm, A0),
	struct ZBase *, libBase, 15, Zlib
);
static AROS_LD3(LONG, DeflateParams,
	AROS_LPA(z_streamp, strm, A0),
	AROS_LPA(LONG, level, D0),
	AROS_LPA(LONG, strategy, D1),
	struct ZBase *, libBase, 16, Zlib
);
static AROS_LD2(LONG, InflateInit2,
	AROS_LPA(z_streamp, strm, A0),
	AROS_LPA(LONG, windowBits, D0),
	struct ZBase *, libBase, 17, Zlib
);
static AROS_LD3(LONG, InflateSetDictionary,
	AROS_LPA(z_streamp, strm, A0),
	AROS_LPA(APTR, dictionary, A1),
	AROS_LPA(ULONG, dictLength, D0),
	struct ZBase *, libBase, 18, Zlib
);
static AROS_LD1(LONG, InflateReset,
	AROS_LPA(z_streamp, strm, A0),
	struct ZBase *, libBase, 19, Zlib
);
static AROS_LD4(LONG, Compress,
	AROS_LPA(APTR, dest, A0),
	AROS_LPA(ULONG *, destLen, A1),
	AROS_LPA(APTR, source, A2),
	AROS_LPA(ULONG, sourceLen, D0),
	struct ZBase *, libBase, 20, Zlib
);
static AROS_LD4(LONG, Uncompress,
	AROS_LPA(APTR, dest, A0),
	AROS_LPA(ULONG *, destLen, A1),
	AROS_LPA(APTR, source, A2),
	AROS_LPA(ULONG, sourceLen, D0),
	struct ZBase *, libBase, 21, Zlib
);
static AROS_LD3(ULONG, Adler32,
	AROS_LPA(ULONG, adler, D0),
	AROS_LPA(APTR, buf, A0),
	AROS_LPA(ULONG, len, D1),
	struct ZBase *, libBase, 22, Zlib
);
static AROS_LD3(ULONG, CRC32,
	AROS_LPA(ULONG, crc, D0),
	AROS_LPA(APTR, buf, A0),
	AROS_LPA(ULONG, len, D1),
	struct ZBase *, libBase, 23, Zlib
);
static AROS_LD1(LONG, InflateSync,
	AROS_LPA(z_streamp, strm, A0),
	struct ZBase *, libBase, 24, Zlib
);
#else
static struct ZBase *LibInit (REG(d0, struct ZBase *libBase), REG(a0, BPTR seglist),
	REG(a6, struct ExecBase *exec_base));
static struct ZBase *Zlib_LibOpen (REG(a6, struct ZBase *libBase), REG(d0, ULONG version));
static BPTR Zlib_LibClose (REG(a6, struct ZBase *libBase));
static BPTR Zlib_LibExpunge (REG(a6, struct ZBase *libBase));
static APTR Zlib_LibReserved (REG(a6, struct ZBase *libBase));
static const char *Zlib_ZlibVersion(void);
static LONG Zlib_DeflateInit(REG(a0, z_streamp strm), REG(d0, LONG level));
static LONG Zlib_Deflate(REG(a0, z_streamp strm), REG(d0, LONG flush));
static LONG Zlib_DeflateEnd(REG(a0, z_streamp strm));
static LONG Zlib_InflateInit(REG(a0, z_streamp strm));
static LONG Zlib_Inflate(REG(a0, z_streamp strm), REG(d0, LONG flush));
static LONG Zlib_InflateEnd(REG(a0, z_streamp strm));
static LONG Zlib_DeflateInit2(REG(a0, z_streamp strm), REG(d0, LONG level), REG(d1, LONG method),
	REG(d2, LONG windowBits), REG(d3, LONG memLevel), REG(d4, LONG strategy));
static LONG Zlib_DeflateSetDictionary(REG(a0, z_streamp strm), REG(a1, APTR dictionary),
	REG(d0, ULONG dictLength));
static LONG Zlib_DeflateCopy(REG(a0, z_streamp dest), REG(a1, z_streamp source));
static LONG Zlib_DeflateReset(REG(a0, z_streamp strm));
static LONG Zlib_DeflateParams(REG(a0, z_streamp strm), REG(d0, LONG level), REG(d1, LONG strategy));
static LONG Zlib_InflateInit2(REG(a0, z_streamp strm), REG(d0, LONG windowBits));
static LONG Zlib_InflateSetDictionary(REG(a0, z_streamp strm), REG(a1, APTR dictionary),
	REG(d0, ULONG dictLength));
static LONG Zlib_InflateReset(REG(a0, z_streamp strm));
static LONG Zlib_Compress(REG(a0, APTR dest), REG(a1, ULONG *destLen), REG(a2, APTR source),
	REG(d0, ULONG sourceLen));
static LONG Zlib_Uncompress(REG(a0, APTR dest), REG(a1, ULONG *destLen), REG(a2, APTR source),
	REG(d0, ULONG sourceLen));
static ULONG Zlib_Adler32(REG(d0, ULONG adler), REG(a0, APTR buf), REG(d1, ULONG len));
static ULONG Zlib_CRC32(REG(d0, ULONG crc), REG(a0, APTR buf), REG(d1, ULONG len));
static LONG Zlib_InflateSync(REG(a0, z_streamp strm));
#endif

#ifdef __AROS__
#ifdef __PPC__
#define LIB_ENTRY(a,b) AROS_SLIB_ENTRY(a, Zlib, b)
#else
#define LIB_ENTRY(a,b) AROS_SLIB_ENTRY(a, Zlib)
#endif
#else
#define LIB_ENTRY(a,b) Zlib_##a
#endif

CONST_APTR LibVectors[] = {
	(APTR)LIB_ENTRY(LibOpen, 1),
	(APTR)LIB_ENTRY(LibClose, 2),
	(APTR)LIB_ENTRY(LibExpunge, 3),
	(APTR)LIB_ENTRY(LibReserved, 4),
	(APTR)LIB_ENTRY(ZlibVersion, 5),
	(APTR)LIB_ENTRY(DeflateInit, 6),
	(APTR)LIB_ENTRY(Deflate, 7),
	(APTR)LIB_ENTRY(DeflateEnd, 8),
	(APTR)LIB_ENTRY(InflateInit, 9),
	(APTR)LIB_ENTRY(Inflate, 10),
	(APTR)LIB_ENTRY(InflateEnd, 11),
	(APTR)LIB_ENTRY(DeflateInit2, 12),
	(APTR)LIB_ENTRY(DeflateSetDictionary, 13),
	(APTR)LIB_ENTRY(DeflateCopy, 14),
	(APTR)LIB_ENTRY(DeflateReset, 15),
	(APTR)LIB_ENTRY(DeflateParams, 16),
	(APTR)LIB_ENTRY(InflateInit2, 17),
	(APTR)LIB_ENTRY(InflateSetDictionary, 18),
	(APTR)LIB_ENTRY(InflateReset, 19),
	(APTR)LIB_ENTRY(Compress, 20),
	(APTR)LIB_ENTRY(Uncompress, 21),
	(APTR)LIB_ENTRY(Adler32, 22),
	(APTR)LIB_ENTRY(CRC32, 23),
	(APTR)LIB_ENTRY(InflateSync, 24),
	(APTR)-1
};

const IPTR LibInitTab[] = {
	sizeof(struct ZBase),
	(IPTR)LibVectors,
	(IPTR)NULL,
	(IPTR)LibInit
};

const struct Resident USED_VAR ROMTag = {
	RTC_MATCHWORD,
	(struct Resident *)&ROMTag,
	(APTR)(&ROMTag + 1),
	RTF_AUTOINIT,
	VERSION,
	NT_LIBRARY,
	0,
	(STRPTR)LIBNAME,
	(STRPTR)VSTRING,
	(APTR)LibInitTab
};

#ifdef __AROS__
static AROS_UFH3(struct ZBase *, LibInit,
	AROS_UFHA(struct ZBase *, libBase, D0),
	AROS_UFHA(BPTR, seglist, A0),
	AROS_UFHA(struct ExecBase *, exec_base, A6)
)
{
	AROS_USERFUNC_INIT
#else
static struct ZBase *LibInit (REG(d0, struct ZBase *libBase), REG(a0, BPTR seglist),
	REG(a6, struct ExecBase *exec_base))
{
#endif
	libBase->libNode.lib_Node.ln_Type = NT_LIBRARY;
	libBase->libNode.lib_Node.ln_Pri  = 0;
	libBase->libNode.lib_Node.ln_Name = LIBNAME;
	libBase->libNode.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
	libBase->libNode.lib_Version      = VERSION;
	libBase->libNode.lib_Revision     = REVISION;
	libBase->libNode.lib_IdString     = VSTRING;

	libBase->seglist = seglist;
	SysBase = exec_base;

	if (malloc_init()) {
		get_crc_table();
		return libBase;
	}

	DeleteLibrary((struct Library *)libBase);

	return NULL;
#ifdef __AROS__
	AROS_USERFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(struct ZBase *, LibOpen,
	AROS_LHA(ULONG, version, D0),
	struct ZBase *, libBase, 1, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static struct ZBase *Zlib_LibOpen (REG(a6, struct ZBase *libBase), REG(d0, ULONG version)) {
#endif
	libBase->libNode.lib_OpenCnt++;
	libBase->libNode.lib_Flags &= ~LIBF_DELEXP;
	return libBase;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH0(BPTR, LibClose,
	struct ZBase *, libBase, 2, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static BPTR Zlib_LibClose (REG(a6, struct ZBase *libBase)) {
#endif
	libBase->libNode.lib_OpenCnt--;
	return 0;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH0(BPTR, LibExpunge,
	struct ZBase *, libBase, 3, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static BPTR Zlib_LibExpunge (REG(a6, struct ZBase *libBase)) {
#endif
	BPTR result = 0;

	if (libBase->libNode.lib_OpenCnt > 0) {
		libBase->libNode.lib_Flags |= LIBF_DELEXP;
		return 0;
	}

	Remove(&libBase->libNode.lib_Node);

	result = libBase->seglist;

	malloc_exit();

	DeleteLibrary((struct Library *)libBase);

	return result;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH0(APTR, LibReserved,
	struct ZBase *, libBase, 4, Zlib
)
{
	AROS_LIBFUNC_INIT
	return NULL;
	AROS_LIBFUNC_EXIT
}
#else
static APTR Zlib_LibReserved (REG(a6, struct ZBase *libBase)) {
	return NULL;
}
#endif

#ifdef __AROS__
static AROS_LH0(const char *, ZlibVersion,
	struct ZBase *, libBase, 5, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static const char *Zlib_ZlibVersion(void) {
#endif
	return zlibVersion();
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(LONG, DeflateInit,
	AROS_LHA(z_streamp, strm, A0),
	AROS_LHA(LONG, level, D0),
	struct ZBase *, libBase, 6, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_DeflateInit(REG(a0, z_streamp strm), REG(d0, LONG level)) {
#endif
	return deflateInit(strm, level);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(LONG, Deflate,
	AROS_LHA(z_streamp, strm, A0),
	AROS_LHA(LONG, flush, D0),
	struct ZBase *, libBase, 7, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_Deflate(REG(a0, z_streamp strm), REG(d0, LONG flush)) {
#endif
	return deflate(strm, flush);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(LONG, DeflateEnd,
	AROS_LHA(z_streamp, strm, A0),
	struct ZBase *, libBase, 8, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_DeflateEnd(REG(a0, z_streamp strm)) {
#endif
	return deflateEnd(strm);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(LONG, InflateInit,
	AROS_LHA(z_streamp, strm, A0),
	struct ZBase *, libBase, 9, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_InflateInit(REG(a0, z_streamp strm)) {
#endif
	return inflateInit(strm);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(LONG, Inflate,
	AROS_LHA(z_streamp, strm, A0),
	AROS_LHA(LONG, flush, D0),
	struct ZBase *, libBase, 10, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_Inflate(REG(a0, z_streamp strm), REG(d0, LONG flush)) {
#endif
	return inflate(strm, flush);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(LONG, InflateEnd,
	AROS_LHA(z_streamp, strm, A0),
	struct ZBase *, libBase, 11, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_InflateEnd(REG(a0, z_streamp strm)) {
#endif
	return inflateEnd(strm);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH6(LONG, DeflateInit2,
	AROS_LHA(z_streamp, strm, A0),
	AROS_LHA(LONG, level, D0),
	AROS_LHA(LONG, method, D1),
	AROS_LHA(LONG, windowBits, D2),
	AROS_LHA(LONG, memLevel, D3),
	AROS_LHA(LONG, strategy, D4),
	struct ZBase *, libBase, 12, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_DeflateInit2(REG(a0, z_streamp strm), REG(d0, LONG level), REG(d1, LONG method),
	REG(d2, LONG windowBits), REG(d3, LONG memLevel), REG(d4, LONG strategy))
{
#endif
	return deflateInit2(strm, level, method, windowBits, memLevel, strategy);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(LONG, DeflateSetDictionary,
	AROS_LHA(z_streamp, strm, A0),
	AROS_LHA(APTR, dictionary, A1),
	AROS_LHA(ULONG, dictLength, D0),
	struct ZBase *, libBase, 13, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_DeflateSetDictionary(REG(a0, z_streamp strm), REG(a1, APTR dictionary),
	REG(d0, ULONG dictLength))
{
#endif
	return deflateSetDictionary(strm, dictionary, dictLength);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(LONG, DeflateCopy,
	AROS_LHA(z_streamp, dest, A0),
	AROS_LHA(z_streamp, source, A1),
	struct ZBase *, libBase, 14, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_DeflateCopy(REG(a0, z_streamp dest), REG(a1, z_streamp source)) {
#endif
	return deflateCopy(dest, source);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(LONG, DeflateReset,
	AROS_LHA(z_streamp, strm, A0),
	struct ZBase *, libBase, 15, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_DeflateReset(REG(a0, z_streamp strm)) {
#endif
	return deflateReset(strm);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(LONG, DeflateParams,
	AROS_LHA(z_streamp, strm, A0),
	AROS_LHA(LONG, level, D0),
	AROS_LHA(LONG, strategy, D1),
	struct ZBase *, libBase, 16, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_DeflateParams(REG(a0, z_streamp strm), REG(d0, LONG level), REG(d1, LONG strategy)) {
#endif
	return deflateParams(strm, level, strategy);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(LONG, InflateInit2,
	AROS_LHA(z_streamp, strm, A0),
	AROS_LHA(LONG, windowBits, D0),
	struct ZBase *, libBase, 17, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_InflateInit2(REG(a0, z_streamp strm), REG(d0, LONG windowBits)) {
#endif
	return inflateInit2(strm, windowBits);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(LONG, InflateSetDictionary,
	AROS_LHA(z_streamp, strm, A0),
	AROS_LHA(APTR, dictionary, A1),
	AROS_LHA(ULONG, dictLength, D0),
	struct ZBase *, libBase, 18, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_InflateSetDictionary(REG(a0, z_streamp strm), REG(a1, APTR dictionary),
	REG(d0, ULONG dictLength))
{
#endif
	return inflateSetDictionary(strm, dictionary, dictLength);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(LONG, InflateReset,
	AROS_LHA(z_streamp, strm, A0),
	struct ZBase *, libBase, 19, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_InflateReset(REG(a0, z_streamp strm)) {
#endif
	return inflateReset(strm);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH4(LONG, Compress,
	AROS_LHA(APTR, dest, A0),
	AROS_LHA(ULONG *, destLen, A1),
	AROS_LHA(APTR, source, A2),
	AROS_LHA(ULONG, sourceLen, D0),
	struct ZBase *, libBase, 20, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_Compress(REG(a0, APTR dest), REG(a1, ULONG *destLen), REG(a2, APTR source),
	REG(d0, ULONG sourceLen))
{
#endif
	return compress(dest, destLen, source, sourceLen);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH4(LONG, Uncompress,
	AROS_LHA(APTR, dest, A0),
	AROS_LHA(ULONG *, destLen, A1),
	AROS_LHA(APTR, source, A2),
	AROS_LHA(ULONG, sourceLen, D0),
	struct ZBase *, libBase, 21, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_Uncompress(REG(a0, APTR dest), REG(a1, ULONG *destLen), REG(a2, APTR source),
	REG(d0, ULONG sourceLen))
{
#endif
	return uncompress(dest, destLen, source, sourceLen);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(ULONG, Adler32,
	AROS_LHA(ULONG, adler, D0),
	AROS_LHA(APTR, buf, A0),
	AROS_LHA(ULONG, len, D1),
	struct ZBase *, libBase, 22, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static ULONG Zlib_Adler32(REG(d0, ULONG adler), REG(a0, APTR buf), REG(d1, ULONG len)) {
#endif
	return adler32(adler, buf, len);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(ULONG, CRC32,
	AROS_LHA(ULONG, crc, D0),
	AROS_LHA(APTR, buf, A0),
	AROS_LHA(ULONG, len, D1),
	struct ZBase *, libBase, 23, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static ULONG Zlib_CRC32(REG(d0, ULONG crc), REG(a0, APTR buf), REG(d1, ULONG len)) {
#endif
	return crc32(crc, buf, len);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(LONG, InflateSync,
	AROS_LHA(z_streamp, strm, A0),
	struct ZBase *, libBase, 24, Zlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG Zlib_InflateSync(REG(a0, z_streamp strm)) {
#endif
	return inflateSync(strm);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}
