/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
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
    NSCMD_DEVICEQUERY,
    0
};

#endif

/****************************************************************************************/

static BOOL fillrequest(struct IORequest *ioreq, BOOL *trigged, struct GameportBase *GPBase);
static VOID mouseCallback(struct GameportBase *GPBase,
			  struct pHidd_Mouse_ExtEvent *ev);
static AROS_INTP(gpSendQueuedEvents);


/****************************************************************************************/

/* 'data' is a pointer to GPBase->gp_nTicks. */

AROS_INTH1(gpVBlank, LIBBASETYPEPTR, GPBase)
{ 
    AROS_INTFUNC_INIT

    if (GPBase->gp_nTicks < ~0)
    {
        GPBase->gp_nTicks++;
    }

    return 0;

    AROS_INTFUNC_EXIT
}

static int GM_UNIQUENAME(init)(LIBBASETYPEPTR GPBase)
{
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
    GPBase->gp_Interrupt.is_Code 	 = (VOID_FUNC)gpSendQueuedEvents;

    GPBase->gp_VBlank.is_Code         = (VOID_FUNC)gpVBlank;
    GPBase->gp_VBlank.is_Data         = GPBase;
    GPBase->gp_VBlank.is_Node.ln_Name = "Gameport VBlank server";
    GPBase->gp_VBlank.is_Node.ln_Pri  = 0;
    GPBase->gp_VBlank.is_Node.ln_Type = NT_INTERRUPT;
	
    /* Add a VBLANK server to take care of event timing. */
    AddIntServer(INTB_VERTB, &GPBase->gp_VBlank);
    
    return TRUE;
}

/****************************************************************************************/

static int GM_UNIQUENAME(open)
(
    LIBBASETYPEPTR GPBase,
    struct IORequest *ioreq,
    ULONG unitnum,
    ULONG flags
)
{
    struct Library *OOPBase;

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
        
    OOPBase = OpenLibrary("oop.library", 0);
    if (!OOPBase) {
        ioreq->io_Error = IOERR_OPENFAIL;
        return FALSE;
    }

    if (!HiddMouseAB)
    {
        HiddMouseAB = OOP_ObtainAttrBase(IID_Hidd_Mouse);

	if (!HiddMouseAB)
	{
	    ioreq->io_Error = IOERR_OPENFAIL;
	    CloseLibrary(OOPBase);
	    D(bug("gameport.device: Could not get attrbase\n"));

	    return FALSE;
	}
    }

    D(bug("gameport.device: Attrbase: %x\n", HiddMouseAB));
        
/******* nlorentz: End of stuff added by me ********/

    if(!GPBase->gp_MouseHiddBase)
    {
	GPBase->gp_MouseHiddBase = OpenLibrary("mouse.hidd", 0);

	/* Install our own keyboard handler if opened for the first time */
	if(GPBase->gp_MouseHiddBase) {
	    struct TagItem tags[] = {
		{ aHidd_Mouse_IrqHandler    , (IPTR)mouseCallback},
		{ aHidd_Mouse_IrqHandlerData, (IPTR)GPBase	 },
		{ TAG_DONE					 }
	    };

	    GPBase->gp_Hidd = OOP_NewObject(NULL, CLID_Hidd_Mouse, tags);
	    D(bug("keyboard.device: keyboard HIDD object 0x%p\n", GPBase->gp_Hidd));
 	    if(!GPBase->gp_Hidd)
	    {
	        CloseLibrary(GPBase->gp_MouseHiddBase);
		GPBase->gp_MouseHiddBase = NULL; /* Do cleanup below. */
            }
	}

    }
    CloseLibrary(OOPBase);

    if(!GPBase->gp_MouseHiddBase)
    {
	ioreq->io_Error = IOERR_OPENFAIL;
	return FALSE;
	/* TODO: Clean up. */
    }

    return TRUE;
}

/****************************************************************************************/

static int GM_UNIQUENAME(close)
(
    LIBBASETYPEPTR GPBase,
    struct IORequest *ioreq
)
{
    FreeMem(ioreq->io_Unit, sizeof(GPUnit));

    return TRUE;
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
			  struct pHidd_Mouse_ExtEvent *ev)
{
    UWORD amigacode = 0;
    
    D(bug("mouseCallBack(GPBase=%p, button=%d, x=%d, y=%d, type=%d, flags=0x%04X)\n",
	  GPBase, ev->button, ev->x, ev->y, ev->type, ev->flags));
    
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
    GPBase->gp_eventBuffer[GPBase->gp_writePos++] = ev->flags;
    
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
    AROS_INTC1(gpSendQueuedEvents, GPBase);
#endif
    }
    
    Enable();
}

/****************************************************************************************/

/* nlorentz: Software interrupt to be called when keys are received
Copied and pasted from the function above */

#undef SysBase

static AROS_INTH1(gpSendQueuedEvents, struct GameportBase *, GPBase)
{
    AROS_INTFUNC_INIT

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

    return 0;

    AROS_INTFUNC_EXIT
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
	WORD  flags;
		
	code = GPBase->gp_eventBuffer[gpUn->gpu_readPos++];
	x = GPBase->gp_eventBuffer[gpUn->gpu_readPos++];
	y = GPBase->gp_eventBuffer[gpUn->gpu_readPos++];
	flags = GPBase->gp_eventBuffer[gpUn->gpu_readPos++];
	
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
#else
        (void)(wheel); /* This is unused */
        (void)(down);  /* This is unused */
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
	    if (flags & vHidd_Mouse_Relative) event->ie_Qualifier |= IEQUALIFIER_RELATIVEMOUSE;
	    
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
