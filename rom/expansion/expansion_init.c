/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Expansion Resident and initialization.
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include "expansion_intern.h"
#include "libdefs.h"

static const UBYTE name[];
static const UBYTE version[];
static const APTR inittabl[4];
static void *const LIBFUNCTABLE[];
LIBBASETYPEPTR AROS_SLIB_ENTRY(init,BASENAME)();
extern const char LIBEND;

int Expansion_entry()
{
    return -1;
}

const struct Resident Expansion_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Expansion_resident,
    (APTR)&LIBEND,
    RTF_AUTOINIT|RTF_SINGLETASK,
    VERSION_NUMBER,
    NT_TYPE,
    110,
    (STRPTR)name,
    (STRPTR)&version[6],
    (ULONG *)inittabl
};

static const UBYTE name[]=NAME_STRING;
static const UBYTE version[]=VERSION_STRING;

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct IntExpansionBase),
    (APTR)LIBFUNCTABLE,
    NULL,
    &AROS_SLIB_ENTRY(init,BASENAME)
};

AROS_UFH3(LIBBASETYPEPTR, AROS_SLIB_ENTRY(init,Expansion),
    AROS_UFHA(LIBBASETYPEPTR, LIBBASE, D0),
    AROS_UFHA(BPTR, segList, A0),
    AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
    AROS_USERFUNC_INIT

    /* Store arguments */
    IntExpBase(LIBBASE)->eb_SegList = (ULONG)segList;
    IntExpBase(LIBBASE)->eb_SysBase = sysBase;

    LIBBASE->LibNode.lib_Node.ln_Pri = 0;
    LIBBASE->LibNode.lib_Node.ln_Type = NT_TYPE;
    LIBBASE->LibNode.lib_Node.ln_Name = (STRPTR)name;
    LIBBASE->LibNode.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    LIBBASE->LibNode.lib_Version = VERSION_NUMBER;
    LIBBASE->LibNode.lib_Revision = REVISION_NUMBER;
    LIBBASE->LibNode.lib_IdString = (STRPTR)&version[6];

    NEWLIST(&LIBBASE->MountList);

#if 0
    /* See what expansion hardware we can detect. */
    ConfigChain();
#endif

    return LIBBASE;
    AROS_USERFUNC_EXIT
}

AROS_LH1(LIBBASETYPEPTR, open,
	AROS_LHA(ULONG, version, D0),
	LIBBASETYPEPTR, LIBBASE, 1, BASENAME)
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
	LIBBASETYPEPTR, LIBBASE, 2, BASENAME)
{
	AROS_LIBFUNC_INIT

	/* I have one fewer openers */
	if( !--LIBBASE->LibNode.lib_OpenCnt)
	{
#ifdef DISK_BASED
		/* Delayed expunge pending? */
		if(LIBBASE->LibNode.lib_Flags & LIBF_DELEXP)
			return expunge();
#endif
	}
	return 0;

	AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
	LIBBASETYPEPTR, LIBBASE, 3, BASENAME)
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

#ifdef DISK_BASED
	/* Get rid of the library. Remove from the library list. */
	Remove(&LIBBASE->LibNode);

	/* Save returncode here, FreeMem() will destroy the field. */
	ret = IntExpBase(LIBBASE)->eb_SegList;

	/* Free the memory */
	FreeMem((char*)LIBBASE - LIBBASE->LibNode.lib_NegSize,
		LIBBASE->LibNode.lib_NegSize + LIBBASE->LibNode.lib_PosSize);
#else
	ret = 0;
#endif
	return ret;

	AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, LIBBASETYPEPTR, LIBBASE, 4, BASENAME)
{
	AROS_LIBFUNC_INIT
	return 0;
	AROS_LIBFUNC_EXIT
}
