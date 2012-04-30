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
#include <bzlib.h>
#include "support.h"
#include <SDI_compiler.h>
#include "bz2.library_rev.h"

#define LIBNAME "bz2.library"
const char USED verstag[] = VERSTAG;

#ifndef __AROS__
#define IPTR ULONG
#endif

struct ExecBase *SysBase;

struct BZ2Base {
	struct Library libNode;
	UWORD pad;
	BPTR seglist;
};

int malloc_init(void);
void malloc_exit(void);

#ifdef __AROS__
static AROS_UFP3(struct BZ2Base *, LibInit,
	AROS_UFPA(struct BZ2Base *, libBase, D0),
	AROS_UFPA(BPTR, seglist, A0),
	AROS_UFPA(struct ExecBase *, exec_base, A6)
);
static AROS_LD1(struct BZ2Base *, LibOpen,
	AROS_LPA(ULONG, version, D0),
	struct BZ2Base *, libBase, 1, BZlib
);
static AROS_LD0(BPTR, LibClose,
	struct BZ2Base *, libBase, 2, BZlib
);
static AROS_LD0(BPTR, LibExpunge,
	struct BZ2Base *, libBase, 3, BZlib
);
static AROS_LD0(APTR, LibReserved,
	struct BZ2Base *, libBase, 4, BZlib
);
static AROS_LD0(const char *, BZlibVersion,
	struct BZ2Base *, libBase, 5, BZlib
);
static AROS_LD4(LONG, CompressInit,
	AROS_LPA(bz_stream *, strm, A0),
	AROS_LPA(LONG, blockSize100k, D0),
	AROS_LPA(LONG, verbosity, D1),
	AROS_LPA(LONG, workFactor, D2),
	struct BZ2Base *, libBase, 6, BZlib
);
static AROS_LD2(LONG, Compress,
	AROS_LPA(bz_stream *, strm, A0),
	AROS_LPA(LONG, action, D0),
	struct BZ2Base *, libBase, 7, BZlib
);
static AROS_LD1(LONG, CompressEnd,
	AROS_LPA(bz_stream *, strm, A0),
	struct BZ2Base *, libBase, 8, BZlib
);
static AROS_LD3(LONG, DecompressInit,
	AROS_LPA(bz_stream *, strm, A0),
	AROS_LPA(LONG, verbosity, D0),
	AROS_LPA(LONG, small, D1),
	struct BZ2Base *, libBase, 9, BZlib
);
static AROS_LD1(LONG, Decompress,
	AROS_LPA(bz_stream *, strm, A0),
	struct BZ2Base *, libBase, 10, BZlib
);
static AROS_LD1(LONG, DecompressEnd,
	AROS_LPA(bz_stream *, strm, A0),
	struct BZ2Base *, libBase, 11, BZlib
);
static AROS_LD7(LONG, BuffToBuffCompress,
	AROS_LPA(APTR, dest, A0),
	AROS_LPA(ULONG *, destLen, A1),
	AROS_LPA(APTR, source, A2),
	AROS_LPA(ULONG, sourceLen, D0),
	AROS_LPA(LONG, blockSize100k, D1),
	AROS_LPA(LONG, verbosity, D2),
	AROS_LPA(LONG, workFactor, D3),
	struct BZ2Base *, libBase, 12, BZlib
);
static AROS_LD6(LONG, BuffToBuffDecompress,
	AROS_LPA(APTR, dest, A0),
	AROS_LPA(ULONG *, destLen, A1),
	AROS_LPA(APTR, source, A2),
	AROS_LPA(ULONG, sourceLen, D0),
	AROS_LPA(LONG, small, D1),
	AROS_LPA(LONG, verbosity, D2),
	struct BZ2Base *, libBase, 13, BZlib
);
#else
static struct BZ2Base *LibInit (REG(d0, struct BZ2Base *libBase), REG(a0, BPTR seglist),
	REG(a6, struct ExecBase *exec_base));
static struct BZ2Base *BZlib_LibOpen (REG(a6, struct BZ2Base *libBase), REG(d0, ULONG version));
static BPTR BZlib_LibClose (REG(a6, struct BZ2Base *libBase));
static BPTR BZlib_LibExpunge (REG(a6, struct BZ2Base *libBase));
static APTR BZlib_LibReserved (REG(a6, struct BZ2Base *libBase));
static const char *BZlib_BZlibVersion(void);
static LONG BZlib_CompressInit(REG(a0, bz_stream *strm), REG(d0, LONG blockSize100k),
	REG(d1, LONG verbosity), REG(d2, LONG workFactor));
