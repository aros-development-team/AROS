/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <dos/dos.h>
#include <exec/execbase.h>
#include "mathffp_intern.h"
#include "libdefs.h"

/* Basename != Libname */
#define Math_functable	Mathffp_functable
#define Math_end	Mathffp_end

#define INIT	AROS_SLIB_ENTRY(init,BASENAME)

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const LIBFUNCTABLE[];
LIBBASETYPEPTR INIT();
extern const char LIBEND;

int MathFFP_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident Mathffp_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Mathffp_resident,
    (APTR)&LIBEND,
    RTF_AUTOINIT|RTF_COLDSTART,
    VERSION_NUMBER,
    NT_LIBRARY,
    -120,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]=MATHFFPNAME;

static const char version[]=VERSION_STRING;

static const APTR inittabl[4]=
{
    (APTR)sizeof(LIBBASETYPE),
    (APTR)LIBFUNCTABLE,
    NULL,
    &INIT
};

AROS_UFH3(LIBBASETYPEPTR, AROS_SLIB_ENTRY(init,Mathffp),
 AROS_UFHA(LIBBASETYPEPTR,	LIBBASE, D0),
 AROS_UFHA(BPTR,		segList, A0),
 AROS_UFHA(struct ExecBase *,	sysBase, A6)
)
{
    AROS_USERFUNC_INIT

    SysBase = sysBase;

    /* You would return NULL if the init failed */
    return LIBBASE;
    AROS_USERFUNC_EXIT
}

AROS_LH1(LIBBASETYPEPTR, open,
 AROS_LHA(ULONG, version, D0),
	   LIBBASETYPEPTR, LIBBASE, 1, Mathffp)
{
    AROS_LIBFUNC_INIT

    /* Keep compiler happy */
    version=0;

    /* I have one more opener. */
    LIBBASE->LibNode.lib_OpenCnt++;
    LIBBASE->LibNode.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
	   LIBBASETYPEPTR, LIBBASE, 2, Mathffp)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    if(!--LIBBASE->LibNode.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(LIBBASE->LibNode.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
	   LIBBASETYPEPTR, LIBBASE, 3, Mathffp)
{
    AROS_LIBFUNC_INIT
#ifndef DISK_BASED
    if (!(LIBBASE->LibNode.lib_OpenCnt) )
    {
	/* Allow the driver to release uneccessary memory */
    }

    /* Don't delete this library. It's in ROM and therefore cannot be
       deleted */
    return 0L;
#else
    BPTR ret;

    /* Test for openers. */
    if (LIBBASE->LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	LIBBASE->LibNode.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&LIBBASE->LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=0L;

    /* Free the memory. */
    FreeMem((char *)LIBBASE-LIBBASE->LibNode.lib_NegSize,
	    LIBBASE->LibNode.lib_NegSize+LIBBASE->LibNode.lib_PosSize);

    return ret;
#endif
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
	    LIBBASETYPEPTR, LIBBASE, 4, Mathffp)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
