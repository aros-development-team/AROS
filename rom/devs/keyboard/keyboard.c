/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Keyboard device
    Lang: English
*/

/* HISTORY:  12.04.98  SDuvan  Began work
             xx.06.98  SDuvan  Fixes, added amigakeyboard.HIDD
 */

#define AROS_ALMOST_COMPATIBLE 1
#include <exec/resident.h>
#include <exec/interrupts.h>
#include <devices/inputevent.h>
#include <devices/keyboard.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/lists.h>
#include <aros/libcall.h>
#include "abstractkeycodes.h"
#include "keyboard_intern.h"

#ifdef  __GNUC__
#include "keyboard_gcc.h"
#endif

#define DEBUG 0
#include <aros/debug.h>


#define ioStd(x)  ((struct IOStdReq *)x)
#define kbUn      ((struct KBUnit *)(ioreq->io_Unit))

#define min(a,b)  ((a) < (b)) ? (a) : (b)
#define ALIGN(x)  ((((x) + (__AROS_STRUCTURE_ALIGNMENT - 1)) / __AROS_STRUCTURE_ALIGNMENT) * __AROS_STRUCTURE_ALIGNMENT)

#define isQualifier(x)   (((x) >= AKC_QUALIFIERS_FIRST) && ((x) <= AKC_QUALIFIERS_LAST))

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

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable;

struct KeyboardBase *AROS_SLIB_ENTRY(init, Keyboard)();
void AROS_SLIB_ENTRY(open, Keyboard)();
BPTR AROS_SLIB_ENTRY(close, Keyboard)();
BPTR AROS_SLIB_ENTRY(expunge, Keyboard)();
int  AROS_SLIB_ENTRY(null, Keyboard)();
void AROS_SLIB_ENTRY(beginio, Keyboard)();
LONG AROS_SLIB_ENTRY(abortio, Keyboard)();

static const char end;

int AROS_SLIB_ENTRY(entry, Keyboard)(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

const struct Resident Keyboard_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Keyboard_resident,
    (APTR)&end,
    RTF_AUTOINIT|RTF_COLDSTART,
    41,
    NT_DEVICE,
    45,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[] = "keyboard.device";

static const char version[] = "$VER: keyboard.device 41.0 (13.4.98)\r\n";

static const APTR inittabl[4] =
{
    (APTR)sizeof(struct KeyboardBase),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init, Keyboard)
};

static void *const functable[] =
{
    &AROS_SLIB_ENTRY(open, Keyboard),
    &AROS_SLIB_ENTRY(close, Keyboard),
    &AROS_SLIB_ENTRY(expunge, Keyboard),
    &AROS_SLIB_ENTRY(null, Keyboard),
    &AROS_SLIB_ENTRY(beginio, Keyboard),
    &AROS_SLIB_ENTRY(abortio, Keyboard),
    (void *)-1
};


AROS_LH2(struct KeyboardBase *,  init,
 AROS_LHA(struct KeyboardBase *, KBBase, D0),
 AROS_LHA(BPTR,         segList, A0),
	  struct ExecBase *, sysBase, 0, Keyboard)
{
    AROS_LIBFUNC_INIT


    /* Store arguments */
    KBBase->kb_sysBase = sysBase;
    KBBase->kb_seglist = segList;
    
    InitSemaphore(&KBBase->kb_QueueLock);
    NEWLIST(&KBBase->kb_ResetHandlerList);
    NEWLIST(&KBBase->kb_PendingQueue);
    
    return KBBase;
    AROS_LIBFUNC_EXIT
}