static LONG BZlib_Compress(REG(a0, bz_stream *strm), REG(d0, LONG action));
static LONG BZlib_CompressEnd(REG(a0, bz_stream *strm));
static LONG BZlib_DecompressInit(REG(a0, bz_stream *strm), REG(d0, LONG verbosity),
	REG(d1, LONG small));
static LONG BZlib_Decompress(REG(a0, bz_stream *strm));
static LONG BZlib_DecompressEnd(REG(a0, bz_stream *strm));
static LONG BZlib_BuffToBuffCompress(REG(a0, APTR dest), REG(a1, ULONG *destLen),
	REG(a2, APTR source), REG(d0, ULONG sourceLen), REG(d1, LONG blockSize100k),
	REG(d2, LONG verbosity), REG(d3, LONG workFactor));
static LONG BZlib_BuffToBuffDecompress(REG(a0, APTR dest), REG(a1, ULONG *destLen),
	REG(a2, APTR source), REG(d0, ULONG sourceLen), REG(d1, LONG small),
	REG(d2, LONG verbosity));
#endif

#ifdef __AROS__
#ifdef ABIV1
#define LIB_ENTRY(a,b) AROS_SLIB_ENTRY(a, BZlib, b)
#else
#define LIB_ENTRY(a,b) AROS_SLIB_ENTRY(a, BZlib)
#endif
#else
#define LIB_ENTRY(a,b) BZlib_##a
#endif

const CONST_APTR LibVectors[] = {
	(APTR)LIB_ENTRY(LibOpen, 1),
	(APTR)LIB_ENTRY(LibClose, 2),
	(APTR)LIB_ENTRY(LibExpunge, 3),
	(APTR)LIB_ENTRY(LibReserved, 4),
	(APTR)LIB_ENTRY(BZlibVersion, 5),
	(APTR)LIB_ENTRY(CompressInit, 6),
	(APTR)LIB_ENTRY(Compress, 7),
	(APTR)LIB_ENTRY(CompressEnd, 8),
	(APTR)LIB_ENTRY(DecompressInit, 9),
	(APTR)LIB_ENTRY(Decompress, 10),
	(APTR)LIB_ENTRY(DecompressEnd, 11),
	(APTR)LIB_ENTRY(BuffToBuffCompress, 12),
	(APTR)LIB_ENTRY(BuffToBuffDecompress, 13),
	(APTR)-1
};

const IPTR LibInitTab[] = {
	sizeof(struct BZ2Base),
	(IPTR)LibVectors,
	(IPTR)NULL,
	(IPTR)LibInit
};

CONST struct Resident USED lib_res = {
	RTC_MATCHWORD,
	(struct Resident *)&lib_res,
	(APTR)(&lib_res + 1),
	RTF_AUTOINIT,
	VERSION,
	NT_LIBRARY,
	0,
	(STRPTR)LIBNAME,
	(STRPTR)VSTRING,
	(APTR)&LibInitTab
};

