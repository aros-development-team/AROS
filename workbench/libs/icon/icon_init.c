/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Init of icon.library
    Lang: english
*/
#include <stddef.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#include <workbench/icon.h>
#include <utility/utility.h>
#include "initstruct.h"
#include "icon_intern.h"
#include "libdefs.h"

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void * const FUNCTABLE[];
extern const struct inittable datatable;
extern struct LIBBASETYPE * INIT();
extern const char END;

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&END,
    RTF_AUTOINIT,
    LIBVERSION,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]=LIBNAME;

const char version[]=VERSION;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct LIBBASETYPE),
    (APTR)FUNCTABLE,
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
    S_END (END);
};

#define O(n) offsetof(struct LIBBASETYPE,n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(library.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(library.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(library.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(library.lib_Version     )), { LIBVERSION } } },
    { { I_CPYO(1,W,O(library.lib_Revision    )), { 0 } } },
    { { I_CPYO(1,L,O(library.lib_IdString    )), { (IPTR)&version[6] } } },
  I_END ()
};

#undef O

struct ExecBase * SysBase; /* global variable */

AROS_LH2(struct LIBBASETYPE *, init,
 AROS_LHA(struct LIBBASETYPE *, LIBBASE, D0),
 AROS_LHA(BPTR,               segList,   A0),
     struct ExecBase *, sysBase, 0, _LIBnAME)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    SysBase=sysBase;
    LIBBASE->seglist=segList;

    /* You would return NULL here if the init failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct LIBBASETYPE *, open,
 AROS_LHA(ULONG, version, D0),
     struct LIBBASETYPE *, LIBBASE, 1, _LIBnAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Keep compiler happy */
    version=0;

    if (!DOSBase)
	DOSBase = OpenLibrary (DOSNAME, 39);

    if (!DOSBase)
	return NULL;

    if (!UtilityBase)
	UtilityBase = OpenLibrary (UTILITYNAME, 39);

    if (!UtilityBase)
	return NULL;

    LIBBASE->dsh.h_Entry = (void *)dosstreamhook;
    LIBBASE->dsh.h_Data = DOSBase;

    /* I have one more opener. */
    LIBBASE->library.lib_OpenCnt++;
    LIBBASE->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct LIBBASETYPE *, LIBBASE, 2, _LIBnAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */
    if(!--LIBBASE->library.lib_OpenCnt)
    {
	if (DOSBase)
	    CloseLibrary (DOSBase);

	if (UtilityBase)
	    CloseLibrary (UtilityBase);

	/* Delayed expunge pending? */
	if(LIBBASE->library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct LIBBASETYPE *, LIBBASE, 3, _LIBnAME)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
  This function is single-threaded by exec by calling Forbid.
  Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(LIBBASE->library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	LIBBASE->library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&LIBBASE->library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=LIBBASE->seglist;

    /* Free the memory. */
    FreeMem((char *)LIBBASE-LIBBASE->library.lib_NegSize,
	LIBBASE->library.lib_NegSize+LIBBASE->library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct LIBBASETYPE *, LIBBASE, 4, _LIBnAME)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

