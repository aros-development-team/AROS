/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Gameport device
    Lang: English
*/

/* NOTE: Currently, only mice are supported */

/****************************************************************************************/

#include <exec/interrupts.h>
#include <exec/initializers.h>
#include <hardware/intbits.h>
#include <devices/inputevent.h>
#include <devices/gameport.h>
#include <devices/newstyle.h>
#include <devices/rawkeycodes.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/lists.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <hidd/mouse.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include "gameport_intern.h"
#include "devs_private.h"

#ifdef  __GNUC__
#include "gameport_gcc.h"
#endif

#define DEBUG 0
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

/****************************************************************************************/

#define NEWSTYLE_DEVICE 1
#define ALIGN_IS_EVIL	1

#define ioStd(x)    	((struct IOStdReq *)x)
#define gpUn        	((struct GPUnit *)(ioreq->io_Unit))

#define min(a,b)    	((a) < (b)) ? (a) : (b)
#define ABS(a)      	((a) >= 0) ? (a) : (-(a))
#define ALIGN(x)    	((((x) + (__AROS_STRUCTURE_ALIGNMENT - 1)) / \
		    	__AROS_STRUCTURE_ALIGNMENT) * __AROS_STRUCTURE_ALIGNMENT)

#if ALIGN_IS_EVIL

#define NUM_INPUTEVENTS(bytesize) ((bytesize) / sizeof(struct InputEvent))
#define NEXT_INPUTEVENT(event)    (((struct InputEvent *)(event)) + 1)

#else

/* Number of InputEvents we can store in io_Data */
/* be careful, the io_Length might be the size of the InputEvent structure,
   but it can be that the ALIGN() returns a larger size and then nEvents would
   be 0.
 */

#define NUM_INPUTEVENTS(bytesize) (((bytesize) == sizeof(struct InputEvent)) ? \
    	    	    	    	   1 : (bytesize) / ALIGN(sizeof(struct InputEvent)))
#define NEXT_INPUTEVENT(event)	  ((struct InputEvent *)((UBYTE*)(event) + \
    	    	    	    	   ALIGN(sizeof(struct InputEvent))))

#endif /* ALIGN_IS_EVIL */

#define IECODE_DUMMY_WHEEL 0xFE

/****************************************************************************************/

#if NEWSTYLE_DEVICE

static const UWORD SupportedCommands[] =
{
    CMD_CLEAR,
    GPD_ASKCTYPE,
    GPD_SETCTYPE,
    GPD_ASKTRIGGER,
    GPD_SETTRIGGER,
    GPD_READEVENT,
    CMD_HIDDINIT,
    NSCMD_DEVICEQUERY,
    0
};

#endif

/****************************************************************************************/

static BOOL fillrequest(struct IORequest *ioreq, BOOL *trigged, struct GameportBase *GPBase);
static VOID mouseCallback(struct GameportBase *GPBase,
			  struct pHidd_Mouse_Event *ev);
AROS_UFP3S(VOID, gpSendQueuedEvents,
	   AROS_UFPA(struct GameportBase *, GPBase  , A1),
	   AROS_UFPA(APTR                 , thisfunc, A5),
	   AROS_UFPA(struct ExecBase *    , SysBase , A6));


/****************************************************************************************/

/* 'data' is a pointer to GPBase->gp_nTicks. */

AROS_UFH4(ULONG, gpVBlank,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, mySysBase, A6))
{ 
    AROS_USERFUNC_INIT

    if ((*(ULONG *)data) < ~0)
    {
	(*(ULONG *)data)++;
    }

    return 0;

    AROS_USERFUNC_EXIT
}

