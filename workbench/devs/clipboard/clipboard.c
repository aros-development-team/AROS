/*
    (C) 1998-99 AROS - The Amiga Research OS
    $Id$

    Desc: Clipboard device
    Lang: English
*/

/* HISTORY:  230498  SDuvan  Began work
 *           310598  SDuvan  Finished overall structure(?)
 *           210399  SDuvan  Removed special support for PRIMARY_CLIP unit.
 */

#define AROS_ALMOST_COMPATIBLE 1
#include "clipboard_intern.h"
#include <exec/resident.h>
#include <devices/clipboard.h>
#include <exec/io.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/lists.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#define DEBUG 1
#include <aros/debug.h>
#include <aros/machine.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef    __GNUC__
#include  "clipboard_gcc.h"
#endif


#define ioClip(x)  ((struct IOClipReq *)x)
#define min(x,y)   (((x) < (y)) ? (x) : (y))

#define CBUn    (((struct ClipboardUnit *)ioreq->io_Unit))

void writeCb(struct IORequest *ioreq, struct ClipboardBase *CBBase);
void readCb(struct IORequest *ioreq, struct ClipboardBase *CBBase);

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
    RTF_AUTOINIT|RTF_COLDSTART,
    41,
    NT_DEVICE,
    45,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[] = "clipboard.device";

static const char version[] = "$VER: clipboard.device 41.0 (3.4.1999)\r\n";

static const APTR inittabl[4] =
{
    (APTR)sizeof(struct ClipboardBase),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init, Clipboard)
};

static void *const functable[] =
{
    &AROS_SLIB_ENTRY(open, Clipboard),
    &AROS_SLIB_ENTRY(close, Clipboard),
    &AROS_SLIB_ENTRY(expunge, Clipboard),
    &AROS_SLIB_ENTRY(null, Clipboard),
    &AROS_SLIB_ENTRY(beginio, Clipboard),
    &AROS_SLIB_ENTRY(abortio, Clipboard),
    (void *)-1
};


AROS_LH2(struct ClipboardBase *,  init,
 AROS_LHA(struct ClipboardBase *, CBBase, D0),
 AROS_LHA(BPTR,          segList, A0),
	  struct ExecBase *, sysBase, 0, Clipboard)
{
    AROS_LIBFUNC_INIT

    /* Store arguments */
    CBBase->cb_sysBase = sysBase;
    CBBase->cb_seglist = segList;

    InitSemaphore(&CBBase->cb_SignalSemaphore);
    NEWLIST(&CBBase->cb_UnitList);
    NEWLIST(&CBBase->cb_HookList);
	
    return CBBase;
    AROS_LIBFUNC_EXIT
}


/* Putchar procedure needed by RawDoFmt() */

AROS_UFH2(void, putchr,
    AROS_UFHA(UBYTE,    chr, D0),
    AROS_UFHA(STRPTR *, p,   A3))
{
    AROS_LIBFUNC_INIT
    *(*p)++=chr;
    AROS_LIBFUNC_EXIT
}

