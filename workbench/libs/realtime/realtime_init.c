/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Realtime.library initialization code.
    Lang: English.
*/

/* HISTORY:      25.06.99   SDuvan  Began implementing... */


#define  AROS_ALMOST_COMPATIBLE

#include <utility/utility.h>

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

#include "initstruct.h"
#include <stddef.h>

#include <exec/libraries.h>

#include "realtime_intern.h"
#include "libdefs.h"

#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

#define INIT	AROS_SLIB_ENTRY(init, RealTime)

BOOL AllocTimer(struct internal_RealTimeBase *RealTimeBase);
void FreeTimer(struct internal_RealTimeBase *RealTimeBase);

extern void Pulse();

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
    (APTR)sizeof(struct internal_RealTimeBase),
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
 AROS_LHA(struct internal_RealTimeBase *, RealTimeBase, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, RealTime)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    WORD   i;			/* Loop variable */

    /* Store arguments */
    RealTimeBase->rtb_SysBase = sysBase;
    RealTimeBase->rtb_SegList = segList;

    for(i = 0; i < RT_MAXLOCK; i++)
    {
	InitSemaphore(&RealTimeBase->rtb_Locks[i]);
    }

    NEWLIST(&RealTimeBase->rtb_ConductorList);

    RealTimeBase->rtb_TickErr = 0;	/* How may such a thing be measured? */

    return RealTimeBase;

    AROS_LIBFUNC_EXIT
}


AROS_LH1(struct RealTimeBase *, open,
 AROS_LHA(ULONG, version, D0),
	   struct RealTimeBase *, RealTimeBase, 1, RealTime)
{
    AROS_LIBFUNC_INIT

    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Keep compiler happy */
    version = 0;

    if (GPB(RealTimeBase)->rtb_UtilityBase == NULL)
    {
	GPB(RealTimeBase)->rtb_UtilityBase = OpenLibrary("utility.library", 41);
    }

    if (GPB(RealTimeBase)->rtb_UtilityBase == NULL)
    {
	return NULL;
    }

    if (GPB(RealTimeBase)->rtb_DOSBase == NULL)
    {
	GPB(RealTimeBase)->rtb_DOSBase = OpenLibrary("dos.library", 41);
    }

    if (GPB(RealTimeBase)->rtb_DOSBase == NULL)
    {
	return NULL;
    }

    if (RealTimeBase->rtb_LibNode.lib_OpenCnt == 0)
    {
	/* I use a process here just to be able to use CreateNewProc() so
	   I don't have to fiddle with stack order and such... */
	struct TagItem tags[] = { { NP_Entry   , (IPTR)Pulse            },
				  { NP_Name    , (IPTR)"RealTime Pulse" },
				  { NP_Priority, (IPTR)127              },
				  { NP_UserData, (IPTR)RealTimeBase           },
				  { TAG_DONE   , (IPTR)NULL             } };
	
	GPB(RealTimeBase)->rtb_PulseTask = (struct Task *)CreateNewProc(tags);

	if (GPB(RealTimeBase)->rtb_PulseTask == NULL)
	{
	    return NULL;
	}

	kprintf("Realtime pulse task created\n");

	if (!AllocTimer((struct internal_RealTimeBase *)RealTimeBase))
	{
	    return NULL;
	}
	
	kprintf("Realtime pulse timer created\n");
    }

    /* I have one more opener. */
    RealTimeBase->rtb_LibNode.lib_OpenCnt++;
    RealTimeBase->rtb_LibNode.lib_Flags &= ~LIBF_DELEXP;

    return RealTimeBase;

    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, close, struct RealTimeBase *, RealTimeBase, 2, RealTime)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */

    if(--(RealTimeBase->rtb_LibNode.lib_OpenCnt) == 0)
    {
	return expunge();
    }
    else
    {
	RealTimeBase->rtb_LibNode.lib_Flags &= ~LIBF_DELEXP;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, expunge, struct internal_RealTimeBase *, RealTimeBase, 3, RealTime)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    CloseLibrary(GPB(RealTimeBase)->rtb_UtilityBase);

    /* Test for openers. */
    if (RealTimeBase->rtb_LibNode.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	RealTimeBase->rtb_LibNode.lib_Flags |= LIBF_DELEXP;
	return 0;
    }

    FreeTimer(RealTimeBase);

    /* Shut down the pulse message task -- must be done AFTER freeing the
       timer! */
    Signal(RealTimeBase->rtb_PulseTask, SIGBREAKF_CTRL_C);

    /* Get rid of the library. Remove it from the list. */
    Remove(&RealTimeBase->rtb_LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret = RealTimeBase->rtb_SegList;

    /* Free the memory. */
    FreeMem((char *)RealTimeBase-RealTimeBase->rtb_LibNode.lib_NegSize,
	    RealTimeBase->rtb_LibNode.lib_NegSize + RealTimeBase->rtb_LibNode.lib_PosSize);

    return ret;

    AROS_LIBFUNC_EXIT
}


AROS_LH0I(int, null, struct RealTimeBase *, RealTimeBase, 4, RealTime)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}


/* RealTime timer interrupt -- currently only a VBlank interrupt */
AROS_UFH4(ULONG, rtVBlank,
	  AROS_UFHA(ULONG, dummy, A0),
	  AROS_UFHA(void *, data, A1),
	  AROS_UFHA(ULONG, dummy2, A5),
	  AROS_UFHA(struct ExecBase *, mySysBase, A6))
{
    AROS_USERFUNC_INIT
    struct internal_RealTimeBase *RealTimeBase = GPB(data);

    // kprintf("Signalling task %p\n", RealTimeBase->rtb_PulseTask);
    Signal(RealTimeBase->rtb_PulseTask, SIGF_SINGLE);

    return 0;

    AROS_USERFUNC_EXIT
}


BOOL AllocTimer(struct internal_RealTimeBase *RealTimeBase)
{
    /* TODO */
    /* This should be replaced by some timer.device thing when an accurate
       timer is available -- UNIT_MICROHZ? */

    RealTimeBase->rtb_VBlank.is_Code         = (APTR)&rtVBlank;
    RealTimeBase->rtb_VBlank.is_Data         = (APTR)RealTimeBase;
    RealTimeBase->rtb_VBlank.is_Node.ln_Name = "RealTime VBlank server";
    RealTimeBase->rtb_VBlank.is_Node.ln_Pri  = 127;
    RealTimeBase->rtb_VBlank.is_Node.ln_Type = NT_INTERRUPT;
    
    /* Add a VBLANK server to take care of the heartbeats. */
    AddIntServer(INTB_VERTB, &RealTimeBase->rtb_VBlank);

    return TRUE;
}


void FreeTimer(struct internal_RealTimeBase *RealTimeBase)
{
    RemIntServer(INTB_VERTB, &RealTimeBase->rtb_VBlank);
}
