/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/13 15:35:11  digulla
    Replaced __AROS_LA by __AROS_LHA

    Revision 1.2  1996/08/01 17:41:28  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/types.h>
#include <exec/resident.h>
#include <clib/exec_protos.h>
#include <aros/libcall.h>
#ifdef __GNUC__
    #include "dummylib_gcc.h"
#endif
#include "initstruct.h"

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const functable[];
extern const struct inittable datatable;
extern struct dummybase *dummy_init();
extern struct dummybase *dummy_open();
extern BPTR dummy_close();
extern BPTR dummy_expunge();
extern int dummy_null();
extern ULONG dummy_add();
extern ULONG dummy_asl();
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

const char version[]="$VER: dummy 1.0 (28.3.96)\n\015";

const APTR inittabl[4]=
{
    (APTR)sizeof(struct dummybase),
    (APTR)functable,
    (APTR)&datatable,
    &dummy_init
};

void *const functable[]=
{
    &dummy_open,
    &dummy_close,
    &dummy_expunge,
    &dummy_null,
    &dummy_add,
    &dummy_asl,
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
    { { I_CPYO(1,L,O(library.lib_Node.ln_Name)), { (LONG)name } } },
    { { I_CPYO(1,B,O(library.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(library.lib_Version     )), { 1 } } },
    { { I_CPYO(1,W,O(library.lib_Revision    )), { 0 } } },
    { { I_CPYO(1,L,O(library.lib_IdString    )), { (LONG)&version[6] } } },
	I_END ()
};

#undef O

__AROS_LH2(struct dummybase *, init,
 __AROS_LHA(struct dummybase *, dummybase, D0),
 __AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, SysBase, 0, dummy)
{
    __AROS_FUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    dummybase->sysbase=SysBase;
    dummybase->seglist=segList;

    /* You would return NULL here if the init failed. */
    return dummybase;
    __AROS_FUNC_EXIT
}

/* Use This from now on */
#ifdef SysBase
#undef SysBase
#endif
#define SysBase dummybase->sysbase

__AROS_LH1(struct dummybase *, open,
 __AROS_LHA(ULONG, version, D0),
	   struct dummybase *, dummybase, 1, dummy)
{
    __AROS_FUNC_INIT
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
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, close, struct dummybase *, dummybase, 2, dummy)
{
    __AROS_FUNC_INIT
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
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, expunge, struct dummybase *, dummybase, 3, dummy)
{
    __AROS_FUNC_INIT

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
    __AROS_FUNC_EXIT
}
__AROS_LH0I(int, null, struct dummybase *, dummybase, 4, dummy)
{
    __AROS_FUNC_INIT
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH2I(ULONG, add,
    __AROS_LHA(ULONG,a,D0),
    __AROS_LHA(ULONG,b,D1),
    struct dummybase *,dummybase,5,dummy)
{
    __AROS_FUNC_INIT
    return a+b;
    __AROS_FUNC_EXIT
}

__AROS_LH2I(ULONG, asl,
    __AROS_LHA(ULONG,a,D0),
    __AROS_LHA(ULONG,b,D1),
    struct dummybase *,dummybase,6,dummy)
{
    __AROS_FUNC_INIT
    return a<<b;
    __AROS_FUNC_EXIT
}

const char end=0;
