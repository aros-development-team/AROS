/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Keyboard device
    Lang: English
*/

/* HISTORY:  12.04.98  SDuvan  Began work
             xx.06.98  SDuvan  Fixes, added amigakeyboard.HIDD
 */

/****************************************************************************************/

#include <exec/resident.h>
#include <exec/interrupts.h>
#include <exec/initializers.h>
#include <devices/inputevent.h>
#include <devices/keyboard.h>
#include <devices/newstyle.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/lists.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <hidd/keyboard.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include "abstractkeycodes.h"
#include "keyboard_intern.h"
#include "devs_private.h"

#ifdef  __GNUC__
#include "keyboard_gcc.h"
#endif

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

#define NEWSTYLE_DEVICE 1
#define ALIGN_IS_EVIL	1

#define ioStd(x)    	((struct IOStdReq *)x)
#define kbUn        	((struct KBUnit *)(ioreq->io_Unit))

#define min(a,b)    	((a) < (b)) ? (a) : (b)
#define ALIGN(x)    	((((x) + (__AROS_STRUCTURE_ALIGNMENT - 1)) / __AROS_STRUCTURE_ALIGNMENT) * __AROS_STRUCTURE_ALIGNMENT)

#define isQualifier(x)  ((((x) & ~KEYUPMASK) >= AKC_QUALIFIERS_FIRST) && (((x) & ~KEYUPMASK) <= AKC_QUALIFIERS_LAST))

/* Temporary - we should make a bit vector of this to check for numeric pad keys */
#define isNumericPad(x)  ((x) == AKC_NUM_1 || (x) == AKC_NUM_2 || \
			  (x) == AKC_NUM_3 || (x) == AKC_NUM_4 || \
			  (x) == AKC_NUM_5 || (x) == AKC_NUM_6 || \
			  (x) == AKC_NUM_7 || (x) == AKC_NUM_8 || \
			  (x) == AKC_NUM_9 || (x) == AKC_NUM_0 || \
			  (x) == AKC_NUM_POINT  || (x) == AKC_NUM_ENTER  || \
			  (x) == AKC_NUM_DASH   || (x) == AKC_NUM_LPAREN || \
			  (x) == AKC_NUM_RPAREN || (x) == AKC_NUM_SLASH  || \
			  (x) == AKC_NUM_PLUS   || (x) == AKC_NUM_TIMES)

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

/****************************************************************************************/

#if NEWSTYLE_DEVICE

static const UWORD SupportedCommands[] =
{
    CMD_CLEAR,
    KBD_ADDRESETHANDLER,
    KBD_REMRESETHANDLER,
    KBD_RESETHANDLERDONE,
    KBD_READMATRIX,
    KBD_READEVENT,
    CMD_HIDDINIT,
    NSCMD_DEVICEQUERY,
    0
};

#endif

/****************************************************************************************/

VOID keyCallback(struct KeyboardBase *KBBase, UWORD keyCode);
AROS_UFP3(VOID, kbdSendQueuedEvents,
    AROS_UFPA(struct KeyboardBase *, KBBase, A1),
    AROS_UFPA(APTR, thisfunc, A5),
    AROS_UFPA(struct ExecBase *, SysBase, A6));
static BOOL writeEvents(struct IORequest *ioreq, struct KeyboardBase *KBBase);
    
/****************************************************************************************/

