/*
    Copyright (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: aros.library Resident and initialization.
    Lang: english
*/

#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>

#include "aros_intern.h"

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

#define INIT AROS_SLIB_ENTRY(init,Aros)

static const UBYTE name[];
static const UBYTE version[];
static const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];
struct LIBBASETYPE *AROS_SLIB_ENTRY(init,Aros)();
extern const char LIBEND;

int Aros_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident Aros_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Aros_resident,
    (APTR)&LIBEND,
    RTF_AUTOINIT|RTF_COLDSTART,
    VERSION_NUMBER,
    NT_LIBRARY,
    102,		/* Immediately after utility.library */
    (STRPTR)name,
    (STRPTR)&version[6],
    (ULONG *)inittabl
};

static const UBYTE name[]=NAME_STRING;

static const UBYTE version[]=VERSION_STRING;

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct ArosBase),
    (APTR)LIBFUNCTABLE,
    NULL,
    &INIT
};

AROS_LH2(struct LIBBASETYPE *, init,
    AROS_LHA(struct LIBBASETYPE *, LIBBASE, D0),
    AROS_LHA(BPTR,               segList,   A0),
    struct ExecBase *, sysBase, 0, BASENAME)
{
    AROS_LIBFUNC_INIT

    /* Store arguments */
    LIBBASE->aros_sysBase=sysBase;
    LIBBASE->aros_segList=segList;

    /* Set up ArosBase */
    LIBBASE->aros_LibNode.lib_Node.ln_Pri = 0;
    LIBBASE->aros_LibNode.lib_Node.ln_Type = NT_LIBRARY;
    LIBBASE->aros_LibNode.lib_Node.ln_Name = (STRPTR)name;
    LIBBASE->aros_LibNode.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    LIBBASE->aros_LibNode.lib_Version = VERSION_NUMBER;
    LIBBASE->aros_LibNode.lib_Revision = REVISION_NUMBER;
    LIBBASE->aros_LibNode.lib_IdString = (STRPTR)&version[6];

    D(bug("aros.library starting...\n"));

    /* You would return NULL if the init failed */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct LIBBASETYPE *, open,
 AROS_LHA(ULONG, version, D0),
	   struct LIBBASETYPE *, LIBBASE, 1, BASENAME)
{
    AROS_LIBFUNC_INIT

    if(!(LIBBASE->aros_utilityBase))
    {
	if(!(LIBBASE->aros_utilityBase = OpenLibrary("utility.library", 37)))
	    return NULL;
    }

    /* I have one more opener. */
    LIBBASE->aros_LibNode.lib_OpenCnt++;
    LIBBASE->aros_LibNode.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
	   struct LIBBASETYPE *, LIBBASE, 2, BASENAME)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    if(!--LIBBASE->aros_LibNode.lib_OpenCnt)
    {
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

/* aros.library must not ever be expunged! */

AROS_LH0(BPTR, expunge,
	   struct LIBBASETYPE *, LIBBASE, 3, BASENAME)
{
    AROS_LIBFUNC_INIT

    /* Test for openers. */
    if(LIBBASE->aros_LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	LIBBASE->aros_LibNode.lib_Flags|=LIBF_DELEXP;
    }

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
