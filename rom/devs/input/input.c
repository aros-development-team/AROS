/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Input device
    Lang: english
*/

/****************************************************************************************/

#include <exec/resident.h>
#include <exec/interrupts.h>
#include <exec/initializers.h>
#include <devices/inputevent.h>
#include <devices/input.h>
#include <devices/newstyle.h>
#include <proto/exec.h>
#include <proto/input.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#    include "input_intern.h"
#endif

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

#define NEWSTYLE_DEVICE 1

/****************************************************************************************/

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable;

struct inputbase *AROS_SLIB_ENTRY(init,Input)();
void AROS_SLIB_ENTRY(open,Input)();
BPTR AROS_SLIB_ENTRY(close,Input)();
BPTR AROS_SLIB_ENTRY(expunge,Input)();
int AROS_SLIB_ENTRY(null,Input)();
void AROS_SLIB_ENTRY(beginio,Input)();
LONG AROS_SLIB_ENTRY(abortio,Input)();

extern UWORD AROS_SLIB_ENTRY(PeekQualifier,Input) ();
extern void AROS_SLIB_ENTRY(AddNullEvent,Input) ();

static const char end;

/****************************************************************************************/

int AROS_SLIB_ENTRY(entry,Input)(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

/****************************************************************************************/

const struct Resident Input_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Input_resident,
    (APTR)&end,
    RTF_AUTOINIT|RTF_COLDSTART,
    41,
    NT_DEVICE,
    30,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="input.device";

static const char version[]="$VER: input 41.1 (17.3.2001)\r\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct inputbase),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,Input)
};