AROS_SET_LIBFUNC(GM_UNIQUENAME(Init), LIBBASETYPE, KBBase)
{
    AROS_SET_LIBFUNC_INIT

    /* reset static data */
    HiddKbdAB = 0;

    InitSemaphore(&KBBase->kb_QueueLock);
    NEWLIST(&KBBase->kb_ResetHandlerList);
    NEWLIST(&KBBase->kb_PendingQueue);
    
    return TRUE;
    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_OPENDEVFUNC(GM_UNIQUENAME(Open),
		    LIBBASETYPE, KBBase,
		    struct IORequest, ioreq,
		    unitnum, flags
)
{
    AROS_SET_DEVFUNC_INIT

    if (ioreq->io_Message.mn_Length < sizeof(struct IOStdReq))
    {
        D(bug("keyport.device/open: IORequest structure passed to OpenDevice is too small!\n"));
        ioreq->io_Error = IOERR_OPENFAIL;
	return FALSE;
    }
    
    if(KBBase->kb_keyBuffer == NULL)
    {
	KBBase->kb_keyBuffer = AllocMem(sizeof(UWORD)*KB_BUFFERSIZE, MEMF_ANY);
    }

    /* No memory for key buffer? */
    if(KBBase->kb_keyBuffer == NULL)
    {
	ioreq->io_Error = IOERR_OPENFAIL;
	return FALSE;
    }

    if((ioreq->io_Unit = AllocMem(sizeof(KBUnit), MEMF_CLEAR)) == NULL)
    {
	ioreq->io_Error = IOERR_OPENFAIL;
	return FALSE;
    }

/* nlorentz: Some extra stuff that must be inited */
    if (NULL == KBBase->kb_Matrix)
    {
        KBBase->kb_Matrix = AllocMem(KB_MATRIXSIZE, MEMF_ANY|MEMF_CLEAR);

	if (NULL == KBBase->kb_Matrix)
	{
	    ioreq->io_Error = IOERR_OPENFAIL;
	    return FALSE;
	}
    }
    
    if (!KBBase->kb_OOPBase)
    {
	KBBase->kb_OOPBase = OpenLibrary(AROSOOP_NAME, 0);
	if (!KBBase->kb_OOPBase)
	{
	    ioreq->io_Error = IOERR_OPENFAIL;
	    return FALSE;
	}
    }
    
    if (!HiddKbdAB)
    {
        HiddKbdAB = OOP_ObtainAttrBase(IID_Hidd_Kbd);
	if (!HiddKbdAB)
	{
	    ioreq->io_Error = IOERR_OPENFAIL;
	    D(bug("keyboard.device: Could not get attrbase\n"));
	    return FALSE;
	}
    }
    D(bug("keyboard.device: Attrbase: %x\n", HiddKbdAB));
    
    KBBase->kb_Interrupt.is_Node.ln_Type = NT_INTERRUPT;
    KBBase->kb_Interrupt.is_Node.ln_Pri = 0;
    KBBase->kb_Interrupt.is_Data = (APTR)KBBase;
    KBBase->kb_Interrupt.is_Code = kbdSendQueuedEvents;
	
/******* nlorentz: End of stuff added by me ********/
    

/* nlorentz: No lowlevel library yet */
#if 0
    if(!KBBase->kb_LowLevelBase)
    {
	KBBase->kb_LowLevelBase = OpenLibrary("lowlevel.library", 41);

	/* Install our own keyboard handler if opened for the first time */
	if(KBBase->kb_LowLevelBase)
 	    if((KBBase->kb_kbIrqHandle = AddKBInt(keyCallback, KBBase)) == NULL)
	    {
	        CloseLibrary(KBBase->kb_LowLevelBase);
		KBBase->kb_LowLevelBase = NULL; /* Do cleanup below. */
            }

    }
    
    if(!KBBase->kb_LowLevelBase)
    {
	ioreq->io_Error = IOERR_OPENFAIL;
	return FALSE;
	/* TODO: Clean up. */
    }
#endif

    return TRUE;
    
    AROS_SET_DEVFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_CLOSEDEVFUNC(GM_UNIQUENAME(Close),
		      LIBBASETYPE, KBBase,
		      struct IORequest, ioreq
)
{
    AROS_SET_DEVFUNC_INIT

    FreeMem(ioreq->io_Unit, sizeof(KBUnit));

    return TRUE;
    AROS_SET_DEVFUNC_EXIT
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

/****************************************************************************************/

AROS_LH1(void, beginio,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct KeyboardBase *, KBBase, 5, Keyboard)
{
    AROS_LIBFUNC_INIT
    
    BOOL request_queued = FALSE;
    
    
    D(bug("kbd: beginio(ioreq=%p, cmd=%d)\n", ioreq, ioreq->io_Command));
    
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
		d->DeviceType 	 	 = NSDEVTYPE_KEYBOARD;
		d->DeviceSubType 	 = 0;
		d->SupportedCommands 	 = (UWORD *)SupportedCommands;

		ioStd(ioreq)->io_Actual   = sizeof(struct NSDeviceQueryResult);
	    }
	    break;
#endif

	case CMD_CLEAR:
	    kbUn->kbu_readPos = KBBase->kb_writePos;
	    break;

	case KBD_ADDRESETHANDLER:
	    Disable();
	    Enqueue((struct List *)(&KBBase->kb_ResetHandlerList),
		    (struct Node *)(ioStd(ioreq)->io_Data));
	    KBBase->kb_nHandlers++;
	    Enable();
	    break;

	case KBD_REMRESETHANDLER:
	    Disable();
	    Remove((struct Node *)(ioStd(ioreq)->io_Data));
	    KBBase->kb_nHandlers--;
	    Enable();
	    break;

	case KBD_RESETHANDLERDONE:
	    /* We don't want any phony resets. */
	    
	    if(KBBase->kb_ResetPhase == TRUE)
	    {
		if(--(KBBase->kb_nHandlers) <= 0)
		    ColdReboot();	/* Shut down system */
	    }
	    else
	    {
		/* There is no good (defined) IOERR to return in this situation */
		ioreq->io_Error = IOERR_NOCMD;
	    }
	    break;

	case KBD_READMATRIX:
	    ioStd(ioreq)->io_Actual = min(KB_MATRIXSIZE, ioStd(ioreq)->io_Length);
	    CopyMem(KBBase->kb_Matrix, ioStd(ioreq)->io_Data,
		    ioStd(ioreq)->io_Actual);
	    break;

	case KBD_READEVENT:

	    /* TODO */
	    /* Check for reset... via keybuffer or via HIDD? */
	    /* if(bufferkey == 0x78) ... */

    	#if 0
	    if((((IPTR)ioStd(ioreq)->io_Data) & (__AROS_STRUCTURE_ALIGNMENT - 1)) != 0)
	    {
		D(bug("kbd: Bad address\n"));
		ioreq->io_Error = IOERR_BADADDRESS;
		break;
	    }
    	#endif
	
	    Disable(); /* !! */

	    if(kbUn->kbu_readPos == KBBase->kb_writePos)
	    {
		ioreq->io_Flags &= ~IOF_QUICK;
		request_queued = TRUE;
		D(bug("kbd: No keypresses, putting request in queue\n"));

		kbUn->kbu_flags |= KBUF_PENDING;
		AddTail((struct List *)&KBBase->kb_PendingQueue,
			(struct Node *)ioreq);
	    } else {
		D(bug("kbd: Events ready\n"));

		writeEvents(ioreq, KBBase);
	    }

	    Enable();

	    break;

	/* nlorentz: This command lets the keyboard.device initialize
	   the HIDD to use. It must be done this way, because
	   HIDDs might be loaded from disk, and keyboard.device is
	   inited before DOS is up and running.
	   The name of the HIDD class is in
	   ioStd(rew)->io_Data. Note that maybe we should
	   receive a pointer to an already created HIDD object instead.
	   Also note that the below is just a temporary hack, should
	   probably use IRQ HIDD instead to set the IRQ handler.
	*/   

	case CMD_HIDDINIT: {
            struct TagItem tags[] =
	    {
		{ aHidd_Kbd_IrqHandler		, (IPTR)keyCallback	},
		{ aHidd_Kbd_IrqHandlerData	, (IPTR)KBBase 		},
		{ TAG_DONE						}
	    };
	    
	    D(bug("keyboard.device: Received CMD_HIDDINIT, hiddname=\"%s\"\n"
		    , (STRPTR)ioStd(ioreq)->io_Data ));

	    if (KBBase->kb_Hidd != NULL)
		OOP_DisposeObject(KBBase->kb_Hidd);
	    KBBase->kb_Hidd = OOP_NewObject(NULL, (STRPTR)ioStd(ioreq)->io_Data, tags);
	    if (!KBBase->kb_Hidd)
	    {
		D(bug("keyboard.device: Failed to open hidd.\n"));
		ioreq->io_Error = IOERR_OPENFAIL;
	    }
	    break; }

	default:
	    ioreq->io_Error = IOERR_NOCMD;
	    break;
	    
    } /* switch (ioreq->io_Command) */
    
    /* If the quick bit is not set, send the message to the port */
    if(!(ioreq->io_Flags & IOF_QUICK) && !request_queued)
	ReplyMsg(&ioreq->io_Message);
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

static BOOL writeEvents(struct IORequest *ioreq, struct KeyboardBase *KBBase)
{
    int    nEvents;             /* Number of struct InputEvent:s that there is
				   room for in memory pointed to by io_Data */
    UWORD  code;                /* Value of current keycode */
    UWORD  trueCode;            /* Code without possible keypress addition */
    int    i;			/* Loop variable */
    struct InputEvent *event;   /* Temporary variable */
    BOOL   moreevents = TRUE;
    BOOL   activate_resetphase = FALSE;
    
    event = (struct InputEvent *)(ioStd(ioreq)->io_Data);

    /* Number of InputEvents we can store in io_Data */
    /* be careful, the io_Length might be the size of the InputEvent structure,
       but it can be that the ALIGN() returns a larger size and then nEvents would
       be 0.
     */
     
    nEvents = NUM_INPUTEVENTS(ioStd(ioreq)->io_Length);
    
    if(nEvents == 0)
    {
	ioreq->io_Error = IOERR_BADLENGTH;
	D(bug("kbd: Bad length\n"));
	return TRUE;
    }

    D(bug("NEvents = %i", nEvents));
    
    ioreq->io_Error = 0;
    
    for(i = 0; i < nEvents; i++)
    {
	/* Update eventpointer -- this must be done here as I must set
	   ie_NextEvent to NULL if there are no more keys in the buffer. */
	if(i != 0)
	    event = event->ie_NextEvent;

	code = KBBase->kb_keyBuffer[kbUn->kbu_readPos++];
	
	if(kbUn->kbu_readPos == KB_BUFFERSIZE)
	    kbUn->kbu_readPos = 0;
	
	trueCode = code & AMIGAKEYMASK;

	if(isQualifier(code) == TRUE)
	{
	    
	    /* Key released ? ... */
	    if(code & KEYUPMASK)
	    {
    	    #if 1
    	    	/* stegerg: on PC keyboards caps lock also generates up events */
    	    	if (trueCode != AKC_CAPS_LOCK)
    	    #endif
		kbUn->kbu_Qualifiers &= ~(1 << (trueCode - AKC_QUALIFIERS_FIRST));
	    }
	    else  /* ... or pressed? */
	    {
    	    	if (trueCode == AKC_CAPS_LOCK)
		{
		    kbUn->kbu_Qualifiers ^= IEQUALIFIER_CAPSLOCK;
		}
		else
		{
		    kbUn->kbu_Qualifiers |= 1 << (trueCode - AKC_QUALIFIERS_FIRST); 
		}
	    }
	}
	
	D(bug("kbd: Adding event of code %d\n", code));
	
	event->ie_Class 		= IECLASS_RAWKEY;
	event->ie_SubClass 		= 0;
	event->ie_Code 			= code;
	event->ie_Qualifier 		= kbUn->kbu_Qualifiers;
	event->ie_Qualifier 		|= isNumericPad(trueCode) ? IEQUALIFIER_NUMERICPAD : 0;
	event->ie_Prev1DownCode 	= (UBYTE)(kbUn->kbu_LastCode & 0xff);
	event->ie_Prev1DownQual 	= kbUn->kbu_LastQuals;
	event->ie_Prev2DownCode 	= (UBYTE)(kbUn->kbu_LastLastCode & 0xff);
	event->ie_Prev2DownQual 	= kbUn->kbu_LastLastQuals;
	event->ie_TimeStamp.tv_secs 	= 0;
	event->ie_TimeStamp.tv_micro	= 0;
	
	/* Update list of previous states for dead key handling */
	
	if (!(code & IECODE_UP_PREFIX) && !isQualifier(code))
	{
	    kbUn->kbu_LastLastCode  = kbUn->kbu_LastCode;
	    kbUn->kbu_LastLastQuals = kbUn->kbu_LastQuals;
	    kbUn->kbu_LastCode      = code;
	    kbUn->kbu_LastQuals     = (UBYTE)(kbUn->kbu_Qualifiers & 0xff);
	}

	if(code == 0x78) activate_resetphase = TRUE;

	/* No more keys in buffer? */
	if(kbUn->kbu_readPos == KBBase->kb_writePos)
	{
	    moreevents = FALSE;
	    break;
	}
	
    	event->ie_NextEvent = NEXT_INPUTEVENT(event);
	
    }

    D(bug("Done writing events!"));
    event->ie_NextEvent = NULL;

    if(activate_resetphase && !KBBase->kb_ResetPhase)
    {
	struct Interrupt *node;

    	KBBase->kb_ResetPhase = TRUE;
	
	if(!IsListEmpty(&KBBase->kb_ResetHandlerList))
	{
	    /* We may want to install a timer here so that ColdReboot()
	       will eventually be called even if a reset handler hang. */
	    ForeachNode(&KBBase->kb_ResetHandlerList, node)
	    {
		/* We may be inside an interrupt when we come here. Maybe
		   we shall use some other technique? */
		AROS_UFC3(VOID, node->is_Code,
			  AROS_UFCA(APTR, node->is_Data, A1),
			  AROS_UFCA(APTR, node->is_Code, A5),
			  AROS_UFCA(struct ExecBase *, SysBase, A6));
	    }
	}
	else
	{
	    ColdReboot();	/* Bye bye AROS */
	}
    }
    
    return moreevents;
}

/****************************************************************************************/

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IORequest *,    ioreq,  A1),
	  struct KeyboardBase *, KBBase, 6,  Keyboard)
{
    AROS_LIBFUNC_INIT

