/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Console.device
    Lang: English
*/

/****************************************************************************************/


#include <string.h>

#include <proto/exec.h>
#include <proto/console.h>
#include <proto/intuition.h>
#include <exec/resident.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/initializers.h>
#include <devices/inputevent.h>
#include <devices/conunit.h>
#include <devices/newstyle.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/classusr.h>
#include <graphics/rastport.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <graphics/rastport.h>

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#    include "console_gcc.h"
#endif

#include "consoleif.h"

#include LC_LIBDEFS_FILE

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

#define NEWSTYLE_DEVICE 1

/****************************************************************************************/


/****************************************************************************************/

#if NEWSTYLE_DEVICE

static const UWORD SupportedCommands[] =
{
    CMD_READ,
    CMD_WRITE,
    NSCMD_DEVICEQUERY,
    0
};

#endif

/****************************************************************************************/

AROS_SET_LIBFUNC(GM_UNIQUENAME(Init), LIBBASETYPE, ConsoleDevice)
{
    AROS_SET_LIBFUNC_INIT
    
    NEWLIST(&ConsoleDevice->unitList);
    InitSemaphore(&ConsoleDevice->unitListLock);
    InitSemaphore(&ConsoleDevice->consoleTaskLock);
    
    ConsoleDevice->gfxBase = (GraphicsBase *)OpenLibrary("graphics.library", 37);
    if (!ConsoleDevice->gfxBase)
	Alert(AT_DeadEnd | AN_ConsoleDev | AG_OpenLib | AO_GraphicsLib);

    ConsoleDevice->intuitionBase = (IntuiBase *)OpenLibrary("intuition.library", 37);
    if (!ConsoleDevice->intuitionBase)
	Alert(AT_DeadEnd | AN_ConsoleDev | AG_OpenLib | AO_Intuition);

    ConsoleDevice->utilityBase = OpenLibrary("utility.library", 37);
    if (!ConsoleDevice->utilityBase)
	Alert(AT_DeadEnd | AN_ConsoleDev | AG_OpenLib | AO_UtilityLib);

    ConsoleDevice->keymapBase = OpenLibrary("keymap.library", 37);
    if (!ConsoleDevice->keymapBase)
	Alert(AT_DeadEnd | AN_ConsoleDev | AG_OpenLib | AO_KeyMapLib);
    
    /* Create the console classes */
    CONSOLECLASSPTR = makeConsoleClass(ConsoleDevice);
    STDCONCLASSPTR = makeStdConClass(ConsoleDevice);
    if (!CONSOLECLASSPTR || !STDCONCLASSPTR)
	Alert(AT_DeadEnd | AN_ConsoleDev | AG_NoMemory);

    /* Create the console.device task. */
    ConsoleDevice->consoleTask = AllocMem(sizeof(struct Task), MEMF_CLEAR|MEMF_PUBLIC);
    if(ConsoleDevice->consoleTask)
    {
	struct Task * const task = ConsoleDevice->consoleTask;
	APTR stack;

	/* Initialise the task */
	NEWLIST(&task->tc_MemEntry);
	task->tc_Node.ln_Type = NT_TASK;
	task->tc_Node.ln_Name = "console.device";
	task->tc_Node.ln_Pri = COTASK_PRIORITY;

	/* Initialise Command Port now we have the task */
	ConsoleDevice->commandPort.mp_Node.ln_Type = NT_MSGPORT;
	ConsoleDevice->commandPort.mp_SigTask = task;
	ConsoleDevice->commandPort.mp_Flags = PA_SIGNAL;
	ConsoleDevice->commandPort.mp_SigBit = 16;
	NEWLIST(&ConsoleDevice->commandPort.mp_MsgList);

	task->tc_SigAlloc = 1L<<16 | SysBase->TaskSigAlloc;

	stack = AllocMem(COTASK_STACKSIZE, MEMF_PUBLIC);
	if(stack != NULL)
	{
#if 1
    	    struct TagItem tags[] =
	    {
	    	{TASKTAG_ARG1, (IPTR)ConsoleDevice  },
		{TAG_DONE   	    	    	    }
	    };
	    
	    task->tc_SPLower = stack;
	    task->tc_SPUpper = (UBYTE *)stack + COTASK_STACKSIZE;
    	#if AROS_STACK_GROWS_DOWNWARDS
	    task->tc_SPReg = (UBYTE *)task->tc_SPUpper - SP_OFFSET;
    	#else
	    task->tc_SPReg = (UBYTE *)task->tc_SPLower + SP_OFFSET;
    	#endif

	    if(NewAddTask(task, consoleTaskEntry, NULL, tags) != NULL)
	    {
		return ConsoleDevice;
		/* ALL OK */
	    }
	    
#else
	    task->tc_SPLower = stack;
	    task->tc_SPUpper = (UBYTE *)stack + COTASK_STACKSIZE;

    	#if AROS_STACK_GROWS_DOWNWARDS
	    task->tc_SPReg = (UBYTE *)task->tc_SPUpper - SP_OFFSET - sizeof(APTR);
	    ((APTR *)task->tc_SPUpper)[-1] = ConsoleDevice;
    	#else
	    task->tc_SPReg = (UBYTE *)task->tc_SPLower + SP_OFFSET + sizeof(APTR);
	    ((APTR *)(task->tc_SPLower + SP_OFFSET))[0] = ConsoleDevice;
    	#endif

	    if(AddTask(task, consoleTaskEntry, NULL) != NULL)
	    {
		return TRUE;
		/* ALL OK */
	    }

#endif

	    FreeMem(stack, COTASK_STACKSIZE);
	}
	FreeMem(task, sizeof(struct Task));
    }

    Alert(AT_DeadEnd | AN_ConsoleDev | AG_NoMemory);
    return FALSE;
    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_OPENDEVFUNC(GM_UNIQUENAME(Open),
		     LIBBASETYPE, ConsoleDevice,
		     struct IOStdReq, ioreq,
		     unitnum, flags
)
{
    AROS_SET_DEVFUNC_INIT
    
    BOOL success = FALSE;
    
    /* Keep compiler happy */
    flags=0;
    
    EnterFunc(bug("OpenConsole()\n"));

    if (ioreq->io_Message.mn_Length < sizeof(struct IOStdReq))
    {
        D(bug("console.device/open: IORequest structure passed to OpenDevice is too small!\n"));
        goto open_fail;
    }
    
    if (((LONG)unitnum) == CONU_LIBRARY) /* unitnum is ULONG while CONU_LIBRARY is -1 :-(   */
    {
    	D(bug("Opening CONU_LIBRARY unit\n"));
    	ioreq->io_Device = (struct Device *)ConsoleDevice;
	
	/* Set io_Unit to NULL, so that CloseDevice knows this is a CONU_LIBRARY unit */
	ioreq->io_Unit = NULL;
	success = TRUE;
    }
    else
    {
	Class *classptr = NULL; /* Keep compiler happy */
	
	struct TagItem conunit_tags[] =
	{
	    {A_Console_Window,	NULL},
	    {TAG_DONE, 0}
	};
	
	/* Init tags */
	
	conunit_tags[0].ti_Data = (IPTR)ioreq->io_Data; /* Window */
	
    	
	/* Select class of which to create console object */
    	switch (unitnum)
    	{
    	    case CONU_STANDARD:
	    	D(bug("Opening CONU_STANDARD console\n"));
    	    	classptr = STDCONCLASSPTR;
    	    	break;
    		
    	    case CONU_CHARMAP:
    		classptr = CHARMAPCLASSPTR;
    		break;
    		
    	    case CONU_SNIPMAP:
    	    	classptr = SNIPMAPCLASSPTR;
    	    	break;

	    default:
	    	goto open_fail;


	}
    	/* Create console object */
    	ioreq->io_Unit = (struct Unit *)NewObjectA(classptr, NULL, conunit_tags);
    	if (ioreq->io_Unit)
    	{
	    struct opAddTail add_msg;
    	    success = TRUE;
	    
	    /* Add the newly created unit to console's list of units */
	    ObtainSemaphore(&ConsoleDevice->unitListLock);

	    add_msg.MethodID = OM_ADDTAIL;
	    add_msg.opat_List = (struct List *)&ConsoleDevice->unitList;
	    DoMethodA((Object *)ioreq->io_Unit, (Msg)&add_msg);
	    
	    ReleaseSemaphore(&ConsoleDevice->unitListLock);
    	} /* if (console unit created) */
    	
    } /* if (not CONU_LIBRARY) */

    if (!success)
    	goto open_fail;

    return TRUE;
    
open_fail:

    ioreq->io_Error = IOERR_OPENFAIL;    

    return FALSE;

    AROS_SET_DEVFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_CLOSEDEVFUNC(GM_UNIQUENAME(Close),
		      LIBBASETYPE, ConsoleDevice,
		      struct IORequest, ioreq
)
{
    AROS_SET_DEVFUNC_INIT
    
    if (ioreq->io_Unit)
    {
    	ULONG mid = OM_REMOVE;

	/* Remove the consoe from the console list */
	ObtainSemaphore(&ConsoleDevice->unitListLock);
	DoMethodA((Object *)ioreq->io_Unit, (Msg)&mid);
	ReleaseSemaphore(&ConsoleDevice->unitListLock);
	
    	DisposeObject((Object *)ioreq->io_Unit);
    }
    
    return 0;
    AROS_SET_DEVFUNC_EXIT
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init),0)
ADD2OPENDEV(GM_UNIQUENAME(Open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close),0)

/****************************************************************************************/

AROS_LH1(void, beginio,
 AROS_LHA(struct IOStdReq *, ioreq, A1),
	   struct ConsoleBase *, ConsoleDevice, 5, Console)
{
    AROS_LIBFUNC_INIT
    LONG error=0;

    BOOL done_quick = TRUE;

    /* WaitIO will look into this */
    ioreq->io_Message.mn_Node.ln_Type=NT_MESSAGE;

    EnterFunc(bug("BeginIO(ioreq=%p)\n", ioreq));

    switch (ioreq->io_Command)
    {
#if NEWSTYLE_DEVICE
        case NSCMD_DEVICEQUERY:
	    if(ioreq->io_Length < ((LONG)OFFSET(NSDeviceQueryResult, SupportedCommands)) + sizeof(UWORD *))
	    {
		ioreq->io_Error = IOERR_BADLENGTH;
	    }
	    else
	    {
	        struct NSDeviceQueryResult *d;

    		d = (struct NSDeviceQueryResult *)ioreq->io_Data;

		d->DevQueryFormat 	 = 0;
		d->SizeAvailable 	 = sizeof(struct NSDeviceQueryResult);
		d->DeviceType 	 	 = NSDEVTYPE_CONSOLE;
		d->DeviceSubType 	 = 0;
		d->SupportedCommands 	 = (UWORD *)SupportedCommands;

		ioreq->io_Actual = sizeof(struct NSDeviceQueryResult);
	    }
	    break;
#endif


    	case CMD_WRITE: {
	    ULONG towrite;

#if DEBUG
	    {
	    	char *str;
	    	int i;
	    	str = ioreq->io_Data;
	    	for (i = 0; i < ioreq->io_Length; i ++)
	    	{
	    	    kprintf("%c\n", *str ++);
	    	}
	    }
#endif
	    if (ioreq->io_Length == -1) {
	    	towrite = strlen((STRPTR)ioreq->io_Data);
	    } else {
	    	towrite = ioreq->io_Length;
	    }


	    ioreq->io_Actual = writeToConsole((struct ConUnit *)ioreq->io_Unit
	    	, ioreq->io_Data
		, towrite
		, ConsoleDevice
	    );

    	    break; }

	case CMD_READ:
	    done_quick = FALSE;

	    break;

	default:
	    error = IOERR_NOCMD;
	    break;

    } /* switch (ioreq->io_Command) */

    if (!done_quick)
    {
        /* Mark IO request to be done non-quick */
    	ioreq->io_Flags &= ~IOF_QUICK;
    	/* Send to input device task */
    	PutMsg(&ConsoleDevice->commandPort, (struct Message *)ioreq);
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

    ReturnVoid("BeginIO");
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct ConsoleBase *, ConsoleDevice, 6, Console)
{
    AROS_LIBFUNC_INIT

    LONG ret = -1;

    ObtainSemaphore(&ConsoleDevice->consoleTaskLock);

    /* The ioreq can either be in the ConsoleDevice->commandPort MsgPort,
       or be in the ConsoleDevice->readRequests List, or be already done.

       In the first two cases ln_Type will be NT_MESSAGE (= it can be
       aborted), in the last case ln_Type will be NT_REPLYMSG (cannot
       abort, because already done)

       The consoleTaskLock Semaphore hopefully makes sure that there are no
       other/"in-between" cases.

    */

    if (ioreq->io_Message.mn_Node.ln_Type != NT_REPLYMSG)
    {
	ioreq->io_Error = IOERR_ABORTED;
	Remove(&ioreq->io_Message.mn_Node);
	ReplyMsg(&ioreq->io_Message);

	ret = 0;
    }

    ReleaseSemaphore(&ConsoleDevice->consoleTaskLock);

    return ret;

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