void cb_sprintf(struct ClipboardBase *CBBase, UBYTE *buffer,
		UBYTE *format, ...)
{
    RawDoFmt(format, &format+1, (VOID_FUNC)putchr, &buffer);
}


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

    D(bug("clipboard_open: Entering...\n"));

    if(unitnum > 255)
    {
	ioClip(ioreq)->io_Error = IOERR_OPENFAIL;
	return;
    }

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
	D(bug("Checking for CLIPS:\n"));

	if((tempLock = Lock("CLIPS:", ACCESS_READ)) == NULL)
	{
	    /* CLIPS: is not assigned - revert to DEVS:Clipboards */

	    D(bug("CLIPS: not found\n"));
	    D(bug("Checking for DEVS:\n"));

	    if((tempLock2 = Lock("DEVS:", ACCESS_READ)) == NULL)
	    {
		D(bug("DEVS: Not found."));
		ioreq->io_Error = IOERR_OPENFAIL;
	    }
	    else
	    {
		D(bug("Found DEVS:\n"));
		D(bug("Checking for DEVS:Clipboards\n"));

		if((tempLock3 = Lock("DEVS:Clipboards", ACCESS_READ)) == NULL)
		{
		    D(bug("Not found -- creating DEVS:Clipboards.\n"));
		    if(CreateDir("DEVS:Clipboards") == NULL)
		    {
			ioreq->io_Error = IOERR_OPENFAIL;
		    }
		}
	    }
	    CBBase->cb_ClipDir = "DEVS:Clipboards/";
	}
	else
	{
	    D(bug("Found CLIPS:\n"));
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
	if(tempNode->ln_Type == unitnum)
	{
	    found = TRUE;
	    break;
	}
    }	

    if(found == FALSE)
    {
	struct MsgPort *replyPort;

	D(bug("Building unit...\n"));

	ioreq->io_Unit = (struct Unit *)AllocMem(sizeof(struct ClipboardUnit),
						 MEMF_CLEAR | MEMF_PUBLIC);

	if(ioreq->io_Unit == NULL)
	{
	    /* TODO: Clean up! */
	    ;
	}

	CBUn->cu_UnitNum = unitnum;
	CBUn->cu_Node.ln_Type = unitnum;
	CBUn->cu_PostID = -1;             /* Make sure PostID != ClipID. */

	InitSemaphore(&CBUn->cu_UnitLock);

	/* Construct clipboard unit filename. */
	CBUn->cu_clipFilename = AllocMem(CBUN_FILENAMELEN, MEMF_ANY);

	if(CBUn->cu_clipFilename == NULL)
	{
	    /* TODO: Clean up! */
	    ;
	}


	cb_sprintf(CBBase, CBUn->cu_clipFilename, "%s%u", CBBase->cb_ClipDir,
		   unitnum);

	/* Create the replyport for satisfy messages. Note that SIGB_SINGLE
	   is used as the signal as we can't use CreateMsgPort() as it
	   requires that a signal bit in the calling task is free.
	   There should, ideally, be a function internal_CreateMsgPort()
	   which takes a signal number as input and creates a message port,
	   and this function should be used by both this routine and
	   the real CreateMsgPort() */

	replyPort = (struct MsgPort *)AllocMem(sizeof(struct MsgPort),
					       MEMF_PUBLIC | MEMF_CLEAR);

	if(replyPort == NULL)
	{
	    /* TODO: Clean up. Free structures, close libraries...  */
	    /*       Maybe arrange so that I can call close() here. */
	    ;
	}

	replyPort->mp_SigBit = SIGB_SINGLE;
	replyPort->mp_MsgList.lh_Head = 
	    (struct Node *)&replyPort->mp_MsgList.lh_Tail;
	replyPort->mp_MsgList.lh_TailPred = 
	    (struct Node *)&replyPort->mp_MsgList.lh_Head;
		
	replyPort->mp_Flags = PA_SIGNAL;
	replyPort->mp_Node.ln_Type = NT_MSGPORT;

	CBUn->cu_Satisfy.sm_Msg.mn_ReplyPort = replyPort;

	/* Initialization is done, and everything went OK. Add unit to the
	   list of clipboard units. */
	Insert((struct List *)&CBBase->cb_UnitList, (struct Node *)CBUn, NULL);
    }
    else
    {
	CBUn = (struct ClipboardUnit *)tempNode;
    }

    CBUn->cu_OpenCnt++;

    /* I have one more opener. */
    CBBase->cb_device.dd_Library.lib_OpenCnt++;

    ReleaseSemaphore(&CBBase->cb_SignalSemaphore);

    AROS_LIBFUNC_EXIT
}


AROS_LH1(BPTR, close,
 AROS_LHA(struct IORequest *,     ioreq,  A1),
	  struct ClipboardBase *, CBBase,  2, Clipboard)
{
    AROS_LIBFUNC_INIT

    /* Let any following attemps to use the device crash hard. */
    ioreq->io_Device = (struct Device *)-1;

    CBUn->cu_OpenCnt--;

    if(CBUn->cu_OpenCnt == 0)
    {
	Remove((struct Node *)ioreq->io_Unit);
	FreeMem(CBUn->cu_clipFilename, CBUN_FILENAMELEN);
	FreeMem(ioreq->io_Unit, sizeof(struct ClipboardUnit));
    }

    CBBase->cb_device.dd_Library.lib_OpenCnt--;

    if(CBBase->cb_device.dd_Library.lib_OpenCnt == 0)
	expunge();

    return 0;
    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, expunge, struct ClipboardBase *, CBBase, 3, Clipboard)
{
    AROS_LIBFUNC_INIT

    BPTR ret;	     /* Temporary vaiable to preserve seglist. */

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

    return ret;

    AROS_LIBFUNC_EXIT
}


AROS_LH0I(int, null, struct ClipboardBase *, KBBase, 4, Clipboard)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}


AROS_LH1(void, beginio,
	 AROS_LHA(struct IORequest *, ioreq, A1),
	 struct ClipboardBase *, CBBase, 5, Clipboard)
{
    AROS_LIBFUNC_INIT

    ioreq->io_Error = 0;

    switch (ioreq->io_Command)
    {
    case CBD_CHANGEHOOK:
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

	D(bug("Command: CMD_WRITE\n"));

	writeCb(ioreq, CBBase);
	break;

    case CMD_READ:

	D(bug("Command: CMD_READ\n"));

	/* Get new ID if this is the beginning of a read operation */
	if(ioClip(ioreq)->io_ClipID == 0)
	{
	    D(bug("Trying to get unit lock.\n"));

	    ObtainSemaphore(&CBUn->cu_UnitLock);
	    CBUn->cu_ReadID++;
	    ioClip(ioreq)->io_ClipID = CBUn->cu_ReadID;

	    D(bug("Got unit lock.\n"));

	    CBUn->cu_clipFile = Open(CBUn->cu_clipFilename, MODE_OLDFILE);
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
		ioreq->io_Error = IOERR_ABORTED;
		break;
	    }


	    /* Make sure WE are signalled. */
	    CBUn->cu_Satisfy.sm_Msg.mn_ReplyPort->mp_SigTask = me;

	    PutMsg(CBUn->cu_PostPort, (struct Message *)&CBUn->cu_Satisfy);
	    WaitPort(CBUn->cu_Satisfy.sm_Msg.mn_ReplyPort);
	}

