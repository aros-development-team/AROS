/*
    Copyright © 1998-2004, The AROS Development Team. All rights reserved. 
    $Id$

    Clipboard device.
*/

/****************************************************************************************/

#define AROS_ALMOST_COMPATIBLE 1
#include <exec/resident.h>
#include <devices/clipboard.h>
#include <devices/newstyle.h>
#include <exec/io.h>
#include <exec/initializers.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/alib.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/lists.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include "clipboard_intern.h"
#define DEBUG 0
#include <aros/debug.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef    __GNUC__
#include  "clipboard_gcc.h"
#endif

#ifdef __MORPHOS__
    unsigned long __abox__ = 1;
#endif

/****************************************************************************************/
#ifndef __MORPHOS__
#define NEWSTYLE_DEVICE 1
#endif

#define ioClip(x)	((struct IOClipReq *)x)
#define min(x,y)	(((x) < (y)) ? (x) : (y))

#define CBUn		(((struct ClipboardUnit *)ioreq->io_Unit))

/****************************************************************************************/

static void readCb(struct IORequest *ioreq, struct ClipboardBase *CBBase);
static void writeCb(struct IORequest *ioreq, struct ClipboardBase *CBBase);
static void updateCb(struct IORequest *ioreq, struct ClipboardBase *CBBase);

/****************************************************************************************/

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable;

struct  ClipboardBase  *AROS_SLIB_ENTRY(init, Clipboard)();
void  AROS_SLIB_ENTRY(open, Clipboard)();
BPTR  AROS_SLIB_ENTRY(close, Clipboard)();
BPTR  AROS_SLIB_ENTRY(expunge, Clipboard)();
int   AROS_SLIB_ENTRY(null, Clipboard)();
void  AROS_SLIB_ENTRY(beginio, Clipboard)();
LONG  AROS_SLIB_ENTRY(abortio, Clipboard)();

static const char end;

/****************************************************************************************/

int AROS_SLIB_ENTRY(entry, Clipboard)(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

const struct Resident Clipboard_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Clipboard_resident,
    (APTR)&end,
#ifdef __MORPHOS__
    RTF_PPC | RTF_EXTENDED | RTF_AUTOINIT,
#else
    RTF_AUTOINIT|RTF_COLDSTART,
#endif
    50,
    NT_DEVICE,
    45,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl,
#ifdef __MORPHOS__
    0,	/* Revision */
    NULL /* Tags */
#endif
};

static const char name[] = "clipboard.device";

static const char version[] = "$VER: clipboard.device 50.0 (11.5.2002)\r\n";

static const APTR inittabl[4] =
{
    (APTR)sizeof(struct ClipboardBase),
    (APTR)functable,
#ifdef __MORPHOS__
	NULL,
#else
    (APTR)&datatable,
#endif
    &AROS_SLIB_ENTRY(init, Clipboard)
};

static void *const functable[] =
{
#ifdef __MORPHOS__
    (void *const) FUNCARRAY_32BIT_NATIVE,
#endif
    &AROS_SLIB_ENTRY(open, Clipboard),
    &AROS_SLIB_ENTRY(close, Clipboard),
    &AROS_SLIB_ENTRY(expunge, Clipboard),
    &AROS_SLIB_ENTRY(null, Clipboard),
    &AROS_SLIB_ENTRY(beginio, Clipboard),
    &AROS_SLIB_ENTRY(abortio, Clipboard),
    (void *)-1
};

/****************************************************************************************/

#if NEWSTYLE_DEVICE

static const UWORD SupportedCommands[] =
{
    CMD_READ,
    CMD_WRITE,
    CMD_UPDATE,
    CBD_CHANGEHOOK,
    CBD_POST,
    CBD_CURRENTREADID,
    CBD_CURRENTWRITEID,
    NSCMD_DEVICEQUERY,
    0
};

#endif

/****************************************************************************************/

