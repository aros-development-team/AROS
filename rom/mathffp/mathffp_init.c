/* This file is derived from graphics_init with
** all unnecessary stuff erased
*/
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Mathffp library
    Lang: english
*/
#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/libcall.h>
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
static void *const FUNCTABLE[];
struct LIBBASETYPE * INIT();
extern const char END;

extern int  driver_init (struct LIBBASETYPE *);
extern int  driver_open (struct LIBBASETYPE *);
extern void driver_close (struct LIBBASETYPE *);
extern void driver_expunge (struct LIBBASETYPE *);

int MathFFP_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident Mathffp_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Mathffp_resident,
    (APTR)&END,
    RTF_AUTOINIT,
    39,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]=MATHFFPNAME;

static const char version[]=VERSION;

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct LIBBASETYPE),
    (APTR)FUNCTABLE,
    NULL,
    &INIT
};

AROS_LH2(struct LIBBASETYPE *, init,
 AROS_LHA(struct LIBBASETYPE *, LIBBASE, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, BASENAME)
{
    AROS_LIBFUNC_INIT

    SysBase = sysBase;

    Disable();
    if (!driver_init (LIBBASE))
    {
	Enable();
	return NULL;
    }
    Enable();

    /* You would return NULL if the init failed */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct LIBBASETYPE *, open,
 AROS_LHA(ULONG, version, D0),
	   struct LIBBASETYPE *, LIBBASE, 1, Mathffp)
{
    AROS_LIBFUNC_INIT

    /* Keep compiler happy */
    version=0;

    Disable();
    if (!driver_open (LIBBASE))
    {
	Enable();
	return NULL;
    }
    Enable();

    /* I have one more opener. */
    LIBBASE->LibNode.lib_OpenCnt++;
    LIBBASE->LibNode.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
	   struct LIBBASETYPE *, LIBBASE, 2, Mathffp)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    if(!--LIBBASE->LibNode.lib_OpenCnt)
    {
	driver_close (LIBBASE);

	/* Delayed expunge pending? */
	if(LIBBASE->LibNode.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
	   struct LIBBASETYPE *, LIBBASE, 3, Mathffp)
{
    AROS_LIBFUNC_INIT
#ifndef DISK_BASED
    if (!(LIBBASE->LibNode.lib_OpenCnt) )
    {
	/* Allow the driver to release uneccessary memory */
	driver_expunge (LIBBASE);
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
	    struct LIBBASETYPE *, LIBBASE, 4, Mathffp)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