#ifdef __AROS__
static AROS_UFH3(struct BZ2Base *, LibInit,
	AROS_UFHA(struct BZ2Base *, libBase, D0),
	AROS_UFHA(BPTR, seglist, A0),
	AROS_UFHA(struct ExecBase *, exec_base, A6)
)
{
	AROS_USERFUNC_INIT
#else
static struct BZ2Base *LibInit (REG(d0, struct BZ2Base *libBase), REG(a0, BPTR seglist),
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

	SysBase = exec_base;
	libBase->seglist = seglist;

	if (malloc_init()) {
		return libBase;
	}

	DeleteLibrary((struct Library *)libBase);

	return NULL;
#ifdef __AROS__
	AROS_USERFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(struct BZ2Base *, LibOpen,
	AROS_LHA(ULONG, version, D0),
	struct BZ2Base *, libBase, 1, BZlib
)
{
	AROS_LIBFUNC_INIT
#else
static struct BZ2Base *BZlib_LibOpen (REG(a6, struct BZ2Base *libBase), REG(d0, ULONG version)) {
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
	struct BZ2Base *, libBase, 2, BZlib
)
{
	AROS_LIBFUNC_INIT
#else
static BPTR BZlib_LibClose (REG(a6, struct BZ2Base *libBase)) {
#endif
	libBase->libNode.lib_OpenCnt--;
	return 0;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH0(BPTR, LibExpunge,
	struct BZ2Base *, libBase, 3, BZlib
)
{
	AROS_LIBFUNC_INIT
#else
static BPTR BZlib_LibExpunge (REG(a6, struct BZ2Base *libBase)) {
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
	struct BZ2Base *, libBase, 4, BZlib
)
{
	AROS_LIBFUNC_INIT
	return NULL;
	AROS_LIBFUNC_EXIT
}
#else
static APTR BZlib_LibReserved (REG(a6, struct BZ2Base *libBase)) {
	return NULL;
}
#endif

#ifdef __AROS__
static AROS_LH0(const char *, BZlibVersion,
	struct BZ2Base *, libBase, 5, BZlib
)
{
	AROS_LIBFUNC_INIT
#else
static const char *BZlib_BZlibVersion(void) {
#endif
	return BZ2_bzlibVersion();
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH4(LONG, CompressInit,
	AROS_LHA(bz_stream *, strm, A0),
	AROS_LHA(LONG, blockSize100k, D0),
	AROS_LHA(LONG, verbosity, D1),
	AROS_LHA(LONG, workFactor, D2),
	struct BZ2Base *, libBase, 6, BZlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG BZlib_CompressInit(REG(a0, bz_stream *strm), REG(d0, LONG blockSize100k),
	REG(d1, LONG verbosity), REG(d2, LONG workFactor))
{
#endif
	return BZ2_bzCompressInit(strm, blockSize100k, verbosity, workFactor);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(LONG, Compress,
	AROS_LHA(bz_stream *, strm, A0),
	AROS_LHA(LONG, action, D0),
	struct BZ2Base *, libBase, 7, BZlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG BZlib_Compress(REG(a0, bz_stream *strm), REG(d0, LONG action)) {
#endif
	return BZ2_bzCompress(strm, action);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(LONG, CompressEnd,
	AROS_LHA(bz_stream *, strm, A0),
	struct BZ2Base *, libBase, 8, BZlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG BZlib_CompressEnd(REG(a0, bz_stream *strm)) {
#endif
	return BZ2_bzCompressEnd(strm);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(LONG, DecompressInit,
	AROS_LHA(bz_stream *, strm, A0),
	AROS_LHA(LONG, verbosity, D0),
	AROS_LHA(LONG, small, D1),
	struct BZ2Base *, libBase, 9, BZlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG BZlib_DecompressInit(REG(a0, bz_stream *strm), REG(d0, LONG verbosity),
	REG(d1, LONG small))
{
#endif
	return BZ2_bzDecompressInit(strm, verbosity, small);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(LONG, Decompress,
	AROS_LHA(bz_stream *, strm, A0),
	struct BZ2Base *, libBase, 10, BZlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG BZlib_Decompress(REG(a0, bz_stream *strm)) {
#endif
	return BZ2_bzDecompress(strm);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(LONG, DecompressEnd,
	AROS_LHA(bz_stream *, strm, A0),
	struct BZ2Base *, libBase, 11, BZlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG BZlib_DecompressEnd(REG(a0, bz_stream *strm)) {
#endif
	return BZ2_bzDecompressEnd(strm);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH7(LONG, BuffToBuffCompress,
	AROS_LHA(APTR, dest, A0),
	AROS_LHA(ULONG *, destLen, A1),
	AROS_LHA(APTR, source, A2),
	AROS_LHA(ULONG, sourceLen, D0),
	AROS_LHA(LONG, blockSize100k, D1),
	AROS_LHA(LONG, verbosity, D2),
	AROS_LHA(LONG, workFactor, D3),
	struct BZ2Base *, libBase, 12, BZlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG BZlib_BuffToBuffCompress(REG(a0, APTR dest), REG(a1, ULONG *destLen),
	REG(a2, APTR source), REG(d0, ULONG sourceLen), REG(d1, LONG blockSize100k),
	REG(d2, LONG verbosity), REG(d3, LONG workFactor))
{
#endif
	return BZ2_bzBuffToBuffCompress(dest, (unsigned int *)destLen, source, sourceLen,
		blockSize100k, verbosity, workFactor);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH6(LONG, BuffToBuffDecompress,
	AROS_LHA(APTR, dest, A0),
	AROS_LHA(ULONG *, destLen, A1),
	AROS_LHA(APTR, source, A2),
	AROS_LHA(ULONG, sourceLen, D0),
	AROS_LHA(LONG, small, D1),
	AROS_LHA(LONG, verbosity, D2),
	struct BZ2Base *, libBase, 13, BZlib
)
{
	AROS_LIBFUNC_INIT
#else
static LONG BZlib_BuffToBuffDecompress(REG(a0, APTR dest), REG(a1, ULONG *destLen),
	REG(a2, APTR source), REG(d0, ULONG sourceLen), REG(d1, LONG small),
	REG(d2, LONG verbosity))
{
#endif
	return BZ2_bzBuffToBuffDecompress(dest, (unsigned int *)destLen, source, sourceLen,
		small, verbosity);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

void bz_internal_error (int errcode) {
	Alert(errcode);
}
