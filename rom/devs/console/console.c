/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Console.device
    Lang: English
*/

/****************************************************************************************/

#include <string.h>

#include <proto/exec.h>
#include <proto/console.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
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

#include "console_gcc.h"
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

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR ConsoleDevice)
{
    ConsoleDevice->cb_IntuitionBase = TaggedOpenLibrary(TAGGEDOPEN_INTUITION);
    if (!ConsoleDevice->cb_IntuitionBase)
        return FALSE;
    ConsoleDevice->cb_KeymapBase = TaggedOpenLibrary(TAGGEDOPEN_KEYMAP);
    if (!ConsoleDevice->cb_KeymapBase) {
        CloseLibrary(ConsoleDevice->cb_IntuitionBase);
        return FALSE;
    }

    NEWLIST(&ConsoleDevice->unitList);
    NEWLIST(&ConsoleDevice->sniphooks);
    InitSemaphore(&ConsoleDevice->unitListLock);
    InitSemaphore(&ConsoleDevice->consoleTaskLock);
    InitSemaphore(&ConsoleDevice->copyBufferLock);
    
    ConsoleDevice->copyBuffer = 0;
    ConsoleDevice->copyBufferSize = 0;

    /* Create the console classes */
    CONSOLECLASSPTR = makeConsoleClass(ConsoleDevice);
    STDCONCLASSPTR = makeStdConClass(ConsoleDevice);
	CHARMAPCLASSPTR = makeCharMapConClass(ConsoleDevice);
	SNIPMAPCLASSPTR = makeSnipMapConClass(ConsoleDevice);

    if (!CONSOLECLASSPTR || !STDCONCLASSPTR || !CHARMAPCLASSPTR || !SNIPMAPCLASSPTR)
	Alert(AT_DeadEnd | AN_ConsoleDev | AG_NoMemory);

    /* Create the console.device task. */
    ConsoleDevice->consoleTask = NewCreateTask(TASKTAG_NAME	  , "console.device",
					       TASKTAG_PRI 	  , COTASK_PRIORITY,
					       TASKTAG_STACKSIZE  , COTASK_STACKSIZE,
					       TASKTAG_TASKMSGPORT, &ConsoleDevice->commandPort,
					       TASKTAG_PC	  , consoleTaskEntry,
					       TASKTAG_ARG1	  , ConsoleDevice,
					       TAG_DONE);

    return ConsoleDevice->consoleTask ? TRUE : FALSE;
}

/****************************************************************************************/

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR ConsoleDevice)
{
    CloseLibrary(ConsoleDevice->cb_IntuitionBase);
    CloseLibrary(ConsoleDevice->cb_KeymapBase);
    return TRUE;
}

/****************************************************************************************/

static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR ConsoleDevice,
    struct IOStdReq *ioreq,
    ULONG unitnum,
    ULONG flags
)
{
    BOOL success = FALSE;
    
    /* Keep compiler happy */
    flags=0;
    
    EnterFunc(bug("OpenConsole()\n"));

    if (((LONG)unitnum) == CONU_LIBRARY) /* unitnum is ULONG while CONU_LIBRARY is -1 :-(   */
    {
    	D(bug("Opening CONU_LIBRARY unit\n"));
    	ioreq->io_Device = (struct Device *)ConsoleDevice;
	
	/* Set io_Unit so that CloseDevice knows this is a CONU_LIBRARY unit */
	/* WB1.3 Setmap sets io_Unit to CONU_LIBRARY (-1) before closing console.device */
	ioreq->io_Unit = (struct Unit*)CONU_LIBRARY;
	success = TRUE;
    }
    else
    {
	Class *classptr = NULL; /* Keep compiler happy */
#ifndef __mc68000
	/* AOS programs don't always initialize mn_Length. */
	if (ioreq->io_Message.mn_Length < sizeof(struct IOStdReq))
	{
	    D(bug("console.device/open: IORequest structure passed to OpenDevice is too small!\n"));
	    goto open_fail;
	}
#endif
	struct TagItem conunit_tags[] =
	{
	    {A_Console_Window,	0},
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
}

/****************************************************************************************/

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR ConsoleDevice,
    struct IORequest *ioreq
)
{
    if (ioreq->io_Unit && ioreq->io_Unit != (struct Unit*)CONU_LIBRARY)
    {
    	ULONG mid = OM_REMOVE;

	/* Remove the consoe from the console list */
	ObtainSemaphore(&ConsoleDevice->unitListLock);
	DoMethodA((Object *)ioreq->io_Unit, (Msg)&mid);
	ReleaseSemaphore(&ConsoleDevice->unitListLock);
	
    	DisposeObject((Object *)ioreq->io_Unit);
    }
    
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init),0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge),0)
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
	    D(bug("CMD_WRITE %p,%d\n", ioreq->io_Data, ioreq->io_Length));
#if DEBUG
	    {
	    	char *str;
	    	int i;
	    	str = ioreq->io_Data;
	    	for (i = 0; i < ioreq->io_Length; i ++)
	    	{
	    	    kprintf("%c", *str ++);
	    	}
		kprintf("\n");
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
	    D(bug("CMD_READ %p,%d\n", ioreq->io_Data, ioreq->io_Length));
#if DEBUG
	    {
	    	char *str;
	    	int i;
	    	str = ioreq->io_Data;
	    	for (i = 0; i < ioreq->io_Length; i ++)
	    	{
	    	    kprintf("%c", *str ++);
	    	}
		kprintf("\n");
	    }
#endif
	    done_quick = FALSE;

	    break;

	case CD_ASKKEYMAP:
	    /* FIXME: Returns always default keymap */
	    if (ioreq->io_Length < sizeof(struct KeyMap))
	    	error = IOERR_BADLENGTH;
	    else
	    	CopyMem(AskKeyMapDefault(), ioreq->io_Data, sizeof(struct KeyMap));
	    break;
	case CD_SETKEYMAP:
	    D(bug("CD_SETKEYMAP\n"));
	    error = IOERR_NOCMD;
	    break;
	case CD_ASKDEFAULTKEYMAP:
	    if (ioreq->io_Length < sizeof(struct KeyMap))
	    	error = IOERR_BADLENGTH;
	    else
	    	CopyMem(AskKeyMapDefault(), ioreq->io_Data, sizeof(struct KeyMap));
	    break;
	case CD_SETDEFAULTKEYMAP:
	    D(bug("CD_SETDEFAULTKEYMAP\n"));
	    error = IOERR_NOCMD;
	    break;

	default:
	    D(bug("IOERR_NOCMD %d\n", ioreq->io_Command));
	    error = IOERR_NOCMD;
	    break;

    } /* switch (ioreq->io_Command) */

    if (!done_quick)
    {
        /* Mark IO request to be done non-quick */
    	ioreq->io_Flags &= ~IOF_QUICK;
    	/* Send to input device task */
    	PutMsg(ConsoleDevice->commandPort, &ioreq->io_Message);
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