AROS_LH3(void, open,
 AROS_LHA(struct IORequest *, ioreq, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct KeyboardBase *, KBBase, 1, Keyboard)
{
    AROS_LIBFUNC_INIT

    /* Keep compiler happy */
    unitnum = 0;
    flags   = 0;

    if(KBBase->kb_keyBuffer == NULL)
    {
	KBBase->kb_keyBuffer = AllocMem(sizeof(UWORD)*KB_BUFFERSIZE, MEMF_ANY);
    }

    /* No memory for key buffer? */
    if(KBBase->kb_keyBuffer == NULL)
    {
	ioreq->io_Error = IOERR_OPENFAIL;
	return;
    }

    if((ioreq->io_Unit = AllocMem(sizeof(KBUnit), MEMF_CLEAR)) == NULL)
    {
	ioreq->io_Error = IOERR_OPENFAIL;
	return;
    }

    if(!KBBase->kb_LowLevelBase)
    {
	KBBase->kb_LowLevelBase = OpenLibrary("lowlevel.library", 41);

	/* Install our own keyboard handler if opened for the first time */
/*	if(KBBase->kb_LowLevelBase)
	    AddKBInt(keyCallback, KBBase); */
    }

    if(!KBBase->kb_LowLevelBase)
    {
	ioreq->io_Error = IOERR_OPENFAIL;
	return;
    }

    /* I have one more opener. */
    KBBase->kb_device.dd_Library.lib_OpenCnt++;

    AROS_LIBFUNC_EXIT
}


AROS_LH1(BPTR, close,
 AROS_LHA(struct IORequest *,    ioreq,  A1),
	  struct KeyboardBase *, KBBase, 2, Keyboard)
{
    AROS_LIBFUNC_INIT

    FreeMem(ioreq->io_Unit, sizeof(KBUnit));

    /* Let any following attemps to use the device crash hard. */
    ioreq->io_Device = (struct Device *)-1;

    KBBase->kb_device.dd_Library.lib_OpenCnt--;
    if(KBBase->kb_device.dd_Library.lib_OpenCnt == 0)
	expunge();

    return 0;
    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, expunge, struct KeyboardBase *, KBBase, 3, Keyboard)
{
    AROS_LIBFUNC_INIT

    /* Do not expunge the device. Set the delayed expunge flag and return. */

    /* TODO: Deallocate key buffer. */
    KBBase->kb_device.dd_Library.lib_Flags |= LIBF_DELEXP;
    return 0;
    AROS_LIBFUNC_EXIT
}


AROS_LH0I(int, null, struct KeyboardBase *, KBBase, 4, Keyboard)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}


