/*
    Copyright (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: layers.library Resident and initialization.
    Lang: english
*/

#include <exec/types.h>
#include <exec/resident.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include "layers_intern.h"

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

extern const UBYTE name[];
extern const UBYTE version[];
extern const APTR inittabl[4];
extern void *const FUNCTABLE[];
struct LIBBASETYPE *INIT();
extern const char END;

int Layers_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident Layers_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Layers_resident,
    (APTR)&END,
    RTF_AUTOINIT|RTF_COLDSTART,
    LIBVERSION,
    NT_LIBRARY,
    64,		/* priority */
    (STRPTR)name,
    (STRPTR)&version[6],
    (ULONG *)inittabl
};

const UBYTE name[]=LIBNAME;

const UBYTE version[]=VERSION;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct LIBBASETYPE),
    (APTR)FUNCTABLE,
    NULL,
    &INIT
};

AROS_LH2(struct LIBBASETYPE *, init,
    AROS_LHA(struct LIBBASETYPE *, LIBBASE, D0),
    AROS_LHA(BPTR,                 segList, A0),
    struct ExecBase *, sysBase, 0, BASENAME)
{
    AROS_LIBFUNC_INIT

    /* Store arguments */
    LIBBASE->lb_SysBase = sysBase;

    /* Set up ArosBase */
    LIBBASE->lb_LibNode.lib_Node.ln_Pri = 0;
    LIBBASE->lb_LibNode.lib_Node.ln_Type = NT_LIBRARY;
    (const)LIBBASE->lb_LibNode.lib_Node.ln_Name = name;
    LIBBASE->lb_LibNode.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    LIBBASE->lb_LibNode.lib_Version = LIBVERSION;
    LIBBASE->lb_LibNode.lib_Revision = LIBREVISION;
    (const)LIBBASE->lb_LibNode.lib_IdString = &version[6];

    D(bug("layers.library starting...\n"));

    /* You would return NULL if the init failed */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct LIBBASETYPE *, open,
 AROS_LHA(ULONG, version, D0),
	   struct LIBBASETYPE *, LIBBASE, 1, BASENAME)
{
    AROS_LIBFUNC_INIT

    /* I have one more opener. */
    LIBBASE->lb_LibNode.lib_OpenCnt++;
    LIBBASE->lb_LibNode.lib_Flags&=~LIBF_DELEXP;

    if(!GfxBase)
    {
	if(!(GfxBase = OpenLibrary(GRAPHICSNAME, 37)))
	    return NULL;
    }

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
	   struct LIBBASETYPE *, LIBBASE, 2, BASENAME)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    if(!--LIBBASE->lb_LibNode.lib_OpenCnt)
    {
	if(GfxBase)
	    CloseLibrary(GfxBase);

	/* Delayed expunge pending? */
	if(LIBBASE->lb_LibNode.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
	   struct LIBBASETYPE *, LIBBASE, 3, BASENAME)
{
    AROS_LIBFUNC_INIT

    /* Test for openers. */
    if(LIBBASE->lb_LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	LIBBASE->lb_LibNode.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&LIBBASE->lb_LibNode.lib_Node);

    /* Free the memory. */
    FreeMem((char *)LIBBASE-LIBBASE->lb_LibNode.lib_NegSize,
	LIBBASE->lb_LibNode.lib_NegSize+LIBBASE->lb_LibNode.lib_PosSize);

    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
	    struct LIBBASETYPE *, LIBBASE, 4, BASENAME)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
