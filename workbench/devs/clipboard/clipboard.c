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
#include <clib/alib_protos.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/lists.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include "clipboard_intern.h"
#define DEBUG 0
#include <aros/debug.h>
#include <stdlib.h>
#include <stdio.h>

#include LC_LIBDEFS_FILE

/****************************************************************************************/
#ifndef __MORPHOS__
#define NEWSTYLE_DEVICE 1
#endif

#define ioClip(x)	((struct IOClipReq *)x)
#define CBUn		(((struct ClipboardUnit *)ioreq->io_Unit))

#define WRITEBUFSIZE    4096

/****************************************************************************************/

static void readCb(struct IORequest *ioreq, struct ClipboardBase *CBBase);
static void writeCb(struct IORequest *ioreq, struct ClipboardBase *CBBase);
static void updateCb(struct IORequest *ioreq, struct ClipboardBase *CBBase);

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

AROS_SET_LIBFUNC(GM_UNIQUENAME(Init), LIBBASETYPE, CBBase)
{
    AROS_SET_LIBFUNC_INIT

    InitSemaphore(&CBBase->cb_SignalSemaphore);
    NEWLIST(&CBBase->cb_UnitList);
    NEWLIST(&CBBase->cb_HookList);

    return TRUE;

    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/
#ifndef __MORPHOS__

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

#define cb_sprintf(CBBase, buffer, format, ...) \
({ ULONG _args[]={__VA_ARGS__}; APTR bufptr = buffer; RawDoFmt(format, _args, (VOID_FUNC)AROS_ASMSYMNAME(putchr), &bufptr); })

#else

/* NOTE: Use 68k putch so that we don't bork with 68k localelib - Piru */
static const UWORD putch[] = {0x16c0, 0x4e75};

#define cb_sprintf(CBBase, buffer, format, ...) \
({ ULONG _args[]={__VA_ARGS__}; RawDoFmt(format, _args, (void (*)(void)) putch, buffer); })

#endif


/****************************************************************************************/

AROS_SET_OPENDEVFUNC(GM_UNIQUENAME(Open),
		     LIBBASETYPE, CBBase,
		     struct IORequest, ioreq,
		     unitnum,
		     flags
)
{
    AROS_SET_DEVFUNC_INIT

    BPTR   tempLock = 0;
    BOOL   found = FALSE;	   /* Does the unit already exist? */
    struct Node *tempNode;	   /* Temporary variable used to see if a unit
				      already exists */

    D(bug("clipboard.device/open: ioreq 0x%lx unitnum %ld flags 0x%lx\n",ioreq,unitnum,flags));

    if(unitnum > 255)
    {
	D(bug("clipboard.device/open: unitnum too large\n"));
	ioClip(ioreq)->io_Error = IOERR_OPENFAIL;
	return FALSE;
    }

#ifndef __MORPHOS__
#warning "You shouldn't check this..only leads to trouble"
    if (ioreq->io_Message.mn_Length < sizeof(struct IOClipReq))
    {
        D(bug("clipboard.device/open: IORequest structure passed to OpenDevice is too small!\n"));
        ioreq->io_Error = IOERR_OPENFAIL;
	return FALSE;
    }
#endif

    ObtainSemaphore(&CBBase->cb_SignalSemaphore);

    if (!CBBase->cb_DosBase)
    {
	CBBase->cb_DosBase = OpenLibrary("dos.library", 39);
    }

    if (!CBBase->cb_DosBase)
    {
	ioreq->io_Error = IOERR_OPENFAIL;
	ReleaseSemaphore(&CBBase->cb_SignalSemaphore);
	return FALSE;
    }

    if (!CBBase->cb_UtilityBase)
    {
	CBBase->cb_UtilityBase = OpenLibrary("utility.library", 39);
    }

    if (!CBBase->cb_UtilityBase)
    {
	ioreq->io_Error = IOERR_OPENFAIL;
	CloseLibrary(CBBase->cb_DosBase);
	ReleaseSemaphore(&CBBase->cb_SignalSemaphore);
	return FALSE;
    }

    /* Set up clipboard directory if we are the first opener */

    if(CBBase->cb_ClipDir == NULL)
    {
	D(bug("clipboard.device/Checking for CLIPS:\n"));

	if (!(tempLock = Lock("CLIPS:", ACCESS_READ)))
	{
	    /* CLIPS: is not assigned - revert to ram:Clipboards */

	    D(bug("clipboard.device/CLIPS: not found\n"));
	    D(bug("clipboard.device/Checking for ram:\n"));

	    if (!(tempLock = Lock("ram:", ACCESS_READ)))
	    {
		D(bug("clipboard.device/ram: Not found."));
		ioreq->io_Error = IOERR_OPENFAIL;
	    }
	    else
	    {
		D(bug("clipboard.device/Found ram:\n"));
		D(bug("clipboard.device/Checking for ram:clipboards\n"));

		if (!(tempLock = Lock("ram:clipboards", ACCESS_READ)))
		{
		    D(bug("clipboard.device/Not found -- creating ram:Clipboards.\n"));
		    if (!(tempLock = CreateDir("ram:clipboards")))
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

	/* Release the possible lock we have made */
	UnLock(tempLock);

	if (ioreq->io_Error)
	{
	    CloseLibrary(CBBase->cb_DosBase);
	    CloseLibrary(CBBase->cb_UtilityBase);
	    ReleaseSemaphore(&CBBase->cb_SignalSemaphore);
	    return FALSE;
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
	D(bug("clipboard.device/Building unit...\n"));

	if ((ioreq->io_Unit = (struct Unit *)AllocMem(sizeof(struct ClipboardUnit),
						     MEMF_CLEAR | MEMF_PUBLIC)))
	{
	    CBUn->cu_Head.cu_UnitNum = unitnum;
	    CBUn->cu_Head.cu_Node.ln_Type = unitnum;
	    CBUn->cu_PostID = 0;
	    CBUn->cu_WriteID = 0;

	    NEWLIST((struct List*) &CBUn->cu_PostRequesters);
	    InitSemaphore(&CBUn->cu_UnitLock);

	    /* Construct clipboard unit filename. */
	    cb_sprintf(CBBase, CBUn->cu_clipFilename, "%s%lu", (ULONG) CBBase->cb_ClipDir,
		       unitnum);

	    CBUn->cu_Satisfy.sm_Unit = unitnum;
		
	    /* Initialization is done, and everything went OK. Add unit to the
	       list of clipboard units. */
	    ADDHEAD((struct List *)&CBBase->cb_UnitList, (struct Node *)CBUn);

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
	    D(bug("clipboard.device/Couldn't alloc Unit\n"));
	    ioreq->io_Error = IOERR_OPENFAIL;
	}

    }
    else
    {
	ioreq->io_Unit = (struct Unit *)tempNode;
    }

    if ((ioreq->io_Error == 0) && CBUn)
    {
        CBUn->cu_OpenCnt++;
    }
    else if (CBUn && (found == FALSE))
    {
        FreeMem(CBUn, sizeof(struct ClipboardUnit));
    }
    
    ReleaseSemaphore(&CBBase->cb_SignalSemaphore);

    return TRUE;
    
    AROS_SET_DEVFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_CLOSEDEVFUNC(GM_UNIQUENAME(Close),
		      LIBBASETYPE, CBBase,
		      struct IORequest, ioreq
)
{
    AROS_SET_DEVFUNC_INIT

    D(bug("clipboard.device/close:ioreq 0x%lx\n",ioreq));

    /* Let any following attemps to use the device crash hard. */
    ioreq->io_Device = (struct Device *)-1;

    ObtainSemaphore(&CBBase->cb_SignalSemaphore);

    CBUn->cu_OpenCnt--;

    D(bug("clipboard.device/close: unitcnt %ld\n",CBUn->cu_OpenCnt));

    if(CBUn->cu_OpenCnt == 0)
    {
	D(bug("clipboard.device/close: removeunit\n",ioreq));
	REMOVE((struct Node *)ioreq->io_Unit);
	FreeMem(ioreq->io_Unit, sizeof(struct ClipboardUnit));

	/* Let any following attemps to use the device crash hard. */
	ioreq->io_Unit = (struct Unit *) -1;
    }

    ReleaseSemaphore(&CBBase->cb_SignalSemaphore);

    return TRUE;

    AROS_SET_DEVFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_LIBFUNC(GM_UNIQUENAME(Expunge), LIBBASETYPE, CBBase)
{
    AROS_SET_LIBFUNC_INIT

    D(bug("clipboard.device/expunge:\n"));

    CloseLibrary(CBBase->cb_DosBase);
    CloseLibrary(CBBase->cb_UtilityBase);

    return TRUE;

    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

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
		REMOVE((struct Node *)(ioClip(ioreq)->io_Data));
		break;

	    case 1:
		ADDHEAD((struct List *)&CBBase->cb_HookList,
		       (struct Node *)ioClip(ioreq)->io_Data);
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
		D(bug("clipboard.device/CMD_READ: Trying to get unit lock. Calling ObtainSemaphore [me=%08lx].\n", FindTask(NULL)));
		
		ObtainSemaphore(&CBUn->cu_UnitLock);
	    
		D(bug("clipboard.device/CMD_READ: Got unit lock.\n"));

		/* If the last write was actually a POST, we must tell
		   the POSTer to WRITE the clip immediately, and we
		   will wait until he have done so. Then we check
		   again in case somebody managed to sneek in a
		   CBD_POST after the CMD_UPDATE. */

		while(CBUn->cu_WriteID != 0 &&
		      CBUn->cu_WriteID == CBUn->cu_PostID)
		{
		    struct PostRequest pr = {
		        { NULL, NULL },
			FindTask(NULL)
		    };
		    
		    /* Make sure we are signalled. */
		    ADDTAIL((struct List*) &CBUn->cu_PostRequesters, (struct Node*) &pr);
		    
		    /* A poster reading will deadlock that process
		     * until somebody else writes to the
		     * clipboard. AmigaOS behaves exactly the same so
		     * it's ok. It's just plain stupid anyway. */
		    
		    if (CBUn->cu_PostPort)
		    {
			D(bug("clipboard.device/Command: CMD_READ..notify PostPort 0x%lx\n", CBUn->cu_PostPort));

			CBUn->cu_Satisfy.sm_ClipID = CBUn->cu_PostID;
			PutMsg(CBUn->cu_PostPort, (struct Message *)&CBUn->cu_Satisfy);
			CBUn->cu_PostPort = NULL;
		    }
		    else
		    {
			D(bug("clipboard.device/Command: no PostPort [me=%08lx]\n", FindTask(NULL)));
		    }

		    Forbid();
		    ReleaseSemaphore(&CBUn->cu_UnitLock);
		    SetSignal(0, SIGF_SINGLE);
		    Wait(SIGF_SINGLE);
		    Permit();
		    D(bug("Got SIGF_SINGLE [me=%08lx]\n",FindTask(NULL)));
		    ObtainSemaphore(&CBUn->cu_UnitLock);
		    D(bug("Got semaphore[me=%08lx]\n",FindTask(NULL)));

		    if(pr.pr_Link.mln_Succ->mln_Succ != NULL)
		    {
			/* Wake up next reader */
			Signal(((struct PostRequest*) pr.pr_Link.mln_Succ)->pr_Waiter, SIGF_SINGLE);
		    }

		    Remove((struct Node*) &pr);
		}

		CBUn->cu_ReadID++;
		ioClip(ioreq)->io_ClipID = CBUn->cu_ReadID;

		CBUn->cu_clipFile = Open(CBUn->cu_clipFilename, MODE_OLDFILE);

		if(!CBUn->cu_clipFile)
		{
		    D(bug("clipboard.device/CMD_READ: No clip file. Calling ReleaseSemaphore [me=%08lx]\n", FindTask(NULL)));
		    ReleaseSemaphore(&CBUn->cu_UnitLock);
		    ioClip(ioreq)->io_ClipID = -1;
		    ioClip(ioreq)->io_Actual = 0;
//		    ioClip(ioreq)->io_Error = IOERR_ABORTED;
		    break;
		}
	    }
	    else if(ioClip(ioreq)->io_ClipID != CBUn->cu_ReadID)
	    {
		D(bug("clipboard.device/CMD_READ: Invalid clip id.\n"));
		ioClip(ioreq)->io_Actual = 0;
//		ioClip(ioreq)->io_Error = IOERR_ABORTED;
		break;
	    }

	    readCb(ioreq, CBBase);

	    break;


	case CMD_UPDATE:
	    D(bug("clipboard.device/Command: CMD_UPDATE\n"));

            updateCb(ioreq, CBBase);
	    break;


	case CBD_POST:
	    D(bug("clipboard.device/Command: CBD_POST [me=%08lx]\n", FindTask(NULL)));
	    ObtainSemaphore(&CBUn->cu_UnitLock);

	    CBUn->cu_WriteID++;
	    CBUn->cu_PostID = CBUn->cu_WriteID;
	    CBUn->cu_PostPort = (struct MsgPort *)ioClip(ioreq)->io_Data;

	    ioClip(ioreq)->io_ClipID = CBUn->cu_PostID;

	    ReleaseSemaphore(&CBUn->cu_UnitLock);

	    D(bug("clipboard.device/CBD_POST: Calling monitoring hooks\n"));

	    /* Call monitoring hooks. */
	    ObtainSemaphore(&CBBase->cb_SignalSemaphore);
	    {
	      struct Node        *tnode;
	      struct ClipHookMsg  chmsg;

	      chmsg.chm_Type = 0;
	      chmsg.chm_ChangeCmd = CBD_POST;
	      chmsg.chm_ClipID = CBUn->cu_PostID;

	      ForeachNode(&CBBase->cb_HookList, tnode)
	      {
		D(bug("Calling hook %08x\n",tnode));
		CallHookA((struct Hook *)tnode, CBUn, &chmsg);
	      }
	      D(bug("Done\n"));
	    }
	    ReleaseSemaphore(&CBBase->cb_SignalSemaphore);

	    D(bug("clipboard.device/CBD_POST: Called monitoring hooks\n"));		    

#if 0
	    // This does not seem to be robust enough; it can lead to
	    // a ping-pong effect. Never mind then.
	    
	    ObtainSemaphore(&CBUn->cu_UnitLock);

	    if(!IsListEmpty((struct List*) &CBUn->cu_PostRequesters))
	    {
	      /* Normally, this can never happen. However, if an app
	         deadlocked by posting and then reading, try to make
	         this CBD_POST turn into a CMD_WRITE immediately. */

	      D(bug("clipboard.device/Command: CMD_POST..notify PostPort 0x%lx\n", CBUn->cu_PostPort));

	      CBUn->cu_Satisfy.sm_ClipID = CBUn->cu_PostID;
	      PutMsg(CBUn->cu_PostPort, (struct Message *)&CBUn->cu_Satisfy);
	      CBUn->cu_PostPort = NULL;
	    }
	    
	    ReleaseSemaphore(&CBUn->cu_UnitLock);
#endif
	    break;


	case CBD_CURRENTREADID:
	    D(bug("clipboard.device/Command: CBD_CURRENTREADID\n"));
	    ioClip(ioreq)->io_ClipID = CBUn->cu_WriteID;
	    /* NOTE: NOT A BUG! Was PostID. Note that AmigaOS really has a
	       ReadID counter that is *almost* always the same as WriteID. */
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
    {
	ReplyMsg(&ioreq->io_Message);
    }

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IORequest *,      ioreq, A1),
	  struct ClipboardBase *, CBBase, 6, Clipboard)
{
    AROS_LIBFUNC_INIT

    /* Keep compiler happy */
    (void) ioreq;
    (void) CBBase;

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
	D(bug("clipboard.device/readcb: nothing to read. setting IOERR_ABORTED as error and releasing semaphore [me=%08lx]\n", FindTask(NULL)));
	Close(CBUn->cu_clipFile);
	CBUn->cu_clipFile = 0;
	ReleaseSemaphore(&CBUn->cu_UnitLock);
//	ioClip(ioreq)->io_Error = IOERR_ABORTED;
	ioClip(ioreq)->io_Actual = 0;
	ioClip(ioreq)->io_ClipID = -1;
	return;
    }

    if(ioClip(ioreq)->io_Offset >= CBUn->cu_clipSize)
    {
	D(bug("clipboard.device/readCb: detected \"end of file\". Closing clipfile and releasing semaphore [me=%08lx]\n", FindTask(NULL)));
	Close(CBUn->cu_clipFile);
	CBUn->cu_clipFile = 0;
	ReleaseSemaphore(&CBUn->cu_UnitLock);
	ioClip(ioreq)->io_Actual = 0;
	ioClip(ioreq)->io_ClipID = -1;
	return;
    }

    if (!ioClip(ioreq)->io_Data)
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

    	if (ioClip(ioreq)->io_Length >= 4)
	{
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
    	}
    	else if (ioClip(ioreq)->io_Length == 2)
	{
	    D(bug("clipboard.device/readCb: Did Read: data length = %d  data = %02x%02x (%c%c)\n",
		  ioClip(ioreq)->io_Length,
		  ((UBYTE *)ioClip(ioreq)->io_Data)[0],
		  ((UBYTE *)ioClip(ioreq)->io_Data)[1],
		  ((UBYTE *)ioClip(ioreq)->io_Data)[0],
		  ((UBYTE *)ioClip(ioreq)->io_Data)[1]));
    	}
	else if (ioClip(ioreq)->io_Length == 1)
	{
	    D(bug("clipboard.device/readCb: Did Read: data length = %d  data = %02x (%c)\n",
		  ioClip(ioreq)->io_Length,
		  ((UBYTE *)ioClip(ioreq)->io_Data)[0],
		  ((UBYTE *)ioClip(ioreq)->io_Data)[0]));
	}
	else
	{
	    D(bug("clipboard.device/readCb: Did Read nothing: data length = 0  data = 0x%x\n",
	    	  ioClip(ioreq)->io_Data));
	}
	
	ioClip(ioreq)->io_Offset += ioClip(ioreq)->io_Actual;

	if (ioClip(ioreq)->io_Actual == 0)
	{
	    Close(CBUn->cu_clipFile);
	    CBUn->cu_clipFile = 0;
	    D(bug("clipboard.device/readCb: io_Actual=0. Calling ReleaseSemaphore [me=%08lx]\n", FindTask(NULL)));
	    ReleaseSemaphore(&CBUn->cu_UnitLock);
	    ioClip(ioreq)->io_ClipID = -1;
	}
    }
}

/****************************************************************************************/

static void writeCb(struct IORequest *ioreq, struct ClipboardBase *CBBase)
{
    D(bug("clipboard.device/writeCb: Trying to get unit lock. Calling ObtainSemaphore [me=%08lx]\n", FindTask(NULL)));
    ObtainSemaphore(&CBUn->cu_UnitLock);
    D(bug("clipboard.device/writeCb: Got unit lock.\n"));

    if(ioClip(ioreq)->io_ClipID == 0 ||
       ioClip(ioreq)->io_ClipID == CBUn->cu_PostID)
    {
	/* A new write begins... */

	CBUn->cu_clipSize = 0;

        if(ioClip(ioreq)->io_ClipID == 0)
	{
	    CBUn->cu_WriteID++;
	    ioClip(ioreq)->io_ClipID = CBUn->cu_WriteID;
	}

	/* No more POST writes accepted */
	CBUn->cu_PostID = 0;

	if (!(CBUn->cu_clipFile = Open(CBUn->cu_clipFilename, MODE_NEWFILE)))
	{
	    D(bug("clipboard.device/writeCb: Opening clipfile in MODE_NEWFILE failed. Releasing Semaphore [me=%08lx]\n", FindTask(NULL)));
	    ReleaseSemaphore(&CBUn->cu_UnitLock);
	    ioClip(ioreq)->io_Error = IOERR_ABORTED;
	    ioClip(ioreq)->io_Actual = 0;
	    ioClip(ioreq)->io_ClipID = -1;
	    return;
	}

	D(bug("clipboard.device/writeCb: Opened file %s\n", CBUn->cu_clipFilename));
    }
    else if(ioClip(ioreq)->io_ClipID == CBUn->cu_WriteID)
    {
        D(bug("We already have the semaphore. [me=%08lx]\n", FindTask(NULL)));
	ReleaseSemaphore(&CBUn->cu_UnitLock);
	
	/* Continue the previous write */
    }
    else
    {
        D(bug("Invalid ClipID. Releasing Semaphore [me=%08lx]\n", FindTask(NULL)));
	ReleaseSemaphore(&CBUn->cu_UnitLock);
	
	/* Error */
	ioClip(ioreq)->io_Error = IOERR_ABORTED;
	ioClip(ioreq)->io_Actual = 0;
	return;
    }

    if(ioClip(ioreq)->io_Offset > CBUn->cu_clipSize)
    {
	ULONG len = ioClip(ioreq)->io_Offset - CBUn->cu_clipSize;
	ULONG buflen = len > WRITEBUFSIZE ? WRITEBUFSIZE : len;
	UBYTE *buf = AllocMem(buflen, MEMF_CLEAR | MEMF_PUBLIC);

    	Seek(CBUn->cu_clipFile, 0, OFFSET_END);

	if (buf)
	{
	    while(len)
	    {
		ULONG size = len > WRITEBUFSIZE ? WRITEBUFSIZE : len;

		Write(CBUn->cu_clipFile, buf, size);
		len -= size;
	    }
	    FreeMem(buf, buflen);
	}
    }
    Seek(CBUn->cu_clipFile, ioClip(ioreq)->io_Offset, OFFSET_BEGINNING);

    D(bug("clipboard.device/writeCb: Did Seek(), offset = %d\n", ioClip(ioreq)->io_Offset));

    if (ioClip(ioreq)->io_Length >= 4)
    {
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
    }
    else if (ioClip(ioreq)->io_Length == 2)
    {
	D(bug("clipboard.device/Doing Write: data length = %d  data = %02x%02x (%c%c)\n",
	    ioClip(ioreq)->io_Length,
	    ((UBYTE *)ioClip(ioreq)->io_Data)[0],
	    ((UBYTE *)ioClip(ioreq)->io_Data)[1],
	    ((UBYTE *)ioClip(ioreq)->io_Data)[0],
	    ((UBYTE *)ioClip(ioreq)->io_Data)[1]));
    }
    else if (ioClip(ioreq)->io_Length == 1)
    {
	D(bug("clipboard.device/Doing Write: data length = %d  data = %02x (%c)\n",
	    ioClip(ioreq)->io_Length,
	    ((UBYTE *)ioClip(ioreq)->io_Data)[0],
	    ((UBYTE *)ioClip(ioreq)->io_Data)[0]));
        
    }
    else
    {
	D(bug("clipboard.device/Doing Write: Not really!!: data length = 0  data = 0x%x\n",
	      ioClip(ioreq)->io_Data));
    }
    
    if (ioClip(ioreq)->io_Length)
    {
    	ioClip(ioreq)->io_Actual = Write(CBUn->cu_clipFile, ioClip(ioreq)->io_Data, ioClip(ioreq)->io_Length);
    }
    else
    {
    	ioClip(ioreq)->io_Actual = 0; /* texteditor.mcc does 0-length writes */
    }

    if ((LONG)ioClip(ioreq)->io_Actual == -1)
    {
	D(bug("clipboard.device/writeCb: write failed\n"));
	Close(CBUn->cu_clipFile);
	CBUn->cu_clipFile = 0;
	D(bug("clipboard.device/writeCb: releasing semaphore [me=%08lx]\n", FindTask(NULL)));
	ReleaseSemaphore(&CBUn->cu_UnitLock);
	ioClip(ioreq)->io_Error = IOERR_ABORTED;
	ioClip(ioreq)->io_Actual = 0;
	ioClip(ioreq)->io_ClipID = -1;
    }
    else
    {
	ioClip(ioreq)->io_Offset += ioClip(ioreq)->io_Actual;
	if(ioClip(ioreq)->io_Offset > CBUn->cu_clipSize)
	{
	    CBUn->cu_clipSize = ioClip(ioreq)->io_Offset;
	}
    }
}

/****************************************************************************************/

static void updateCb(struct IORequest *ioreq, struct ClipboardBase *CBBase)
{
    if(CBUn->cu_WriteID != 0 && CBUn->cu_WriteID == ioClip(ioreq)->io_ClipID)
    {
	D(bug("clipboard.device/updateCb: Closing ClipFile\n"));

	Close(CBUn->cu_clipFile);
	CBUn->cu_clipFile = 0;
	
	if(CBUn->cu_PostRequesters.mlh_Head->mln_Succ != NULL)
	{
	    /* Wake up first reader */
	    D(bug("clipboard.device/updateCb: Waking up %08lx\n", ((struct PostRequest*) CBUn->cu_PostRequesters.mlh_Head)->pr_Waiter));
	    Signal(((struct PostRequest*) CBUn->cu_PostRequesters.mlh_Head)->pr_Waiter, SIGF_SINGLE);
	}
	D(bug("clipboard.device/updateCb: calling ReleaseSemaphore [me=%08lx]\n", FindTask(NULL)));

	ReleaseSemaphore(&CBUn->cu_UnitLock);

	D(bug("clipboard.device/updateCb: Calling monitoring hooks\n"));

	/* Call monitoring hooks. */
	ObtainSemaphore(&CBBase->cb_SignalSemaphore);
	{
	    struct Node        *tnode;
	    struct ClipHookMsg  chmsg;

	    chmsg.chm_Type = 0;
	    chmsg.chm_ChangeCmd = CMD_UPDATE;
	    chmsg.chm_ClipID = ioClip(ioreq)->io_ClipID;

	    ForeachNode(&CBBase->cb_HookList, tnode)
	    {
	      D(bug("Calling hook %08x\n",tnode));
	        CallHookA((struct Hook *)tnode, CBUn, &chmsg);
	    }
	    D(bug("Done\n"));
	}
	ReleaseSemaphore(&CBBase->cb_SignalSemaphore);

	D(bug("clipboard.device/updateCb: Called monitoring hooks\n"));
    }
    else
    {
	ioClip(ioreq)->io_Error = IOERR_ABORTED;
	ioClip(ioreq)->io_Actual = 0;
    }

    ioClip(ioreq)->io_ClipID = -1;

    D(bug("clipboard.device/updateCb: end of function [me=%08lx]\n", FindTask(NULL)));

}

/****************************************************************************************/