AROS_LH1(void, beginio,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct KeyboardBase *, KBBase, 5, Keyboard)
{
    AROS_LIBFUNC_INIT

    UWORD  code;                     /* Value of current keycode */
    UWORD  trueCode;                 /* Code without possible keypress addition */
    int    i;			     /* Loop variable */
    int    nEvents;                  /* Number of struct InputEvent that there is
					room for in memory pointed to by io_Data */
    struct InputEvent *event;        /* Temporary variable */
    
    
    D(bug("id: beginio(ioreq=%p)\n", ioreq));
    
    /* WaitIO will look into this */
    ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;
    
    switch (ioreq->io_Command)
    {
    case CMD_CLEAR:
	kbUn->kbu_readPos = KBBase->kb_writePos;
	break;
	
    case KBD_ADDRESETHANDLER:
	Disable();
	Enqueue((struct List *)(&KBBase->kb_ResetHandlerList),
		(struct Node *)(ioStd(ioreq)->io_Data));
	Enable();
	break;
	
    case KBD_REMRESETHANDLER:
	Disable();
	Remove((struct Node *)(ioStd(ioreq)->io_Data));
	Enable();
	break;
	
    case KBD_RESETHANDLERDONE:
	
	/* We don't want any phony resets. */
	if(KBBase->kb_ResetPhase == TRUE)
	{
	    if(--(KBBase->kb_nHandlers) == 0)
		/* ResetSystem();  Function in Aros.library? */
		;
	}
	else
	{
	    /* There is no good (defined) IOERR to return in this situation */
	    ioreq->io_Error = IOERR_NOCMD;
	}
	break;
	
    case KBD_READMATRIX:
	ioStd(ioreq)->io_Actual = min(KB_MATRIXSIZE, ioStd(ioreq)->io_Length);
	CopyMem(KBBase->kb_Matrix, ioStd(ioreq)->io_Data, ioStd(ioreq)->io_Actual);
	break;
	
    case KBD_READEVENT:
	
	/* TODO */
	/* Check for reset... via keybuffer or via HIDD? */
	/* if(bufferkey == 0x78) ... */
	
	/* Is it OK to presuppose that __AROS_STRUCTURE_ALIGNMENT is 2^n ? */
	/* if((&ioStd(ioreq)->io_Data != ALIGN(&iostd(ioreq)->io_Data))
	   well, this should actually be more like
	   if((&ioStd(ioreq)->io_Data) < (AROS_PTR_MAX - __AROS_STRUCTURE_ALIGNMENT) ?
	   (&ioStd(ioreq)->io_Data != ALIGN(&ioStd(ioreq)->io_Data)) : &ioStd(ioreq)->io_Data != ... */
	
	/* Hmm... (int) */
	if(((IPTR)(&(ioStd(ioreq)->io_Data)) & (__AROS_STRUCTURE_ALIGNMENT - 1)) != 0)
	{
	    ioreq->io_Error = IOERR_BADADDRESS;
	    break;
	}
	
	/* Number of InputEvents we can store in io_Data */
	nEvents = (ioStd(ioreq)->io_Length)/ALIGN(sizeof(struct InputEvent));
	if(nEvents == 0 && ioStd(ioreq)->io_Length < sizeof(struct InputEvent))
	{
	    ioreq->io_Error = IOERR_BADLENGTH;
	    break;
	}
	else
	{
	    nEvents = 1;
	}
	
	if(kbUn->kbu_readPos == KBBase->kb_writePos)
	{
	    ioreq->io_Flags &= ~IOF_QUICK;
	    ObtainSemaphore(&KBBase->kb_QueueLock);
	    kbUn->kbu_flags |= KBUF_PENDING;
	    AddTail((struct List *)&KBBase->kb_PendingQueue, (struct Node *)ioreq);
	    ReleaseSemaphore(&KBBase->kb_QueueLock);
	    break;
	}
	
	event = (struct InputEvent *)(ioStd(ioreq)->io_Data);
	
	for(i = 0; i < nEvents; i++)
	{
	    code = KBBase->kb_keyBuffer[kbUn->kbu_readPos++];
	    
	    if(isQualifier(code) == TRUE)
	    {
		trueCode = code & AMIGAKEYMASK;
		
		/* Key released ? ... */
		if(code & KEYUPMASK)
		{
		    kbUn->kbu_Qualifiers |= 1 << (trueCode - AKC_QUALIFIERS_FIRST);
		}
		else  /* ... or pressed? */
		{
		    /* No CAPS_LOCK releases are reported */
		    if(trueCode == AKC_CAPS_LOCK)
		    {
			kbUn->kbu_Qualifiers ^= IEQUALIFIER_CAPSLOCK;
		    }
		    else
		    {
			kbUn->kbu_Qualifiers &= ~(1 << (trueCode - AKC_QUALIFIERS_FIRST));
		    }
		}
	    }
	    
	    event->ie_Class = IECLASS_RAWKEY;
	    event->ie_SubClass = 0;
	    event->ie_Code = code;
	    event->ie_Qualifier = kbUn->kbu_Qualifiers;
	    event->ie_Qualifier |= isNumericPad(trueCode) ? IEQUALIFIER_NUMERICPAD : 0;
	    event->ie_Prev1DownCode = (UBYTE)(kbUn->kbu_LastCode & 0xff);
	    event->ie_Prev1DownQual = kbUn->kbu_LastQuals;
	    event->ie_Prev2DownCode = (UBYTE)(kbUn->kbu_LastLastCode & 0xff);
	    event->ie_Prev2DownQual = kbUn->kbu_LastLastQuals;
	    event->ie_X = 0;
	    event->ie_Y = 0;
	    event->ie_TimeStamp.tv_secs = 0;
	    event->ie_TimeStamp.tv_micro = 0;
	    
	    /* Update list of previous states for dead key handling */
	    kbUn->kbu_LastLastCode  = kbUn->kbu_LastCode;
	    kbUn->kbu_LastLastQuals = kbUn->kbu_LastQuals;
	    kbUn->kbu_LastCode      = code;
	    kbUn->kbu_LastQuals     = (UBYTE)(kbUn->kbu_Qualifiers & 0xff);
	    
	    /* No more keys in buffer? */
	    if(kbUn->kbu_readPos == KBBase->kb_writePos)
		break;
	    
	    event->ie_NextEvent = (struct InputEvent *) ((UBYTE *)event
				 + ALIGN(sizeof(struct InputEvent)));
	}
	event->ie_NextEvent = NULL;
	
	break;
	
    default:
	ioreq->io_Error = IOERR_NOCMD;
	break;
    }
    
    /* If the quick bit is not set, send the message to the port */
    if(!(ioreq->io_Flags & IOF_QUICK))
	ReplyMsg(&ioreq->io_Message);
    
    AROS_LIBFUNC_EXIT
}


