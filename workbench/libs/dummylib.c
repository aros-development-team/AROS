/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang:
*/
#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#ifdef __GNUC__
#   include "dummylib_gcc.h"
#endif
#include "initstruct.h"
#include <stddef.h>

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const functable[];
extern const struct inittable datatable;
extern struct dummybase *AROS_SLIB_ENTRY(init,dummy)();
extern struct dummybase *AROS_SLIB_ENTRY(open,dummy)();
extern BPTR AROS_SLIB_ENTRY(close,dummy)();
extern BPTR AROS_SLIB_ENTRY(expunge,dummy)();
extern int AROS_SLIB_ENTRY(null,dummy)();
extern ULONG AROS_SLIB_ENTRY(add,dummy)();
extern ULONG AROS_SLIB_ENTRY(asl,dummy)();
extern const char end;

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&end,
    RTF_AUTOINIT,
    1,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]="dummy.library";

const char version[]="$VER: dummylib 41.1 (28.3.96)\n\015";

const APTR inittabl[4]=
{
    (APTR)sizeof(struct dummybase),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,dummy)
};

void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,dummy),
    &AROS_SLIB_ENTRY(close,dummy),
    &AROS_SLIB_ENTRY(expunge,dummy),
    &AROS_SLIB_ENTRY(null,dummy),
    &AROS_SLIB_ENTRY(add,dummy),
    &AROS_SLIB_ENTRY(asl,dummy),
    (void *)-1
};

struct inittable
{
    S_CPYO(1,1,B);
    S_CPYO(2,1,L);
    S_CPYO(3,1,B);
    S_CPYO(4,1,W);
    S_CPYO(5,1,W);
    S_CPYO(6,1,L);
    S_END (end);
};

#define O(n) offsetof(struct dummybase,n)

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

AROS_LH2(struct dummybase *, init,
 AROS_LHA(struct dummybase *, dummybase, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, SysBase, 0, dummy)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    dummybase->sysbase=SysBase;
    dummybase->seglist=segList;

    /* You would return NULL here if the init failed. */
    return dummybase;
    AROS_LIBFUNC_EXIT
}

/* Use This from now on */
#ifdef SysBase
#undef SysBase
#endif
#define SysBase dummybase->sysbase

AROS_LH1(struct dummybase *, open,
 AROS_LHA(ULONG, version, D0),
	   struct dummybase *, dummybase, 1, dummy)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Keep compiler happy */
    version=0;

    /* I have one more opener. */
    dummybase->library.lib_OpenCnt++;
    dummybase->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return dummybase;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct dummybase *, dummybase, 2, dummy)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */
    if(!--dummybase->library.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(dummybase->library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct dummybase *, dummybase, 3, dummy)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(dummybase->library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	dummybase->library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&dummybase->library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=dummybase->seglist;

    /* Free the memory. */
    FreeMem((char *)dummybase-dummybase->library.lib_NegSize,
	    dummybase->library.lib_NegSize+dummybase->library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}
AROS_LH0I(int, null, struct dummybase *, dummybase, 4, dummy)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH2I(ULONG, add,
    AROS_LHA(ULONG,a,D0),
    AROS_LHA(ULONG,b,D1),
    struct dummybase *,dummybase,5,dummy)
{
    AROS_LIBFUNC_INIT
    return a+b;
    AROS_LIBFUNC_EXIT
}

AROS_LH2I(ULONG, asl,
    AROS_LHA(ULONG,a,D0),
    AROS_LHA(ULONG,b,D1),
    struct dummybase *,dummybase,6,dummy)
{
    AROS_LIBFUNC_INIT
    return a<<b;
    AROS_LIBFUNC_EXIT
}

const char end=0;
