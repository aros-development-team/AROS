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

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const Icon_functable[];
extern const struct inittable datatable;
extern struct IconBase *AROS_SLIB_ENTRY(init,Icon)();
extern struct IconBase *AROS_SLIB_ENTRY(open,Icon)();
extern BPTR AROS_SLIB_ENTRY(close,Icon)();
extern BPTR AROS_SLIB_ENTRY(expunge,Icon)();
extern int AROS_SLIB_ENTRY(null,Icon)();
extern const char Icon_end;

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&Icon_end,
    RTF_AUTOINIT,
    1,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]=ICONNAME;

const char version[]="$VER: icon 1.0 (28.3.96)\n\015";

const APTR inittabl[4]=
{
    (APTR)sizeof(struct IconBase),
    (APTR)Icon_functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,Icon)
};

struct inittable
{
    S_CPYO(1,1,B);
    S_CPYO(2,1,L);
    S_CPYO(3,1,B);
    S_CPYO(4,1,W);
    S_CPYO(5,1,W);
    S_CPYO(6,1,L);
    S_END (Icon_end);
};

#define O(n) offsetof(struct IconBase,n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(library.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(library.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(library.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(library.lib_Version     )), { 1 } } },
    { { I_CPYO(1,W,O(library.lib_Revision    )), { 0 } } },
    { { I_CPYO(1,L,O(library.lib_IdString    )), { (IPTR)&version[6] } } },
  I_END ()
};

#undef O
#undef SysBase

AROS_LH2(struct IconBase *, init,
 AROS_LHA(struct IconBase *, IconBase, D0),
 AROS_LHA(BPTR,               segList,   A0),
     struct ExecBase *, SysBase, 0, Icon)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    IconBase->sysbase=SysBase;
    IconBase->seglist=segList;

    /* You would return NULL here if the init failed. */
    return IconBase;
    AROS_LIBFUNC_EXIT
}

/* Use This from now on */
#define SysBase IconBase->sysbase

AROS_LH1(struct IconBase *, open,
 AROS_LHA(ULONG, version, D0),
     struct IconBase *, IconBase, 1, Icon)
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

    /* I have one more opener. */
    IconBase->library.lib_OpenCnt++;
    IconBase->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return IconBase;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct IconBase *, IconBase, 2, Icon)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */
    if(!--IconBase->library.lib_OpenCnt)
    {
	if (DOSBase)
	    CloseLibrary (DOSBase);

	if (UtilityBase)
	    CloseLibrary (UtilityBase);

	/* Delayed expunge pending? */
	if(IconBase->library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct IconBase *, IconBase, 3, Icon)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
  This function is single-threaded by exec by calling Forbid.
  Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(IconBase->library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	IconBase->library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&IconBase->library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=IconBase->seglist;

    /* Free the memory. */
    FreeMem((char *)IconBase-IconBase->library.lib_NegSize,
	IconBase->library.lib_NegSize+IconBase->library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct IconBase *, IconBase, 4, Icon)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

