/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#define DEBUG 0
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dosasl.h>
#include <dos/doshunks.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/arossupport.h>
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/macros.h>

#include <loadseg/loadseg.h>

#include "dos_intern.h"

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(__mc68000)
static AROS_UFH4(LONG, ReadFunc,
	AROS_UFHA(BPTR, file,   D1),
	AROS_UFHA(APTR, buffer, D2),
	AROS_UFHA(LONG, length, D3),
        AROS_UFHA(struct DosLibrary *, DOSBase, A6)
)
{
    AROS_USERFUNC_INIT

    return FRead(file, buffer, 1, length);

    AROS_USERFUNC_EXIT
}

static AROS_UFH3(APTR, AllocFunc,
	AROS_UFHA(ULONG, length, D0),
	AROS_UFHA(ULONG, flags,  D1),
        AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    return AllocMem(length, flags);

    AROS_USERFUNC_EXIT
}

static AROS_UFH3(void, FreeFunc,
	AROS_UFHA(APTR, buffer, A1),
	AROS_UFHA(ULONG, length, D0),
        AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    FreeMem(buffer, length);

    AROS_USERFUNC_EXIT
}

AROS_UFH4(BPTR, LoadSeg_Overlay,
    AROS_UFHA(UBYTE*, name, D1),
    AROS_UFHA(BPTR, hunktable, D2),
    AROS_UFHA(BPTR, fh, D3),
    AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT

    void (*FunctionArray[3])();
    ULONG hunktype;
    BPTR seg;
    SIPTR error = RETURN_OK;

    FunctionArray[0] = (APTR)ReadFunc;
    FunctionArray[1] = (APTR)AllocFunc;
    FunctionArray[2] = (APTR)FreeFunc;

    D(bug("LoadSeg_Overlay. table=%x fh=%x\n", hunktable, fh));
    if (AROS_UFC4(LONG, ReadFunc,
            AROS_UFCA(BPTR,   fh,               D1),
            AROS_UFCA(void *, &hunktype,        D2),
            AROS_UFCA(LONG,   sizeof(hunktype), D3),
            AROS_UFCA(struct Library *, DOSBase,A6)) != sizeof(hunktype))
    	return BNULL;
    hunktype = AROS_BE2LONG(hunktype);
    if (hunktype != HUNK_HEADER)
    	return BNULL;
    seg = LoadSegment(fh, hunktable, (SIPTR*)FunctionArray, NULL, &error, (struct Library *)DOSBase);
    if (seg == BNULL)
        SetIoErr(error);

    return seg;

    AROS_USERFUNC_EXIT
}

#endif