#ifdef __MORPHOS__
struct ClipboardBase *LIB_init(struct ClipboardBase *CBBase, BPTR segList, struct ExecBase *sysBase)
#else
AROS_UFH3(struct ClipboardBase *,  AROS_SLIB_ENTRY(init,Clipboard),
 AROS_UFHA(struct ClipboardBase *, CBBase, D0),
 AROS_UFHA(BPTR,          segList, A0),
 AROS_UFHA(struct ExecBase *, sysBase, A6)
)
#endif
{
    AROS_USERFUNC_INIT

    /* Store arguments */
    CBBase->cb_sysBase = sysBase;
    CBBase->cb_seglist = segList;

    InitSemaphore(&CBBase->cb_SignalSemaphore);
    NEWLIST(&CBBase->cb_UnitList);
    NEWLIST(&CBBase->cb_HookList);

    return CBBase;

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/
#ifndef __MORPHOS__
/*this trick can't work with MorphOS since we can't rely on varargs to be a linear stream*/

/* Putchar procedure needed by RawDoFmt() */

AROS_UFH2(void, putchr,
    AROS_UFHA(UBYTE,    chr, D0),
    AROS_UFHA(STRPTR *, p,   A3))
{
    AROS_USERFUNC_INIT
    *(*p)++=chr;
    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

void cb_sprintf(struct ClipboardBase *CBBase, UBYTE *buffer,
		UBYTE *format, ...)
{
    RawDoFmt(format, &format+1, (VOID_FUNC)AROS_ASMSYMNAME(putchr), &buffer);
}
#endif


/****************************************************************************************/

AROS_LH3(void, open,
 AROS_LHA(struct IORequest *, ioreq, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct ClipboardBase *, CBBase, 1, Clipboard)
{
    AROS_LIBFUNC_INIT

    BPTR   tempLock = NULL, tempLock2 = NULL, tempLock3 = NULL;
    BOOL   found = FALSE;	   /* Does the unit already exist? */
    struct Node *tempNode;	   /* Temporary variable used to see if a unit
				      already exists */

    /* Keep compiler happy */
    flags = 0;

    D(bug("clipboard.device/open: ioreq 0x%lx unitnum %ld flags 0x%lx\n",ioreq,unitnum,flags));

    CBBase->cb_device.dd_Library.lib_Flags &= ~LIBF_DELEXP;

    if(unitnum > 255)
    {
	D(bug("clipboard.device/open: unitnum too large\n"));
	ioClip(ioreq)->io_Error = IOERR_OPENFAIL;
	return;
    }

#ifndef __MORPHOS__
#warning "You shouldn't check this..only leads to trouble"
    if (ioreq->io_Message.mn_Length < sizeof(struct IOClipReq))
    {
        D(bug("clipboard.device/open: IORequest structure passed to OpenDevice is too small!\n"));
        ioreq->io_Error = IOERR_OPENFAIL;
	return;
    }
#endif

    ObtainSemaphore(&CBBase->cb_SignalSemaphore);

    if(CBBase->cb_DosBase == NULL)
	CBBase->cb_DosBase = OpenLibrary("dos.library", 39);

    if(CBBase->cb_DosBase == NULL)
    {
	ioreq->io_Error = IOERR_OPENFAIL;
	ReleaseSemaphore(&CBBase->cb_SignalSemaphore);
	return;
    }

    if(CBBase->cb_UtilityBase == NULL)
	CBBase->cb_UtilityBase = OpenLibrary("utility.library", 39);

    if(CBBase->cb_UtilityBase == NULL)
    {
	ioreq->io_Error = IOERR_OPENFAIL;
	CloseLibrary(CBBase->cb_DosBase);
	ReleaseSemaphore(&CBBase->cb_SignalSemaphore);
	return;
    }

    /* Set up clipboard directory if we are the first opener */

    if(CBBase->cb_device.dd_Library.lib_OpenCnt == 0)
    {
	D(bug("clipboard.device/Checking for CLIPS:\n"));

	if((tempLock = Lock("CLIPS:", ACCESS_READ)) == NULL)
	{
	    /* CLIPS: is not assigned - revert to ram:Clipboards */

	    D(bug("clipboard.device/CLIPS: not found\n"));
	    D(bug("clipboard.device/Checking for ram:\n"));

	    if((tempLock2 = Lock("ram:", ACCESS_READ)) == NULL)
	    {
		D(bug("clipboard.device/ram: Not found."));
		ioreq->io_Error = IOERR_OPENFAIL;
	    }
	    else
	    {
		D(bug("clipboard.device/Found ram:\n"));
		D(bug("clipboard.device/Checking for ram:clipboards\n"));

		if((tempLock3 = Lock("ram:clipboards", ACCESS_READ)) == NULL)
		{
		    D(bug("clipboard.device/Not found -- creating ram:Clipboards.\n"));
		    if(CreateDir("ram:clipboards") == NULL)
		    {
			D(bug("clipboard.device/can't create clipboards file\n"));
			ioreq->io_Error = IOERR_OPENFAIL;
		    }
		}
	    }
	    CBBase->cb_ClipDir = "ram:clipboards/";
	}
	else
	{
	    D(bug("clipboard.device/Found CLIPS:\n"));
	    CBBase->cb_ClipDir = "CLIPS:";
	}

	/* Release all the locks we have made */
	UnLock(tempLock);
	UnLock(tempLock2);
	UnLock(tempLock3);

	if(ioreq->io_Error != 0)
	{
	    CloseLibrary(CBBase->cb_DosBase);
	    CloseLibrary(CBBase->cb_UtilityBase);
	    ReleaseSemaphore(&CBBase->cb_SignalSemaphore);
	    return;
	}
    }

    ForeachNode(&CBBase->cb_UnitList, tempNode)
    {
	D(bug("clipboard.device/ UnitNode 0x%lx Unit %ld\n",tempNode,tempNode->ln_Type));
	if(tempNode->ln_Type == unitnum)
	{
	    D(bug("clipboard.device/ found UnitNode\n"));
	    found = TRUE;
	    break;
	}
    }

    if(found == FALSE)
    {
	struct MsgPort *replyPort;

	D(bug("clipboard.device/Building unit...\n"));

	if ((ioreq->io_Unit = (struct Unit *)AllocMem(sizeof(struct ClipboardUnit),
						     MEMF_CLEAR | MEMF_PUBLIC)))
	{
	    CBUn->cu_UnitNum = unitnum;
	    CBUn->cu_Node.ln_Type = unitnum;
	    CBUn->cu_PostID = 1;             /* Make sure PostID != ClipID. */
	    CBUn->cu_WriteID = 2;            /* Make sure WriteID != ClipID and != PostID too. */

	    InitSemaphore(&CBUn->cu_UnitLock);

	    /* Construct clipboard unit filename. */
	    if ((CBUn->cu_clipFilename = AllocMem(CBUN_FILENAMELEN, MEMF_ANY)))
	    {
#ifdef __MORPHOS__
		sprintf(CBUn->cu_clipFilename, "%s%u", CBBase->cb_ClipDir,
			unitnum);
#else
		cb_sprintf(CBBase, CBUn->cu_clipFilename, "%s%u", CBBase->cb_ClipDir,
			   unitnum);
#endif

		/* Create the replyport for satisfy messages. Note that SIGB_SINGLE
		   is used as the signal as we can't use CreateMsgPort() as it
		   requires that a signal bit in the calling task is free.
		   There should, ideally, be a function internal_CreateMsgPort()
		   which takes a signal number as input and creates a message port,
		   and this function should be used by both this routine and
		   the real CreateMsgPort() */

		if ((replyPort = (struct MsgPort *)AllocMem(sizeof(struct MsgPort),
				       MEMF_PUBLIC | MEMF_CLEAR)))
		{
		    replyPort->mp_SigBit = SIGB_SINGLE;
		    replyPort->mp_MsgList.lh_Head = (struct Node *)&replyPort->mp_MsgList.lh_Tail;
		    replyPort->mp_MsgList.lh_TailPred = (struct Node *)&replyPort->mp_MsgList.lh_Head;

		    replyPort->mp_Flags = PA_SIGNAL;
		    replyPort->mp_Node.ln_Type = NT_MSGPORT;

		    CBUn->cu_Satisfy.sm_Msg.mn_ReplyPort = replyPort;

		    /* Initialization is done, and everything went OK. Add unit to the
		       list of clipboard units. */
		    Insert((struct List *)&CBBase->cb_UnitList, (struct Node *)CBUn, NULL);

		    /* Check if there is already a clipboard file for this unit existing.
		       If yes, then set WriteID to 1 so that CMD_READing works, and
		       also setup clipSize */

		    if ((CBUn->cu_clipFile = Open(CBUn->cu_clipFilename, MODE_OLDFILE)))
		    {
			if (Seek(CBUn->cu_clipFile, 0, OFFSET_END) != -1)
			{
			    CBUn->cu_clipSize = Seek(CBUn->cu_clipFile, 0, OFFSET_BEGINNING);

			    D(bug("clipboard.device/ <%s> clipsize %ld\n",CBUn->cu_clipFilename,CBUn->cu_clipSize));
			    if (CBUn->cu_clipSize != (ULONG)-1)
			    {
				D(bug("clipboard.device/ WriteID set\n"));
				CBUn->cu_WriteID = 1;
			    }
			}
			Close(CBUn->cu_clipFile);
			CBUn->cu_clipFile = 0;
		    }
		    else
		    {
			D(bug("clipboard.device/no <%s> file\n",CBUn->cu_clipFilename));
		    }
		}
		else
		{
		    D(bug("clipboard.device/Couldn't alloc replyport\n"));
		    ioreq->io_Error = IOERR_OPENFAIL;
		}
	    }
	    else
	    {
		D(bug("clipboard.device/Couldn't alloc filename\n"));
		ioreq->io_Error = IOERR_OPENFAIL;
	    }
	}
	else
	{
	    D(bug("clipboard.device/Couldn't alloc Unit\n"));
	    ioreq->io_Error = IOERR_OPENFAIL;
	}

    }
    else
    {
	ioreq->io_Unit = (void *) tempNode;
    }

    if ((ioreq->io_Error == 0) && CBUn)
    {
    	CBUn->cu_OpenCnt++;

    	/* I have one more opener. */
    	CBBase->cb_device.dd_Library.lib_OpenCnt++;
    }
    else if (CBUn && (found == FALSE))
    {
    	if (CBUn->cu_Satisfy.sm_Msg.mn_ReplyPort)
	{
	    FreeMem(CBUn->cu_Satisfy.sm_Msg.mn_ReplyPort, sizeof(struct MsgPort));
	}
	
	if (CBUn->cu_clipFilename)
	{
	    FreeMem(CBUn->cu_clipFilename, CBUN_FILENAMELEN);
	}

	FreeMem(CBUn, sizeof(struct ClipboardUnit));
    }
    
    ReleaseSemaphore(&CBBase->cb_SignalSemaphore);

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(BPTR, close,
 AROS_LHA(struct IORequest *,     ioreq,  A1),
	  struct ClipboardBase *, CBBase,  2, Clipboard)
{
    AROS_LIBFUNC_INIT

    D(bug("clipboard.device/close:ioreq 0x%lx\n",ioreq));

    /* Let any following attemps to use the device crash hard. */
    ioreq->io_Device = (struct Device *)-1;

    ObtainSemaphore(&CBBase->cb_SignalSemaphore);

    CBUn->cu_OpenCnt--;

    D(bug("clipboard.device/close: unitcnt %ld\n",CBUn->cu_OpenCnt));

    if(CBUn->cu_OpenCnt == 0)
    {
	D(bug("clipboard.device/close: removeunit\n",ioreq));
	Remove((struct Node *)ioreq->io_Unit);
    	FreeMem(CBUn->cu_Satisfy.sm_Msg.mn_ReplyPort, sizeof(struct MsgPort));
	FreeMem(CBUn->cu_clipFilename, CBUN_FILENAMELEN);
	FreeMem(ioreq->io_Unit, sizeof(struct ClipboardUnit));
    }

    ReleaseSemaphore(&CBBase->cb_SignalSemaphore);

    CBBase->cb_device.dd_Library.lib_OpenCnt--;

    if (CBBase->cb_device.dd_Library.lib_OpenCnt == 0)
    {
	if (CBBase->cb_device.dd_Library.lib_Flags & LIBF_DELEXP)
	{
	    return expunge();
	}
    }

    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, expunge, struct ClipboardBase *, CBBase, 3, Clipboard)
{
    AROS_LIBFUNC_INIT

    BPTR ret;	     /* Temporary vaiable to preserve seglist. */

    D(bug("clipboard.device/expunge:\n"));

    if(CBBase->cb_device.dd_Library.lib_OpenCnt != 0)
    {
	/* Do not expunge the device. Set the delayed expunge flag and
	   return. */
	CBBase->cb_device.dd_Library.lib_Flags |= LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the device. Remove it from the list. */
    Remove(&CBBase->cb_device.dd_Library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret = CBBase->cb_seglist;

    CloseLibrary(CBBase->cb_DosBase);
    CloseLibrary(CBBase->cb_UtilityBase);

    /* Free the memory. */
    FreeMem((char *)CBBase-CBBase->cb_device.dd_Library.lib_NegSize,
	    CBBase->cb_device.dd_Library.lib_NegSize
	    + CBBase->cb_device.dd_Library.lib_PosSize);

    D(bug("clipboard.device/expunge: done\n"));
    return ret;

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0I(int, null, struct ClipboardBase *, KBBase, 4, Clipboard)
{
    AROS_LIBFUNC_INIT
    D(bug("clipboard.device/null:\n"));
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(void, beginio,
	 AROS_LHA(struct IORequest *, ioreq, A1),
	 struct ClipboardBase *, CBBase, 5, Clipboard)
{
    AROS_LIBFUNC_INIT

    ioreq->io_Error = 0;

    switch (ioreq->io_Command)
    {
#if NEWSTYLE_DEVICE
        case NSCMD_DEVICEQUERY:
	    if(ioClip(ioreq)->io_Length < ((LONG)OFFSET(NSDeviceQueryResult, SupportedCommands)) + sizeof(UWORD *))
	    {
		ioreq->io_Error = IOERR_BADLENGTH;
	    }
	    else
	    {
		struct NSDeviceQueryResult *d;

		d = (struct NSDeviceQueryResult *)ioClip(ioreq)->io_Data;

		d->DevQueryFormat	 = 0;
		d->SizeAvailable	 = sizeof(struct NSDeviceQueryResult);
		d->DeviceType		 = NSDEVTYPE_CLIPBOARD;
		d->DeviceSubType	 = 0;
		d->SupportedCommands	 = (UWORD *)SupportedCommands;

		ioClip(ioreq)->io_Actual = sizeof(struct NSDeviceQueryResult);
	    }
	    break;
#endif

	case CBD_CHANGEHOOK:

	    D(bug("clipboard.device/Command: CBD_CHANGEHOOK\n"));

	    ObtainSemaphore(&CBBase->cb_SignalSemaphore);

	    /* io_Length is used as a means of specifying if the hook
	       should be added or removed. */
	    switch(ioClip(ioreq)->io_Length)
	    {
	    case 0:
		Remove((struct Node *)(ioClip(ioreq)->io_Data));
		break;

	    case 1:
		Insert((struct List *)&CBBase->cb_HookList,
		       (struct Node *)ioClip(ioreq)->io_Data, NULL);
		break;

	    default:
		ioreq->io_Error = IOERR_BADLENGTH;
		break;
	    }
	    ReleaseSemaphore(&CBBase->cb_SignalSemaphore);
	    break;


	case CMD_WRITE:

	    D(bug("clipboard.device/Command: CMD_WRITE\n"));

	    writeCb(ioreq, CBBase);
	    break;

	case CMD_READ:

	    D(bug("clipboard.device/Command: CMD_READ\n"));

	    /* Get new ID if this is the beginning of a read operation */
	    if(ioClip(ioreq)->io_ClipID == 0)
	    {
		D(bug("clipboard.device/CMD_READ: Trying to get unit lock. Calling ObtainSemaphore.\n"));

		ObtainSemaphore(&CBUn->cu_UnitLock);
		CBUn->cu_ReadID++;
		ioClip(ioreq)->io_ClipID = CBUn->cu_ReadID;

		D(bug("clipboard.device/CMD_READ: Got unit lock.\n"));

		CBUn->cu_clipFile = Open(CBUn->cu_clipFilename, MODE_OLDFILE);

		if(!CBUn->cu_clipFile)
		{
		    ReleaseSemaphore(&CBUn->cu_UnitLock);
		    ioClip(ioreq)->io_ClipID = -1;
		    ioClip(ioreq)->io_Actual = 0;
		    ioClip(ioreq)->io_Error = IOERR_ABORTED;
		    break;
		}
	    }
	    else if(ioClip(ioreq)->io_ClipID != CBUn->cu_ReadID)
	    {
		ioClip(ioreq)->io_Actual = 0;
		ioClip(ioreq)->io_Error = IOERR_ABORTED;
		break;
	    }

	    /* If the last write was actually a POST, we must tell the POSTer to
	       WRITE the clip immediately, and we will wait until he have done
	       so.*/

	    if(CBUn->cu_WriteID == CBUn->cu_PostID)
	    {
		struct Task *me = FindTask(NULL);

		/* If it's the poster reading, we would get a deadlock... The
		   poster should not read if the posted clip may be the active
		   one. That is, he should always check if the posted clip is
		   active using CMD_CURRENTREADID. */

		if(CBUn->cu_Poster == me)
		{
		    ReleaseSemaphore(&CBUn->cu_UnitLock);
		    ioClip(ioreq)->io_ClipID = -1;
		    ioClip(ioreq)->io_Actual = 0;
		    ioClip(ioreq)->io_Error = IOERR_ABORTED;
		    break;
		}


		if (CBUn->cu_PostPort)
		{
		    D(bug("clipboard.device/Command: CMD_READ..notify PostPort 0x%lx\n",
			CBUn->cu_PostPort));

		    /* Make sure WE are signalled. */
		    CBUn->cu_Satisfy.sm_Msg.mn_ReplyPort->mp_SigTask = me;

		    PutMsg(CBUn->cu_PostPort, (struct Message *)&CBUn->cu_Satisfy);
		    WaitPort(CBUn->cu_Satisfy.sm_Msg.mn_ReplyPort);
		}
		else
		{
		    D(bug("clipboard.device/Command: no PostPort\n"));
		}
	    }

	    readCb(ioreq, CBBase);

	    break;


	case CMD_UPDATE:
	    D(bug("clipboard.device/Command: CMD_UPDATE\n"));

            updateCb(ioreq, CBBase);
	    break;


	case CBD_POST:
	    D(bug("clipboard.device/Command: CBD_POST\n"));
	    ObtainSemaphore(&CBUn->cu_UnitLock);

	    CBUn->cu_WriteID++;
	    CBUn->cu_PostID = CBUn->cu_WriteID;
	    CBUn->cu_PostPort = (struct MsgPort *)ioClip(ioreq)->io_Data;
	    CBUn->cu_Poster = FindTask(NULL);

	    ReleaseSemaphore(&CBUn->cu_UnitLock);
	    break;


	case CBD_CURRENTREADID:
	    D(bug("clipboard.device/Command: CBD_CURRENTREADID\n"));
	    ioClip(ioreq)->io_ClipID = CBUn->cu_PostID;
	    break;


	case CBD_CURRENTWRITEID:
	    D(bug("clipboard.device/Command: CBD_CURRENTWRITEID\n"));
	    ioClip(ioreq)->io_ClipID = CBUn->cu_WriteID;
	    break;


	default:
	    D(bug("clipboard.device/Command: <UNKNOWN> (%d = 0x%x)\n", ioreq->io_Command));
	    ioreq->io_Error = IOERR_NOCMD;
	    break;

    } /* switch (ioreq->io_Command) */

    /* If the quick bit is not set, send the message to the port */
    if(!(ioreq->io_Flags & IOF_QUICK))
	ReplyMsg(&ioreq->io_Message);

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IORequest *,      ioreq, A1),
	  struct ClipboardBase *, CBBase, 6, Clipboard)
{
    AROS_LIBFUNC_INIT

    D(bug("clipboard.device/abortio: ioreq 0x%lx\n",ioreq));
    /* Nothing to abort */
    return 0;

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

static void readCb(struct IORequest *ioreq, struct ClipboardBase *CBBase)
{
    /* Is there anything to be read? */
    if(CBUn->cu_WriteID == 0)
    {
	D(bug("clipboard.device/readcb: nothing to read. setting IOERR_ABORTED as error\n"));
	Close(CBUn->cu_clipFile);
	CBUn->cu_clipFile = 0;
	ReleaseSemaphore(&CBUn->cu_UnitLock);
	ioClip(ioreq)->io_Error = IOERR_ABORTED;
	ioClip(ioreq)->io_Actual = 0;
	ioClip(ioreq)->io_ClipID = -1;
	return;
    }

    if(ioClip(ioreq)->io_Offset >= CBUn->cu_clipSize)
    {
	D(bug("clipboard.device/readCb: detected \"end of file\". Closing clipfile and releasing semaphore\n"));
	Close(CBUn->cu_clipFile);
	CBUn->cu_clipFile = 0;
	ReleaseSemaphore(&CBUn->cu_UnitLock);
	ioClip(ioreq)->io_Actual = 0;
	ioClip(ioreq)->io_ClipID = -1;
	return;
    }

    if (ioClip(ioreq)->io_Data == NULL)
    {
	ioClip(ioreq)->io_Offset += ioClip(ioreq)->io_Length;
	ioClip(ioreq)->io_Actual = ioClip(ioreq)->io_Length;
    }
    else
    {
	D(bug("clipboard.device/readCb: Doing read Seek() at offset %d.\n",
	      ioClip(ioreq)->io_Offset));

	Seek(CBUn->cu_clipFile, ioClip(ioreq)->io_Offset, OFFSET_BEGINNING);

	ioClip(ioreq)->io_Actual = Read(CBUn->cu_clipFile, ioClip(ioreq)->io_Data,
					ioClip(ioreq)->io_Length);

	D(bug("clipboard.device/readCb: Did Read: data length = %d  data = %02x%02x%02x%02x (%c%c%c%c)\n",
	      ioClip(ioreq)->io_Length,
	      ((UBYTE *)ioClip(ioreq)->io_Data)[0],
	      ((UBYTE *)ioClip(ioreq)->io_Data)[1],
	      ((UBYTE *)ioClip(ioreq)->io_Data)[2],
	      ((UBYTE *)ioClip(ioreq)->io_Data)[3],
	      ((UBYTE *)ioClip(ioreq)->io_Data)[0],
	      ((UBYTE *)ioClip(ioreq)->io_Data)[1],
	      ((UBYTE *)ioClip(ioreq)->io_Data)[2],
	      ((UBYTE *)ioClip(ioreq)->io_Data)[3]));

	ioClip(ioreq)->io_Offset += ioClip(ioreq)->io_Actual;

	if (ioClip(ioreq)->io_Actual == 0)
	{
	    Close(CBUn->cu_clipFile);
	    CBUn->cu_clipFile = 0;
	    ReleaseSemaphore(&CBUn->cu_UnitLock);
	    ioClip(ioreq)->io_ClipID = -1;
	}
    }
}

/****************************************************************************************/

static void writeCb(struct IORequest *ioreq, struct ClipboardBase *CBBase)
{
    if(ioClip(ioreq)->io_ClipID == CBUn->cu_WriteID)
    {
	/* Continue the previous write */
    }
    else if(ioClip(ioreq)->io_ClipID == 0)
    {
	/* A new write begins... */

	CBUn->cu_clipSize = 0;

	CBUn->cu_WriteID++;
	ioClip(ioreq)->io_ClipID = CBUn->cu_WriteID;

	D(bug("clipboard.device/writeCb: Trying to get unit lock. Calling ObtainSemaphore\n"));
	ObtainSemaphore(&CBUn->cu_UnitLock);
	D(bug("clipboard.device/writeCb: Got unit lock.\n"));

	if((CBUn->cu_clipFile = Open(CBUn->cu_clipFilename, MODE_NEWFILE)) == NULL)
	{
	    D(bug("clipboard.device/writeCb: Opening clipfile in MODE_NEWFILE failed. Releasing Semaphore\n"));
	    ReleaseSemaphore(&CBUn->cu_UnitLock);
	    ioClip(ioreq)->io_Error = IOERR_ABORTED;
	    ioClip(ioreq)->io_Actual = 0;
	    return;
	}

	D(bug("clipboard.device/writeCb: Opened file %s\n", CBUn->cu_clipFilename));
    }
    else if(ioClip(ioreq)->io_ClipID == CBUn->cu_PostID)
    {
	/* If this is the poster writing, we will not obtain the
	   semaphore as it's being kept by the current reader.
	   Instead, we don't give a damn about any semaphores and
	   continue the operation anyway. This may be done as it's
	   the reader who has called us, and he (clipboard.
	   device) made sure it is OK to change things in the
	   clipboard unit. */

	CBUn->cu_clipSize = 0;

	if((CBUn->cu_clipFile = Open(CBUn->cu_clipFilename, MODE_NEWFILE)) == NULL)
	{
	    D(bug("clipboard.device/writeCb: Opening clipfile in MODE_NEWFILE failed. Releasing Semaphore\n"));
	    ioClip(ioreq)->io_Error = IOERR_ABORTED;
	    ioClip(ioreq)->io_Actual = 0;
	    return;
	}

	D(bug("clipboard.device/writeCb: Opened file %s\n", CBUn->cu_clipFilename));
    }
    else
    {
	/* Error */
	ioClip(ioreq)->io_Error = IOERR_ABORTED;
	ioClip(ioreq)->io_Actual = 0;
	return;
    }

    Seek(CBUn->cu_clipFile, ioClip(ioreq)->io_Offset, OFFSET_BEGINNING);
    if(ioClip(ioreq)->io_Offset > CBUn->cu_clipSize)
    {
	ULONG len = ioClip(ioreq)->io_Offset - CBUn->cu_clipSize;
	UBYTE *buf = AllocVec(len > 1024 ? 1024 : len, MEMF_CLEAR | MEMF_PUBLIC);
	if(buf)
	{
	    while(len)
	    {
		ULONG size = len > 1024 ? 1024 : len;
		Write(CBUn->cu_clipFile, buf, size);
		len -= size;
	    }
	    FreeVec(buf);
	}
    }

    D(bug("clipboard.device/writeCb: Did Seek(), offset = %d\n", ioClip(ioreq)->io_Offset));
    D(bug("clipboard.device/Doing Write: data length = %d  data = %02x%02x%02x%02x (%c%c%c%c)\n",
	ioClip(ioreq)->io_Length,
	((UBYTE *)ioClip(ioreq)->io_Data)[0],
	((UBYTE *)ioClip(ioreq)->io_Data)[1],
	((UBYTE *)ioClip(ioreq)->io_Data)[2],
	((UBYTE *)ioClip(ioreq)->io_Data)[3],
	((UBYTE *)ioClip(ioreq)->io_Data)[0],
	((UBYTE *)ioClip(ioreq)->io_Data)[1],
	((UBYTE *)ioClip(ioreq)->io_Data)[2],
	((UBYTE *)ioClip(ioreq)->io_Data)[3]));

    ioClip(ioreq)->io_Actual = Write(CBUn->cu_clipFile, ioClip(ioreq)->io_Data, ioClip(ioreq)->io_Length);

    if ((LONG)ioClip(ioreq)->io_Actual == -1)
    {
	D(bug("clipboard.device/writeCb: write failed\n"));
	Close(CBUn->cu_clipFile);
	CBUn->cu_clipFile = 0;
	if(ioClip(ioreq)->io_ClipID != CBUn->cu_PostID)
	{
	    D(bug("clipboard.device/writeCb: releasing semaphore\n"));
	    ReleaseSemaphore(&CBUn->cu_UnitLock);
	}
	ioClip(ioreq)->io_Error = IOERR_ABORTED;
	ioClip(ioreq)->io_Actual = 0;
	ioClip(ioreq)->io_ClipID = -1;
    }

    ioClip(ioreq)->io_Offset += ioClip(ioreq)->io_Actual;
    if(ioClip(ioreq)->io_Offset > CBUn->cu_clipSize)
    {
	CBUn->cu_clipSize = ioClip(ioreq)->io_Offset;
    }
}

/****************************************************************************************/

static void updateCb(struct IORequest *ioreq, struct ClipboardBase *CBBase)
{
    D(bug("clipboard.device/updateCb: Closing ClipFile\n"));

    Close(CBUn->cu_clipFile);
    CBUn->cu_clipFile = 0;

    D(bug("clipboard.device/updateCb: Calling monitoring hooks\n"));

    /* If it is the POSTer that indicates he is done, we don't
       release the semaphore as, in case, the semaphore was
       never locked because it was originally locked by the
       reader wanting the (to be) posted clip. As we couldn't
       release the semaphore then as another task then could have
       started to write something to the clipboard, we just do
       nothing. */

    if(CBUn->cu_PostID != ioClip(ioreq)->io_ClipID &&
       CBUn->cu_WriteID == ioClip(ioreq)->io_ClipID)
    {
	D(bug("clipboard.device/updateCb: calling ReleaseSemaphore\n"));

	ReleaseSemaphore(&CBUn->cu_UnitLock);
    }

    /* Call monitoring hooks. */
    ObtainSemaphore(&CBBase->cb_SignalSemaphore);
    {
	struct Node        *tnode;
	struct ClipHookMsg  chmsg;

	chmsg.chm_Type = 0;
	chmsg.chm_ClipID = ioClip(ioreq)->io_ClipID;

	ForeachNode(&CBBase->cb_HookList, tnode)
	{
	    CallHookA((struct Hook *)tnode, CBUn, &chmsg);
	}
    }
    ReleaseSemaphore(&CBBase->cb_SignalSemaphore);

    D(bug("clipboard.device/updateCb: Called monitoring hooks\n"));

    ioClip(ioreq)->io_ClipID = -1;

    D(bug("clipboard.device/updateCb: end of function\n"));

}

/****************************************************************************************/

static const char end = 0;

/****************************************************************************************/