static void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,Input),
    &AROS_SLIB_ENTRY(close,Input),
    &AROS_SLIB_ENTRY(expunge,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(beginio,Input),
    &AROS_SLIB_ENTRY(abortio,Input),
    &AROS_SLIB_ENTRY(PeekQualifier,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(AddNullEvent,Input),    
    (void *)-1
};

/****************************************************************************************/

#if NEWSTYLE_DEVICE

static const UWORD SupportedCommands[] =
{
    IND_ADDHANDLER,
    IND_REMHANDLER,
    IND_WRITEEVENT,
    IND_SETTHRESH,
    IND_SETPERIOD,
    NSCMD_DEVICEQUERY,
    0
};

#endif

/****************************************************************************************/

AROS_UFH3(struct inputbase *, AROS_SLIB_ENTRY(init,Input),
 AROS_UFHA(struct inputbase *,	InputDevice,	D0),
 AROS_UFHA(BPTR,		segList,	A0),
 AROS_UFHA(struct ExecBase *,	sysBase,	A6))
{
    AROS_USERFUNC_INIT

    /* Store arguments */
    InputDevice->sysBase = sysBase;
    InputDevice->seglist = segList;

    NEWLIST( &(InputDevice->HandlerList) );

    /*
	These defaults are in terms of 50 Hz ticks. The real VBlank frequency
	does not effect them.
    */
    InputDevice->KeyRepeatThreshold.tv_secs = DEFAULT_KEY_REPEAT_THRESHOLD / 50;
    InputDevice->KeyRepeatThreshold.tv_micro
	= (DEFAULT_KEY_REPEAT_THRESHOLD % 50) * 1000000 / 50;

    InputDevice->KeyRepeatInterval.tv_secs  = DEFAULT_KEY_REPEAT_INTERVAL / 50;
    InputDevice->KeyRepeatInterval.tv_micro
	= (DEFAULT_KEY_REPEAT_INTERVAL % 50) * 1000000 / 50;

    (struct Library *)GfxBase = OpenLibrary("graphics.library", 0);
    
    /* Initialise the input.device task. */
    InputDevice->InputTask = AllocMem(sizeof(struct Task), MEMF_PUBLIC | MEMF_CLEAR);
    if(InputDevice->InputTask && GfxBase)
    {
	struct Task *task = InputDevice->InputTask;
	APTR stack;

	NEWLIST(&task->tc_MemEntry);
	task->tc_Node.ln_Type = NT_TASK;
	task->tc_Node.ln_Name = "input.device";
	task->tc_Node.ln_Pri = IDTASK_PRIORITY;

	/* Initialise CommandPort now we have the task */
	InputDevice->CommandPort.mp_SigTask = task;
	InputDevice->CommandPort.mp_Flags = PA_SIGNAL;
	NEWLIST(&InputDevice->CommandPort.mp_MsgList);
	
	/*
	 *  This is always safe, nobody else knows about our task yet.
	 *  Both the AROS and AmigaOS AddTask() initialise zeroed fields,
	 *  otherwise we have to do it ourselves.
	 */
	InputDevice->CommandPort.mp_SigBit = 16;
	task->tc_SigAlloc = 1L<<16 | SysBase->TaskSigAlloc;

	stack = AllocMem(IDTASK_STACKSIZE, MEMF_CLEAR|MEMF_PUBLIC);
	if(stack != NULL)
	{
#if 1
    	    struct TagItem tags[] =
	    {
	    	{TASKTAG_ARG1, (IPTR)InputDevice},
		{TAG_DONE   	    	    	}
	    };
	    
	    task->tc_SPLower = stack;
	    task->tc_SPUpper = (UBYTE *)stack + IDTASK_STACKSIZE;

    	#if AROS_STACK_GROWS_DOWNWARDS
	    task->tc_SPReg = (UBYTE *)task->tc_SPUpper - SP_OFFSET;
    	#else
	    task->tc_SPReg = (UBYTE *)task->tc_SPLower + SP_OFFSET;
    	#endif

	    if(NewAddTask(task, ProcessEvents, NULL, tags) != NULL)
	    {
		return InputDevice;
	    }
#else
	    task->tc_SPLower = stack;
	    task->tc_SPUpper = (UBYTE *)stack + IDTASK_STACKSIZE;

    	#if AROS_STACK_GROWS_DOWNWARDS
	    task->tc_SPReg = (UBYTE *)task->tc_SPUpper - SP_OFFSET - sizeof(APTR);
	    ((APTR *)task->tc_SPUpper)[-1] = InputDevice;
    	#else
	    task->tc_SPReg = (UBYTE *)task->tc_SPLower + SP_OFFSET + sizeof(APTR);
	    ((APTR *)task->tc_SPLower)[0] = InputDevice;
    	#endif

	    if(AddTask(task, ProcessEvents, NULL) != NULL)
	    {
		return InputDevice;
	    }
#endif
	}
    }

    Alert(AT_DeadEnd | AG_NoMemory | AO_Unknown | AN_Unknown);
    
    return NULL;
    
    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

AROS_LH3(void, open,
 AROS_LHA(struct IORequest *, ioreq, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct inputbase *, InputDevice, 1, Input)
{
    AROS_LIBFUNC_INIT

    D(bug("id: open()\n"));

    if (ioreq->io_Message.mn_Length < sizeof(struct IOStdReq))
    {
        bug("[InputDev] Open: IORequest structure passed to OpenDevice is too small\n");
        ioreq->io_Error = IOERR_OPENFAIL;
	return;
    }

    /* Keep compiler happy */
    unitnum=0;
    flags=0;
    
    ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

    InputDevice->device.dd_Library.lib_OpenCnt ++;
    InputDevice->device.dd_Library.lib_Flags&=~LIBF_DELEXP;
    
    return;    

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(BPTR, close,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct inputbase *, InputDevice, 2, Input)
{
    AROS_LIBFUNC_INIT

    /* Let any following attemps to use the device crash hard. */
    ioreq->io_Device=(struct Device *)-1;

    InputDevice->device.dd_Library.lib_OpenCnt--;

#if 0
    if (InputDevice->device.dd_Library.lib_OpenCnt == 0)
	expunge();
#endif    

    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, expunge, struct inputbase *, InputDevice, 3, Input)
{
    AROS_LIBFUNC_INIT

    /* Do not expunge the device. Set the delayed expunge flag and return. */
    InputDevice->device.dd_Library.lib_Flags |= LIBF_DELEXP;
    return 0;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0I(int, null, struct inputbase *, InputDevice, 4, Input)
{
    AROS_LIBFUNC_INIT
    
    return 0;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

#define ioStd(x)  ((struct IOStdReq *)x)
AROS_LH1(void, beginio,
 AROS_LHA(struct IOStdReq *, ioreq, A1),
	   struct inputbase *, InputDevice, 5, Input)
{
    AROS_LIBFUNC_INIT
    
    LONG error=0;    
    BOOL done_quick = TRUE;
    
    D(bug("id: beginio(ioreq=%p)\n", ioreq));

    /* WaitIO will look into this */
    ioreq->io_Message.mn_Node.ln_Type=NT_MESSAGE;

    switch (ioreq->io_Command)
    {
    #if NEWSTYLE_DEVICE
        case NSCMD_DEVICEQUERY:
	    if(ioStd(ioreq)->io_Length < ((LONG)OFFSET(NSDeviceQueryResult, SupportedCommands)) + sizeof(UWORD *))
	    {
		ioreq->io_Error = IOERR_BADLENGTH;
	    }
	    else
	    {
	        struct NSDeviceQueryResult *d;

    		d = (struct NSDeviceQueryResult *)ioStd(ioreq)->io_Data;
		
		d->DevQueryFormat 	 = 0;
		d->SizeAvailable 	 = sizeof(struct NSDeviceQueryResult);
		d->DeviceType 	 	 = NSDEVTYPE_INPUT;
		d->DeviceSubType 	 = 0;
		d->SupportedCommands 	 = (UWORD *)SupportedCommands;

		ioStd(ioreq)->io_Actual   = sizeof(struct NSDeviceQueryResult);
	    }
	    break;
    #endif

	case IND_ADDHANDLER:
	case IND_REMHANDLER:
	case IND_WRITEEVENT:
	case IND_SETTHRESH:
	case IND_SETPERIOD:
            done_quick = FALSE;
    	    break;

	default:
	    error = IOERR_NOCMD;
	    break;
    }
    
    if (!done_quick)
    {
        /* Mark IO request to be done non-quick */
    	ioreq->io_Flags &= ~IOF_QUICK;
    	/* Send to input device task */
    	PutMsg(&InputDevice->CommandPort, (struct Message *)ioreq);
    }
    else
    {

    	/* If the quick bit is not set but the IO request was done quick,
    	** reply the message to tell we're throgh
    	*/
    	ioreq->io_Error = error;
   	if (!(ioreq->io_Flags & IOF_QUICK))
	    ReplyMsg (&ioreq->io_Message);
    }

    D(bug("id: Return from BeginIO()\n"));

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct inputbase *, InputDevice, 6, Input)
{
    AROS_LIBFUNC_INIT
    
    /* Everything already done. */
    return 0;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

static const char end=0;

/****************************************************************************************/