    LONG ret = -1;
    
    Disable();
    if(kbUn->kbu_flags & KBUF_PENDING)
    {
        if (ioreq->io_Message.mn_Node.ln_Type == NT_MESSAGE)
	{
	    Remove((struct Node *)ioreq);
	    ReplyMsg(&ioreq->io_Message);
	    
	    ioreq->io_Error = IOERR_ABORTED;
	    
	    if (IsListEmpty(&KBBase->kb_PendingQueue)) kbUn->kbu_flags &= ~KBUF_PENDING;
	    
	    ret = 0;
	}
    }
    Enable();
    
    return ret;

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

#define  CORRECT(x)        (((x) & AMIGAKEYMASK) | (((x) & NOTAMIGAKEYMASK) >> 1))
#define  BVBITCLEAR(x, y)  ((y)[(x) / (sizeof(UBYTE)*8)] &= ~(1 << ((x) & (sizeof(UBYTE) - 1))))
#define  BVBITSET(x, y)    ((y)[(x) / (sizeof(UBYTE)*8)] |=  (1 << ((x) & (sizeof(UBYTE) - 1))))

/****************************************************************************************/

#if 0

/****************************************************************************************/

/*
   78      Reset warning.
   F9      Last key code bad, next key is same code retransmitted
   FA      Keyboard key buffer overflow
   FC      Keyboard self-test fail.
   FD      Initiate power-up key stream (for keys held or stuck at
           power on)
   FE      Terminate power-up key stream.
   */

#include  <hardware/cia.h>

/****************************************************************************************/

BOOL HIDDM_initKeyboard(struct KeyboardHIDD *kh)
{
    /* What should be done here? My guess is that we need the IRQ.hidd
       before I can complete this function.
       Presume that an IRQ.hidd exists, and that it has a method
       HIDDV_addServerItem(ULONG level, BOOL (*)checkFunc) that adds an
       interrupt server (sort of) to the real interrupt server at level
       'level'. In the case of the keyboard.hidd, this would be level 6
       (hardware wise) but probably something else in this context.
           Then the code would look something like: */


    kh->kh_irqhidd = FindHidd("IRQ.hidd");

    if(kh->irqhidd == NULL)
   	return FALSE;

    HIDDV_addServerItem(irqhidd_keyboard, checkKBint);
}

/****************************************************************************************/

#endif

/****************************************************************************************/

VOID keyCallback(struct KeyboardBase *KBBase, UWORD keyCode)
{
    D(bug("keyCallBack(KBBase=%p, keyCode=%d)\n"
    		, KBBase, keyCode));
		
    Disable();
    
    KBBase->kb_keyBuffer[(KBBase->kb_writePos)++] = keyCode;

    D(bug("Wrote to buffer\n"));
    
    if(KBBase->kb_writePos == KB_BUFFERSIZE)
	KBBase->kb_writePos = 0;

    if (CORRECT(keyCode) < KB_MAXKEYS)
    {
	if(keyCode & KEYUPMASK)
	    BVBITCLEAR(CORRECT(keyCode), KBBase->kb_Matrix);
	else
	    BVBITSET(CORRECT(keyCode), KBBase->kb_Matrix);
    	D(bug("Wrote to matrix\n"));
    }
    else
    {
    	D(bug("Keycode value too high. Is %d. Should be < %d\n", CORRECT(keyCode), KB_MAXKEYS));
    }
    
    if(!IsListEmpty(&KBBase->kb_PendingQueue))
    {    
#if 0
	D(bug("doing software irq\n"));
	Cause(&KBBase->kb_Interrupt);
#else
	AROS_UFC3(VOID, kbdSendQueuedEvents,
	    AROS_UFCA(struct KeyboardBase *, KBBase  , A1),
	    AROS_UFCA(APTR                 , NULL, A5),
	    AROS_UFCA(struct ExecBase *    , SysBase , A6));
#endif
    }
    
    Enable();
}

/****************************************************************************************/

#undef  BVBITSET
#undef  BVBITCLEAR
#undef  CORRECT

/****************************************************************************************/

/* Software interrupt to be called when keys are received */

#undef SysBase

AROS_UFH3(VOID, kbdSendQueuedEvents,
    AROS_UFHA(struct KeyboardBase *, KBBase, A1),
    AROS_UFHA(APTR, thisfunc, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    /* Broadcast keys */
    struct IORequest *ioreq, *nextnode;
    struct List *pendingList = (struct List *)&KBBase->kb_PendingQueue;
    
    D(bug("Inside software irq\n"));

    ForeachNodeSafe(pendingList, ioreq, nextnode)
    {
        BOOL moreevents;
	
        D(bug("Replying msg: R: %i W: %i\n", kbUn->kbu_readPos,
	      KBBase->kb_writePos));
	
	moreevents = writeEvents(ioreq, KBBase);
	
	Remove((struct Node *)ioreq);
 	ReplyMsg((struct Message *)&ioreq->io_Message);
	
	if (!moreevents) break;
    }

    if (IsListEmpty(pendingList)) kbUn->kbu_flags &= ~KBUF_PENDING;

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

static const char end = 0;

/****************************************************************************************/

