/*
    (C) 1999 AROS - The Amiga Replacement OS
    $Id:

    Desc: Realtime.library initialization code.
    Lang: English.
*/

/* HISTORY:      25.06.99   SDuvan  Began implementing... */


#define  AROS_ALMOST_COMPATIBLE

#include <aros/debug.h>

#include <utility/utility.h>

#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/libcall.h>

#include "initstruct.h"
#include <stddef.h>

#include <exec/libraries.h>

#include "realtime_intern.h"
#include "libdefs.h"

#define INIT	AROS_SLIB_ENTRY(init, RealTime)

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];
extern const struct inittable datatable;
extern struct internal_RealTimeBase *INIT();
extern struct RealTimeBase *AROS_SLIB_ENTRY(open, RealTime)();
extern BPTR AROS_SLIB_ENTRY(close, RealTime)();
extern BPTR AROS_SLIB_ENTRY(expunge, RealTime)();
extern int AROS_SLIB_ENTRY(null, RealTime)();
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

const APTR inittabl[4] =
{
    (APTR)sizeof(struct RealTimeBase),
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

#define O(n) offsetof(struct RealTimeBase, n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(rtb_LibNode.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(rtb_LibNode.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(rtb_LibNode.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(rtb_LibNode.lib_Version     )), { 1 } } },
    { { I_CPYO(1,W,O(rtb_LibNode.lib_Revision    )), { 0 } } },
    { { I_CPYO(1,L,O(rtb_LibNode.lib_IdString    )), { (IPTR)&version[6] } } },
	I_END ()
};

#undef O


AROS_LH2(struct internal_RealTimeBase *, init,
 AROS_LHA(struct internal_RealTimeBase *, RTBase, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, RealTime)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    WORD   i;			/* Loop variable */

    /* Store arguments */
    RTBase->rtb_SysBase = sysBase;
    RTBase->rtb_SegList = segList;

    for(i = 0; i < RT_MAXLOCK; i++)
	InitSemaphore(&RTBase->rtb_Locks[i]);

    NEWLIST(&RTBase->rtb_ConductorList);

    return RTBase;

    AROS_LIBFUNC_EXIT
}


AROS_LH1(struct RealTimeBase *, open,
 AROS_LHA(ULONG, version, D0),
	   struct RealTimeBase *, RTBase, 1, RealTime)
{
    AROS_LIBFUNC_INIT

    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Keep compiler happy */
    version = 0;

    if(GPB(RTBase)->rtb_UtilityBase == NULL)
	GPB(RTBase)->rtb_UtilityBase = OpenLibrary("utility.library", 41);

    if(GPB(RTBase)->rtb_UtilityBase == NULL)
	return NULL;

    /* I have one more opener. */
    RTBase->rtb_LibNode.lib_OpenCnt++;
    RTBase->rtb_LibNode.lib_Flags &= ~LIBF_DELEXP;

    return RTBase;

    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, close, struct RealTimeBase *, RTBase, 2, RealTime)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */

    if(--(RTBase->rtb_LibNode.lib_OpenCnt) == 0)
	return expunge();
    else
	RTBase->rtb_LibNode.lib_Flags &= ~LIBF_DELEXP;

    return NULL;

    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, expunge, struct internal_RealTimeBase *, RTBase, 3, RealTime)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    CloseLibrary(GPB(RTBase)->rtb_UtilityBase);

    /* Test for openers. */
    if(RTBase->rtb_LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	RTBase->rtb_LibNode.lib_Flags |= LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&RTBase->rtb_LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret = RTBase->rtb_SegList;

    /* Free the memory. */
    FreeMem((char *)RTBase-RTBase->rtb_LibNode.lib_NegSize,
	    RTBase->rtb_LibNode.lib_NegSize + RTBase->rtb_LibNode.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}


AROS_LH0I(int, null, struct RealTimeBase *, RTBase, 4, RealTime)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
