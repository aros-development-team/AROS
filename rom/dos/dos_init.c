/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Header for dos.library
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE /* NEWLIST */

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <utility/tagitem.h>
#include <aros/debug.h>
#include "libdefs.h"
#include "dos_intern.h"

#define INIT	AROS_SLIB_ENTRY(init,Dos)

static const char name[];
static const char version[];
static const APTR Dos_inittabl[4];
static void *const LIBFUNCTABLE[];
struct LIBBASETYPE *INIT ();
extern const char LIBEND;

struct DosLibrary *DOSBase;
struct DosLibrary **dosPtr = &DOSBase;

extern void DOSBoot(struct ExecBase *, struct DosLibrary *);

int Dos_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident Dos_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Dos_resident,
    (APTR)&LIBEND,
    RTF_AUTOINIT,
    VERSION_NUMBER,
    NT_LIBRARY,
    -120,
    (char *)name,
    (char *)&version[6],
    (ULONG *)Dos_inittabl
};

static const char name[]=NAME_STRING;
static const char version[]=VERSION_STRING;

static const APTR Dos_inittabl[4]=
{
    (APTR)sizeof(struct LIBBASETYPE),
    (APTR)LIBFUNCTABLE,
    NULL,
    &INIT
};

static struct RootNode rootnode;

#undef SysBase

AROS_LH2(struct LIBBASETYPE *, init,
 AROS_LHA(struct LIBBASETYPE *, LIBBASE, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, SysBase, 0, BASENAME)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */
    ULONG * taskarray;

    /* Store arguments */
    LIBBASE->dl_SysBase = SysBase;
    LIBBASE->dl_SegList = segList;
    
    LIBBASE->dl_Root = &rootnode;

    /* Init the RootNode structure */
    taskarray = (ULONG *)AllocMem(sizeof(ULONG) + sizeof(APTR), MEMF_CLEAR);
    taskarray[0] = 1;
    rootnode.rn_TaskArray = MKBADDR(taskarray);

    NewList((struct List *)&LIBBASE->dl_Root->rn_CliList);
    InitSemaphore(&LIBBASE->dl_Root->rn_RootLock);

    InitSemaphore(&LIBBASE->dl_DosListLock);

    LIBBASE->dl_UtilityBase = OpenLibrary("utility.library",39L);

    if(LIBBASE->dl_UtilityBase == NULL)
    {
	Alert(AT_DeadEnd | AG_OpenLib | AN_DOSLib | AO_UtilityLib);
	return NULL;
    }

    LIBBASE->dl_IntuitionBase = OpenLibrary("intuition.library", 39L);
    if(LIBBASE->dl_IntuitionBase == NULL)
    {
	Alert(AT_DeadEnd | AG_OpenLib | AN_DOSLib | AO_Intuition);
	return NULL;
    }

    {
	/*  iaint:
	    I know this is bad, but I also know that the timer.device
	    will never go away during the life of dos.library. I also
	    don't intend to make any I/O calls using this.

	    I also know that timer.device does exist in the device list
	    at this point in time.

	    I can't allocate a timerequest/MsgPort pair here anyway,
	    because I need a separate one for each caller to Delay()
	*/

        struct MsgPort timermp;
	
	timermp.mp_Node.ln_Succ = NULL;
	timermp.mp_Node.ln_Pred = NULL;
	timermp.mp_Node.ln_Type = NT_MSGPORT;
	timermp.mp_Node.ln_Pri  = 0;
	timermp.mp_Node.ln_Name = NULL;
	timermp.mp_Flags 	= PA_SIGNAL;
	timermp.mp_SigBit 	= SIGB_SINGLE;
	timermp.mp_SigTask	= FindTask(NULL);
	NEWLIST(&timermp.mp_MsgList);
	
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Succ = NULL;
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Pred = NULL;
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Pri  = 0;
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Node.ln_Name = NULL;
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_ReplyPort    = &timermp;	
	LIBBASE->dl_TimerIO.tr_node.io_Message.mn_Length       = sizeof(struct timerequest);

	SetSignal(0, SIGF_SINGLE);
	
	if(OpenDevice("timer.device", UNIT_VBLANK, 
		      &LIBBASE->dl_TimerIO.tr_node, 0) == 0)
	{
	    LIBBASE->dl_TimerBase = LIBBASE->dl_TimerIO.tr_node.io_Device;

	    *dosPtr = LIBBASE;
	    AddLibrary((struct Library *)LIBBASE);

	    /* This is where we start the RTC_AFTERDOS residents */
	    InitCode(RTF_AFTERDOS, 0);

	    /*
		Here we have to get the first node of the mountlist,
		and we try and boot from it, (assign it to SYS:).
	    */
	    DOSBoot(SysBase, DOSBase);

	    /* We now restart the multitasking	- this is done
	       automatically by RemTask() when it switches.
	    */
	    RemTask(NULL);
	}
	Alert(AT_DeadEnd | AG_OpenDev | AN_DOSLib | AO_TimerDev);
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}

#define SysBase     (LIBBASE->dl_SysBase)

AROS_LH1(struct LIBBASETYPE *, open,
    AROS_LHA(ULONG, version, D0),
    struct LIBBASETYPE *, LIBBASE, 1, BASENAME)
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
    LIBBASE->dl_lib.lib_OpenCnt++;
    LIBBASE->dl_lib.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
	   struct LIBBASETYPE *, LIBBASE, 2, BASENAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */
    if(!--LIBBASE->dl_lib.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(LIBBASE->dl_lib.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
	   struct LIBBASETYPE *, LIBBASE, 3, BASENAME)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(LIBBASE->dl_lib.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	LIBBASE->dl_lib.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&LIBBASE->dl_lib.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=LIBBASE->dl_SegList;

    /* Free the memory. */
    FreeMem((char *)LIBBASE-LIBBASE->dl_lib.lib_NegSize,
	    LIBBASE->dl_lib.lib_NegSize+LIBBASE->dl_lib.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
	    struct LIBBASETYPE *, LIBBASE, 4, BASENAME)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
