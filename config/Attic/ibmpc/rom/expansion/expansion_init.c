/*
    Copyright (C) 1996 AROS - The Amiga Replacement OS
    $Id$
    
    Desc: Expansion Resident and initialization.
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/expansion.h>
#include <aros/libcall.h>

#include "expansion_intern.h"
#include "libdefs.h"

static const UBYTE name[];
static const UBYTE version[];
static const APTR inittabl[4];
static void *const FUNCTABLE[];
struct LIBBASETYPE *AROS_SLIB_ENTRY(init,BASENAME)();
extern const char END;

const struct Resident Expansion_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Expansion_resident,
    (APTR)&END,
    RTF_AUTOINIT|RTF_SINGLETASK,
    LIBVERSION,
    NT_LIBRARY,
    110,
    (STRPTR)name,
    (STRPTR)&version[6],
    (ULONG *)inittabl
};

static const UBYTE name[]=LIBNAME;
static const UBYTE version[]=VERSION;

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct IntExpansionBase),
    (APTR)FUNCTABLE,
    NULL,
    &AROS_SLIB_ENTRY(init,BASENAME)
};

AROS_LH2(struct LIBBASETYPE *, init,
    AROS_LHA(struct LIBBASETYPE *,LIBBASE, D0),
    AROS_LHA(BPTR, segList, A0),
    struct ExecBase *, sysBase, 0, Expansion)
{
    AROS_LIBFUNC_INIT
    
    /* Store arguments */
    IntExpBase(LIBBASE)->eb_SegList = (ULONG)segList;
    IntExpBase(LIBBASE)->eb_SysBase = sysBase;
    
    LIBBASE->LibNode.lib_Node.ln_Pri = 0;
    LIBBASE->LibNode.lib_Node.ln_Type = NT_LIBRARY;
    LIBBASE->LibNode.lib_Node.ln_Name = (STRPTR)name;
    LIBBASE->LibNode.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    LIBBASE->LibNode.lib_Version = LIBVERSION;
    LIBBASE->LibNode.lib_Revision = LIBREVISION;
    LIBBASE->LibNode.lib_IdString = (STRPTR)&version[6];
   
    NEWLIST(&LIBBASE->MountList);
 
#if 0
    /* See what expansion hardware we can detect. */
    ConfigChain();
#endif

    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct LIBBASETYPE *, open,
	AROS_LHA(ULONG, version, D0),
	struct LIBBASETYPE *, LIBBASE, 1, BASENAME)
{
	AROS_LIBFUNC_INIT

	/* Keep compiler happy. */
	version = version;

	LIBBASE->LibNode.lib_OpenCnt++;
	LIBBASE->LibNode.lib_Flags &= ~LIBF_DELEXP;

	return LIBBASE;

	AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
	struct LIBBASETYPE *, LIBBASE, 2, BASENAME)
{
	AROS_LIBFUNC_INIT

	/* I have one fewer openers */
	if( !--LIBBASE->LibNode.lib_OpenCnt)
	{

	}
	return 0;

	AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
	struct LIBBASETYPE *, LIBBASE, 3, BASENAME)
{
	AROS_LIBFUNC_INIT

	BPTR ret;

	/* Test for openers */
	if(LIBBASE->LibNode.lib_OpenCnt)
	{
		/* Set delayed expunge and return. */
		LIBBASE->LibNode.lib_Flags |= LIBF_DELEXP;
		return 0;
	}

	ret = 0;

	return ret;

	AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct LIBBASETYPE *, LIBBASE, 4, BASENAME)
{
	AROS_LIBFUNC_INIT
	return 0;
	AROS_LIBFUNC_EXIT
}