AROS_SET_LIBFUNC(GM_UNIQUENAME(init), LIBBASETYPE, GPBase)
{
    AROS_SET_LIBFUNC_INIT

    int i;

    /* reset static data */
    HiddMouseAB = 0;

    for(i = 0; i < GP_NUNITS; i++)
    {
	GPBase->gp_cTypes[i] = GPCT_NOCONTROLLER;
    }
    
    InitSemaphore(&GPBase->gp_QueueLock);
    InitSemaphore(&GPBase->gp_Lock);
    NEWLIST(&GPBase->gp_PendingQueue);

    GPBase->gp_Interrupt.is_Node.ln_Type = NT_INTERRUPT;
    GPBase->gp_Interrupt.is_Node.ln_Pri  = 0;
    GPBase->gp_Interrupt.is_Data 	 = (APTR)GPBase;
    GPBase->gp_Interrupt.is_Code 	 = gpSendQueuedEvents;

    GPBase->gp_VBlank.is_Code         = (APTR)&gpVBlank;
    GPBase->gp_VBlank.is_Data         = (APTR)&GPBase->gp_nTicks;
    GPBase->gp_VBlank.is_Node.ln_Name = "Gameport VBlank server";
    GPBase->gp_VBlank.is_Node.ln_Pri  = 0;
    GPBase->gp_VBlank.is_Node.ln_Type = NT_INTERRUPT;
	
    /* Add a VBLANK server to take care of event timing. */
    AddIntServer(INTB_VERTB, &GPBase->gp_VBlank);
    
    return TRUE;

    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_OPENDEVFUNC(GM_UNIQUENAME(open),
		     LIBBASETYPE, GPBase,
		     struct IORequest, ioreq,
		     unitnum, flags
)
{
    AROS_SET_DEVFUNC_INIT
	
    /* Erroneous unit? */
    if (unitnum > GP_MAXUNIT)
    {
	ioreq->io_Error = IOERR_OPENFAIL;

	return FALSE;
    }
    
    if (ioreq->io_Message.mn_Length < sizeof(struct IOStdReq))
    {
        D(bug("gameport.device/open: IORequest structure passed to OpenDevice "
	      "is too small!\n"));
        ioreq->io_Error = IOERR_OPENFAIL;

	return FALSE;
    }
    
    if (GPBase->gp_eventBuffer == NULL)
    {
	GPBase->gp_eventBuffer = AllocMem(sizeof(UWORD) * GP_BUFFERSIZE,
					  MEMF_ANY);
    }
    
    /* No memory for key buffer? */
    if (GPBase->gp_eventBuffer == NULL)
    {
	ioreq->io_Error = IOERR_OPENFAIL;

	return FALSE;
    }
    
    if ((ioreq->io_Unit = AllocMem(sizeof(GPUnit), MEMF_CLEAR)) == NULL)
    {
	ioreq->io_Error = IOERR_OPENFAIL;

	return FALSE;
    }

    gpUn->gpu_unitNum = unitnum;
        
    if (!GPBase->gp_OOPBase)
    {
	GPBase->gp_OOPBase = OpenLibrary(AROSOOP_NAME, 0);

	if(!GPBase->gp_OOPBase)
	{
	    ioreq->io_Error = IOERR_OPENFAIL;

	    return FALSE;
	}
    }
    
    if (!HiddMouseAB)
    {
        HiddMouseAB = OOP_ObtainAttrBase(IID_Hidd_Mouse);

	if (!HiddMouseAB)
	{
	    ioreq->io_Error = IOERR_OPENFAIL;
	    D(bug("gameport.device: Could not get attrbase\n"));

	    return FALSE;
	}
    }

    D(bug("gameport.device: Attrbase: %x\n", HiddMouseAB));
        
/******* nlorentz: End of stuff added by me ********/

    return TRUE;
    
    AROS_SET_DEVFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_CLOSEDEVFUNC(GM_UNIQUENAME(close),
		      LIBBASETYPE, GPBase,
		      struct IORequest, ioreq
)
{
    AROS_SET_DEVFUNC_INIT
	
    FreeMem(ioreq->io_Unit, sizeof(GPUnit));

    return TRUE;
    AROS_SET_DEVFUNC_EXIT
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(init),0)
ADD2OPENDEV(GM_UNIQUENAME(open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(close),0)

/****************************************************************************************/

AROS_LH1(void, beginio,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct GameportBase *, GPBase, 5, Gameport)
{
    AROS_LIBFUNC_INIT

    BOOL request_queued = FALSE;
    
    D(bug("gpd: beginio(ioreq=%p, cmd=%d)\n", ioreq, ioreq->io_Command));

    /* WaitIO will look into this */
    ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->io_Error = 0;
    
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
	    d->DeviceType  	 = NSDEVTYPE_GAMEPORT;
	    d->DeviceSubType 	 = 0;
	    d->SupportedCommands = (UWORD *)SupportedCommands;
	    
	    ioStd(ioreq)->io_Actual   = sizeof(struct NSDeviceQueryResult);
	}
	break;
#endif
	
    case CMD_CLEAR:
	gpUn->gpu_readPos = GPBase->gp_writePos;
	break;
	
    case GPD_ASKCTYPE:
	if (ioStd(ioreq)->io_Length < sizeof(UBYTE))
	{
	    ioreq->io_Error = IOERR_BADLENGTH;
	    break;
	}
	
	ObtainSemaphoreShared(&GPBase->gp_Lock);
	*((UBYTE *)(ioStd(ioreq)->io_Data)) = (GPBase->gp_cTypes)[gpUn->gpu_unitNum];
	ReleaseSemaphore(&GPBase->gp_Lock);
	break;
	
    case GPD_SETCTYPE:
	if (ioStd(ioreq)->io_Length != sizeof(UBYTE))
	{
	    ioreq->io_Error = IOERR_BADLENGTH;
	    break;
	}
	
	ObtainSemaphore(&GPBase->gp_Lock);
	(GPBase->gp_cTypes)[gpUn->gpu_unitNum] = *((UBYTE *)(ioStd(ioreq)->io_Data));
	ReleaseSemaphore(&GPBase->gp_Lock);
	break;
	
    case GPD_ASKTRIGGER:
	if (ioStd(ioreq)->io_Length != sizeof(struct GamePortTrigger))
	{
	    ioreq->io_Error = IOERR_BADLENGTH;
	    break;
	}
	
	*((struct GamePortTrigger *)(ioStd(ioreq)->io_Data)) = gpUn->gpu_trigger;
	break;
	
    case GPD_SETTRIGGER:
	if (ioStd(ioreq)->io_Length != sizeof(struct GamePortTrigger))
	{
	    ioreq->io_Error = IOERR_BADLENGTH;
	    break;
	}
	
	gpUn->gpu_trigger = *((struct GamePortTrigger *)(ioStd(ioreq)->io_Data));
	break;
	
    case GPD_READEVENT:
    #if 0
	if(((IPTR)(&(ioStd(ioreq)->io_Data)) & (__AROS_STRUCTURE_ALIGNMENT - 1)) != 0)
	{
	    D(bug("gpd: Bad address\n"));
	    ioreq->io_Error = IOERR_BADADDRESS;
	    break;
	}
    #endif
	
	D(bug("gpd: Readpos: %d, Writepos: %d\n", gpUn->gpu_readPos,
	      GPBase->gp_writePos));
	
	/* We queue the request if there are no events in the queue or if
	   the unit didn't trig on the events thate were in the queue. */
	
	Disable(); /* !! */
	
	if (gpUn->gpu_readPos == GPBase->gp_writePos)
	{
	    request_queued = TRUE;
	}
	else
	{
	    BOOL trigged;
	    
	    fillrequest(ioreq, &trigged, GPBase);
	    
	    if (!trigged)
	    {
		request_queued = TRUE;
	    }
	}
	
	if (request_queued)
	{
	    ioreq->io_Flags &= ~IOF_QUICK;
	    
	    D(bug("gpd: No mouse events, putting request in queue\n"));
	    
	    gpUn->gpu_flags |= GBUF_PENDING;
	    AddTail((struct List *)&GPBase->gp_PendingQueue,
		    (struct Node *)ioreq);
	}
	
	Enable();
	
	break;
	
	
	/* nlorentz: This command lets the gameport.device initialize
	   the HIDD to use. It must be done this way, because
	   HIDDs might be loaded from disk, and gameport.device is
	   inited before DOS is up and running.
	   The name of the HIDD class is in
	   ioStd(rew)->io_Data. Note that maybe we should
	   receive a pointer to an allreay created HIDD object instead.
	   Also note that the below is just a temporary hack, should
	   probably use IRQ HIDD instead to set the IRQ handler.
	*/   
	
    case CMD_HIDDINIT:
    {
	struct TagItem tags[] =
	{
	    { aHidd_Mouse_IrqHandler	, (IPTR)mouseCallback	},
	    { aHidd_Mouse_IrqHandlerData	, (IPTR)GPBase 		},
	    { TAG_DONE						}
	};
	IPTR relativecoords = FALSE;
	
	D(bug("gameport.device: Received CMD_HIDDINIT, hiddname=\"%s\"\n",
	      (STRPTR)ioStd(ioreq)->io_Data ));
	
	if (GPBase->gp_Hidd != NULL)
	    OOP_DisposeObject(GPBase->gp_Hidd);
	
	GPBase->gp_Hidd = OOP_NewObject(NULL, (STRPTR)ioStd(ioreq)->io_Data, tags);	
	if (!GPBase->gp_Hidd)
	{
	    D(bug("gameport.device: Failed to open hidd\n"));
	    ioreq->io_Error = IOERR_OPENFAIL;
	}
	
	OOP_GetAttr(GPBase->gp_Hidd, aHidd_Mouse_RelativeCoords, &relativecoords);

	if (relativecoords)
	{
    	    GPBase->gp_RelativeMouse = TRUE;
	}
	break;
    }
    
    default:
	ioreq->io_Error = IOERR_NOCMD;
	break;
	
    } /* switch (ioreq->io_Command) */
    
    /* If the quick bit is not set, send the message to the port */
    if (!(ioreq->io_Flags & IOF_QUICK) && !request_queued)
    {
	ReplyMsg(&ioreq->io_Message);
    }
    
    AROS_LIBFUNC_EXIT
}

/******************************************************************************/


AROS_LH1(LONG, abortio,
 AROS_LHA(struct IORequest *,    ioreq,  A1),
	  struct GameportBase *, GPBase, 6,  Gameport)
{
    AROS_LIBFUNC_INIT

    LONG ret = -1;
    
    Disable();

    if (gpUn->gpu_flags & GBUF_PENDING)
    {	
        if (ioreq->io_Message.mn_Node.ln_Type == NT_MESSAGE)
	{
	    Remove((struct Node *)ioreq);
	    ReplyMsg(&ioreq->io_Message);
	    
	    ioreq->io_Error = IOERR_ABORTED;
	    
	    if (IsListEmpty(&GPBase->gp_PendingQueue))
	    {
		gpUn->gpu_flags &= ~GBUF_PENDING;
	    }
	    
	    ret = 0;
	}
    }

    Enable();
    
    return ret;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

static VOID mouseCallback(struct GameportBase *GPBase,
			  struct pHidd_Mouse_Event *ev)
{
    UWORD amigacode = 0;
    
    D(bug("mouseCallBack(GPBase=%p, button=%d, x=%d, y=%d, type=%d)\n",
	  GPBase, ev->button, ev->x, ev->y, ev->type));
    
    /* Convert the event */
    switch (ev->button)
    {	
	case vHidd_Mouse_Button1:
	    amigacode = IECODE_LBUTTON;
	    break;

	case vHidd_Mouse_Button2:
	    amigacode = IECODE_RBUTTON;
	    break;

	case vHidd_Mouse_Button3:
	    amigacode = IECODE_MBUTTON;
	    break;	
    }
    
    switch (ev->type)
    {
	case vHidd_Mouse_Release:
	    amigacode |= IECODE_UP_PREFIX;
	    break;

	case vHidd_Mouse_Motion:
	    amigacode = IECODE_NOBUTTON;
	    break;
	    
	case vHidd_Mouse_WheelMotion:
	    amigacode = IECODE_DUMMY_WHEEL;
	    break;
    }
    
    Disable();
    
    GPBase->gp_eventBuffer[GPBase->gp_writePos++] = amigacode;
    GPBase->gp_eventBuffer[GPBase->gp_writePos++] = ev->x;
    GPBase->gp_eventBuffer[GPBase->gp_writePos++] = ev->y;
    
    D(bug("Wrote to buffer\n"));
    
    if (GPBase->gp_writePos == GP_NUMELEMENTS)
    {
	GPBase->gp_writePos = 0;
    }
    
    
    if (!IsListEmpty(&GPBase->gp_PendingQueue))
    {
#if 0
        D(bug("doing software irq, node type=%d\n", GPBase->gp_Interrupt.is_Node.ln_Type));
	Cause(&GPBase->gp_Interrupt);	
#else
	AROS_UFC3(VOID, gpSendQueuedEvents,
		  AROS_UFCA(struct GameportBase *, GPBase  , A1),
		  AROS_UFCA(APTR                 , NULL, A5),
		  AROS_UFCA(struct ExecBase *    , SysBase , A6));
#endif
    }
    
    Enable();
}

/****************************************************************************************/

/* nlorentz: Software interrupt to be called when keys are received
Copied and pasted from the function above */

#undef SysBase

AROS_UFH3S(VOID, gpSendQueuedEvents,
    AROS_UFHA(struct GameportBase *, GPBase, A1),
    AROS_UFHA(APTR, thisfunc, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    /* Broadcast keys */
    struct IORequest 	*ioreq, *nextnode;
    struct List 	*pendingList;
    
    pendingList = (struct List *)&GPBase->gp_PendingQueue;
    
    D(bug("Inside software irq\n"));
    
    ForeachNodeSafe(pendingList, ioreq, nextnode)
    {
        BOOL moreevents, trigged;
	
        D(bug("Replying msg\n"));
	moreevents = fillrequest(ioreq, &trigged, GPBase);

	if (trigged)
	{
	    Remove((struct Node *)ioreq);
 	    ReplyMsg((struct Message *)&ioreq->io_Message);
	}
	
	if (!moreevents)
	{
	    break;
	}
    }
    
    if (IsListEmpty(pendingList))
    {
	gpUn->gpu_flags &= ~GBUF_PENDING;
    }

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

/* When this function is called, there *must* be at least one event ready for
   processing. It returns TRUE as long as there are more events to preocess */

static BOOL fillrequest(struct IORequest *ioreq, BOOL *trigged,
			struct GameportBase *GPBase)
{
    BOOL   moreevents = TRUE;
    BOOL   down, up, wheel;
    int    i;			     /* Loop variable */
    int    nEvents;                  /* Number of struct InputEvent that there
					is room for in memory pointed to by
					io_Data */
    struct InputEvent *event;        /* Temporary variable */

    *trigged = FALSE;
         
    nEvents = NUM_INPUTEVENTS(ioStd(ioreq)->io_Length);
    
    if (nEvents == 0)
    {
	ioreq->io_Error = IOERR_BADLENGTH;
	D(bug("gpd: Bad length\n"));

	return TRUE;
    }
    
    event = (struct InputEvent *)(ioStd(ioreq)->io_Data);
    
    ioreq->io_Error = 0;

    for (i = 0; i < nEvents; ) /* no i++ here, this is done if event is to report */
    {
    	UWORD code;
	WORD  x;
	WORD  y;
		
	code = GPBase->gp_eventBuffer[gpUn->gpu_readPos++];
	x = GPBase->gp_eventBuffer[gpUn->gpu_readPos++];
	y = GPBase->gp_eventBuffer[gpUn->gpu_readPos++];
	
	down = up = wheel = FALSE;	/* Reset states */

	/* Take care of the qualifiers */
	switch (code)
	{
	    case IECODE_LBUTTON:
		gpUn->gpu_Qualifiers |= IEQUALIFIER_LEFTBUTTON;
		down = TRUE;
		break;

	    case IECODE_LBUTTON | IECODE_UP_PREFIX:
		gpUn->gpu_Qualifiers &= ~IEQUALIFIER_LEFTBUTTON;
		up = TRUE;
		break;

	    case IECODE_MBUTTON:
		gpUn->gpu_Qualifiers |= IEQUALIFIER_MIDBUTTON;
		down = TRUE;
		break;

	    case IECODE_MBUTTON | IECODE_UP_PREFIX:
		gpUn->gpu_Qualifiers &= ~IEQUALIFIER_MIDBUTTON;
		up = TRUE;
		break;

	    case IECODE_RBUTTON:
		gpUn->gpu_Qualifiers |= IEQUALIFIER_RBUTTON;
		down = TRUE;
		break;

	    case IECODE_RBUTTON | IECODE_UP_PREFIX:
		gpUn->gpu_Qualifiers &= ~IEQUALIFIER_RBUTTON;
		up = TRUE;
		break;
		
	    case IECODE_DUMMY_WHEEL:
	    	wheel = TRUE;
	    	if (y < 0)
		{
		    code = RAWKEY_NM_WHEEL_UP;
		}
		else if (y > 0)
		{
		    code = RAWKEY_NM_WHEEL_DOWN;
		}
		else if (x < 0)
		{
		    code = RAWKEY_NM_WHEEL_LEFT;
		}
		else if (x > 0)
		{
		    code = RAWKEY_NM_WHEEL_RIGHT;
		}
		else
		{
	    	    wheel = FALSE;
		}
		x = y = 0;
		break;
	}
	
	if (gpUn->gpu_readPos == GP_NUMELEMENTS)
	{
	    gpUn->gpu_readPos = 0;
	}
	
	D(bug("gpd: Adding event of code %d\n", code));

#if 0

#warning This needs to be fixed. And needs different handling, depending on whether coords are relative (x86-native) or not (x86-linux)!!!!

	
	/* Should we report this event? */    
	if((down && (gpUn->gpu_trigger.gpt_Keys & GPTF_DOWNKEYS)) ||
	   (up   && (gpUn->gpu_trigger.gpt_Keys & GPTF_UPKEYS))   ||
	   (ABS(gpUn->gpu_lastX - x) > gpUn->gpu_trigger.gpt_XDelta) ||
	   (ABS(gpUn->gpu_lastY - y) > gpUn->gpu_trigger.gpt_YDelta) ||
	   (GPBase->gp_nTicks > gpUn->gpu_trigger.gpt_Timeout) ||
	   (wheel))
#endif

	{
	    i++;
	    
	    if (*trigged == TRUE)
	    {
	        event = event->ie_NextEvent;
	    }
	    else
	    {
	        *trigged = TRUE;
	    }
	    
	    event->ie_Class 	= (wheel ? IECLASS_NEWMOUSE : IECLASS_RAWMOUSE);
	    event->ie_SubClass  = 0;          /* Only port 0 for now */
	    event->ie_Code  	= code;
	    event->ie_Qualifier = gpUn->gpu_Qualifiers;
	    if (GPBase->gp_RelativeMouse) event->ie_Qualifier |= IEQUALIFIER_RELATIVEMOUSE;
	    
	    event->ie_X = x;
	    event->ie_Y = y;
	    
	    gpUn->gpu_lastX = x;
	    gpUn->gpu_lastY = y;
	    
	    event->ie_TimeStamp.tv_secs = GPBase->gp_nTicks;
	    event->ie_TimeStamp.tv_micro = 0;
	    
	    /* Reset frame delta counter */
	    GPBase->gp_nTicks = 0;
	    
	    
	    event->ie_NextEvent = NEXT_INPUTEVENT(event);
	}
	
	/* No more keys in buffer? */
	if (gpUn->gpu_readPos == GPBase->gp_writePos)
	{
	    moreevents = FALSE;
	    break;
	}
    }
    
    event->ie_NextEvent = NULL;
    
    return moreevents;
}

/****************************************************************************************/

static const char end = 0;

/****************************************************************************************/
