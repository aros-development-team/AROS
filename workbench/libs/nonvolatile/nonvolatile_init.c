/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Nonvolatile library initialization code.
    Lang: English
*/

#define  AROS_ALMOST_COMPATIBLE

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <devices/timer.h>
#include <aros/libcall.h>

#include "initstruct.h"
#include <stddef.h>

#include <exec/libraries.h>
#include <exec/alerts.h>
#include "libdefs.h"

#include "nonvolatile_intern.h"

#define INIT	AROS_SLIB_ENTRY(init, Nonvolatile)

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];
extern const struct inittable datatable;
extern struct NVBase *INIT();
extern struct NVBase *AROS_SLIB_ENTRY(open, Nonvolatile)();
extern BPTR AROS_SLIB_ENTRY(close, Nonvolatile)();
extern BPTR AROS_SLIB_ENTRY(expunge, Nonvolatile)();
extern int AROS_SLIB_ENTRY(null, Nonvolatile)();
extern const char LIBEND;

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&LIBEND,
    RTF_AUTOINIT,
    VERSION_NUMBER,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[] = NAME_STRING;

const char version[] = VERSION_STRING;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct NVBase),
    (APTR)LIBFUNCTABLE,
    (APTR)&datatable,
    &INIT
};

struct inittable
{
    S_CPYO(1,1,B);
    S_CPYO(2,1,L);
    S_CPYO(3,1,B);
    S_CPYO(4,1,W);
    S_CPYO(5,1,W);
    S_CPYO(6,1,L);
    S_END (LIBEND);
};

#define O(n) offsetof(struct NVBase, n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(nv_LibNode.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(nv_LibNode.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(nv_LibNode.lib_Flags       )), { LIBF_SUMUSED | LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(nv_LibNode.lib_Version     )), { 1 } } },
    { { I_CPYO(1,W,O(nv_LibNode.lib_Revision    )), { 0 } } },
    { { I_CPYO(1,L,O(nv_LibNode.lib_IdString    )), { (IPTR)&version[6] } } },
	I_END ()
};

#undef O


AROS_LH2(struct NVBase *, init,
 AROS_LHA(struct NVBase *, nvBase, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, Nonvolatile)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    nvBase->nv_SysBase = sysBase;
    nvBase->nv_SegList = segList;

    return nvBase;

    AROS_LIBFUNC_EXIT
}


AROS_LH1(struct NVBase *, open,
 AROS_LHA(ULONG, version, D0),
	   struct NVBase *, nvBase, 1, Nonvolatile)
{
    AROS_LIBFUNC_INIT

    /*
      This function is single-threaded by exec by calling Forbid.
      If you break the Forbid() another task may enter this function
      at the same time. Take care.
    */
    
    /* Keep compiler happy */
    version = 0;
    
    nvBase->nv_LibNode.lib_OpenCnt++;
    nvBase->nv_LibNode.lib_Flags &= ~LIBF_DELEXP;

    if(nvBase->nv_LibNode.lib_OpenCnt == 1)
    {
	D(bug("Opening implementation library (NVDisk)\n"));

	// Should be able to select this one...
	nvBase->nv_ImplementationLib = OpenLibrary("nvdisk.library", 41);

	if(nvBase->nv_ImplementationLib == NULL)
	{
	    // Restore library counter
	    nvBase->nv_LibNode.lib_OpenCnt--;
	    return NULL;
	}
    }

    D(bug("Nonvolatile library successfully opened\n"));

    return nvBase;

    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, close, struct NVBase *, nvBase, 2, Nonvolatile)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */
    --(nvBase->nv_LibNode.lib_OpenCnt);
    
    if((nvBase->nv_LibNode.lib_Flags & LIBF_DELEXP) != 0)
    {
	if(nvBase->nv_LibNode.lib_OpenCnt == 0)
	    return expunge();
	
	nvBase->nv_LibNode.lib_Flags &= ~LIBF_DELEXP;
	return NULL;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, expunge, struct NVBase *, nvBase, 3, Nonvolatile)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    CloseLibrary(nvBase->nv_ImplementationLib);

    /* Test for openers. */
    if(nvBase->nv_LibNode.lib_OpenCnt != 0)
    {
	/* Set the delayed expunge flag and return. */
	nvBase->nv_LibNode.lib_Flags |= LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&nvBase->nv_LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret = nvBase->nv_SegList;

    /* Free the memory. */
    FreeMem((char *)nvBase - nvBase->nv_LibNode.lib_NegSize,
	    nvBase->nv_LibNode.lib_NegSize + nvBase->nv_LibNode.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}


AROS_LH0I(int, null, struct NVBase *, nvBase, 4, Nonvolatile)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