AROS_LH1(LONG, abortio,
 AROS_LHA(struct IORequest *,    ioreq,  A1),
	  struct KeyboardBase *, KBBase, 6,  Keyboard)
{
    AROS_LIBFUNC_INIT

    if(kbUn->kbu_flags & KBUF_PENDING)
    {
	ObtainSemaphore(&KBBase->kb_QueueLock);
	Remove((struct Node *)ioreq);		/* Correct? Interference? */
	ReplyMsg(&ioreq->io_Message);
	kbUn->kbu_flags &= ~KBUF_PENDING;
	ReleaseSemaphore(&KBBase->kb_QueueLock);
    }
    return 0;

    AROS_LIBFUNC_EXIT
}


#define  CORRECT(x)        ((x) & AMIGAKEYMASK) || (((x) & NOTAMIGAKEYMASK) >> 1)
#define  BVBITCLEAR(x, y)  ((y)[(x) / sizeof(UBYTE)] &= ~(1 << ((x) & (sizeof(UBYTE) - 1))))
#define  BVBITSET(x, y)    ((y)[(x) / sizeof(UBYTE)] |=  (1 << ((x) & (sizeof(UBYTE) - 1))))


#if 0

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

#endif


VOID keyCallback(UWORD keyCode, struct KeyboardBase *KBBase)
{
    KBBase->kb_keyBuffer[(KBBase->kb_writePos)++] = keyCode;

    if(KBBase->kb_writePos == KB_BUFFERSIZE)
	KBBase->kb_writePos = 0;

    if(keyCode & KEYUPMASK)
	BVBITCLEAR(CORRECT(keyCode), KBBase->kb_Matrix);
    else
	BVBITSET(CORRECT(keyCode), KBBase->kb_Matrix);

    if(!IsListEmpty(&KBBase->kb_PendingQueue))
	Cause(&KBBase->kb_Interrupt);
}


#undef  BVBITSET
#undef  BVBITCLEAR
#undef  CORRECT


VOID keyBroadCast(struct List *pendingList)
{
    struct KeyboardBase *KBBase;	/* Temporary until solution is decided */

    struct IORequest *ioreq;

    ObtainSemaphore(&KBBase->kb_QueueLock);
    ForeachNode(pendingList, ioreq)
    {
	ReplyMsg((struct Message *)&ioreq->io_Message);
	Remove((struct Node *)ioreq);
	kbUn->kbu_flags &= ~KBUF_PENDING;
    }
    ReleaseSemaphore(&KBBase->kb_QueueLock);
}

static const char end = 0;