	readCb(ioreq, CBBase);

	break;


    case CMD_UPDATE:
	Close(CBUn->cu_clipFile);

	CBUn->cu_clipSize = ioClip(ioreq)->io_Offset;

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


	/* If it is the POSTer that indicates he is done, we don't
	   release the semaphore as, in case, the semaphore was
	   never locked because it was originally locked by the
	   reader wanting the (to be) posted clip. As we couldn't
	   release the semaphore then as another task then could have
	   started to write something to the clipboard, we just do
	   nothing. */

	if(CBUn->cu_PostID != ioClip(ioreq)->io_ClipID)
	    ReleaseSemaphore(&CBUn->cu_UnitLock);
	
	break;


    case CBD_POST:
	ObtainSemaphore(&CBUn->cu_UnitLock);

	CBUn->cu_WriteID++;
	CBUn->cu_PostID = CBUn->cu_WriteID;
	CBUn->cu_PostPort = (struct MsgPort *)ioClip(ioreq)->io_Data;
	CBUn->cu_Poster = FindTask(NULL);
	
	ReleaseSemaphore(&CBUn->cu_UnitLock);
	break;


    case CBD_CURRENTREADID:
	ioClip(ioreq)->io_ClipID = CBUn->cu_PostID;
	break;


    case CBD_CURRENTWRITEID:
	ioClip(ioreq)->io_ClipID = CBUn->cu_WriteID;
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
 AROS_LHA(struct IORequest *,      ioreq, A1),
	  struct ClipboardBase *, CBBase, 6, Clipboard)
{
    AROS_LIBFUNC_INIT

    /* Nothing to abort */
    return 0;

    AROS_LIBFUNC_EXIT
}

void readCb(struct IORequest *ioreq, struct ClipboardBase *CBBase)
{
    /* Is there anything to be read? */
    if(CBUn->cu_WriteID == 0)
    {
	ioClip(ioreq)->io_Error = IOERR_ABORTED;
	return;
    }

    if(ioClip(ioreq)->io_Offset >= CBUn->cu_clipSize)
    {
	ioClip(ioreq)->io_Actual = 0;
	Close(CBUn->cu_clipFile);
	ReleaseSemaphore(&CBUn->cu_UnitLock);
	return;
    }

    D(bug("Doing Seek() at offset %i.\n",
	  ioClip(ioreq)->io_Offset));

    Seek(CBUn->cu_clipFile, ioClip(ioreq)->io_Offset, OFFSET_BEGINNING);

    ioClip(ioreq)->io_Actual = Read(CBUn->cu_clipFile, ioClip(ioreq)->io_Data,
				    ioClip(ioreq)->io_Length);
    ioClip(ioreq)->io_Offset += ioClip(ioreq)->io_Actual;
}


void writeCb(struct IORequest *ioreq, struct ClipboardBase *CBBase)
{
    if(ioClip(ioreq)->io_Offset == 0)
    {
	/* A new write begins... */

	/* If this is the poster writing, we will not obtain the
	   semaphore as it's being kept by the current reader.
	   Instead, we don't give a damn about any semaphores and
	   continue the operation anyway. This may be done as it's
	   the reader who has called us, and he (clipboard.
	   device) made sure it is OK to change things in the
	   clipboard unit. */

	CBUn->cu_WriteID++;
	ioClip(ioreq)->io_ClipID = CBUn->cu_WriteID;

	if(ioClip(ioreq)->io_ClipID != CBUn->cu_PostID)
	{
	    D(bug("Trying to get unit lock.\n"));
	    ObtainSemaphore(&CBUn->cu_UnitLock);
	    D(bug("Got unit lock.\n"));
	}

	if((CBUn->cu_clipFile = Open(CBUn->cu_clipFilename, MODE_NEWFILE)) == NULL)
	{
	    ReleaseSemaphore(&CBUn->cu_UnitLock);
	    ioreq->io_Error = IOERR_ABORTED;
	    return;
	}

	D(bug("Opened file %s\n", CBUn->cu_clipFilename));
    }

    Seek(CBUn->cu_clipFile, ioClip(ioreq)->io_Offset, OFFSET_BEGINNING);

    D(bug("Did Seek(), offset = %i\n", ioClip(ioreq)->io_Offset));

    if(Write(CBUn->cu_clipFile, ioClip(ioreq)->io_Data,
	     ioClip(ioreq)->io_Length) == -1)
    {
	Close(CBUn->cu_clipFile);
	ioreq->io_Error = IOERR_ABORTED;

	/* Should we release the semaphore here? Or maybe release it and
	   indicate that it has been released in case of a user doing
	   CBD_UPDATE in spite of the error? */
    }
    ioClip(ioreq)->io_Offset += ioClip(ioreq)->io_Length;
    ioClip(ioreq)->io_Actual = ioClip(ioreq)->io_Length;
}


static const char end = 0;
