/*
    Copyright (C) 1995-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Filesystem that uses console device for input/output.
    Lang: english
*/

/* 
   Please always update the version-string below, if you modify the code!
*/

/* AROS includes */

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/alerts.h>
#include <utility/tagitem.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <devices/conunit.h>

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "con_handler_intern.h"
#include "support.h"

#include <string.h>
#include <stdio.h>

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable;

struct conbase * AROS_SLIB_ENTRY(init,con_handler) ();
void AROS_SLIB_ENTRY(open,con_handler) ();
BPTR AROS_SLIB_ENTRY(close,con_handler) ();
BPTR AROS_SLIB_ENTRY(expunge,con_handler) ();
int AROS_SLIB_ENTRY(null,con_handler) ();
void AROS_SLIB_ENTRY(beginio,con_handler) ();
LONG AROS_SLIB_ENTRY(abortio,con_handler) ();

static const char end;


int con_handler_entry(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

const struct Resident con_handler_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&con_handler_resident,
    (APTR)&end,
    RTF_AUTOINIT | RTF_AFTERDOS,
    41,
    NT_DEVICE,
    -126,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="con-handler";

static const char version[]="$VER: con-handler 41.1 (22.1.2000)\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct conbase),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,con_handler)
};

static void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,con_handler),
    &AROS_SLIB_ENTRY(close,con_handler),
    &AROS_SLIB_ENTRY(expunge,con_handler),
    &AROS_SLIB_ENTRY(null,con_handler),
    &AROS_SLIB_ENTRY(beginio,con_handler),
    &AROS_SLIB_ENTRY(abortio,con_handler),
    (void *)-1
};

static const UBYTE datatable=0;

/*********************************** Support *******************************/

#define ioReq(x) ((struct IORequest *)x)

/*****************
**  open_con()  ** creates a con unit task
*****************/

#if 0
static const struct TagItem win_tags[] =
{
    {WA_Width,		500},
    {WA_Height,		300},
    {WA_SmartRefresh,	TRUE},
    {WA_DragBar,	TRUE},
    {WA_IDCMP,		0},
    {WA_Title,		(IPTR)"CON:"},
    {WA_Flags,		WFLG_DEPTHGADGET | WFLG_SIZEGADGET | WFLG_DRAGBAR | WFLG_CLOSEGADGET | WFLG_SIZEBRIGHT },
    {TAG_DONE,		0UL}
};
#endif

static const struct NewWindow default_nw =
{
    0,				/* LeftEdge */
    0,				/* TopEdge */
    560,			/* Width */
    300,			/* Height */
    1,				/* DetailPen */
    0,				/* BlockPen */
    IDCMP_CLOSEWINDOW,		/* IDCMP */
    WFLG_DEPTHGADGET |
    WFLG_SIZEGADGET |
    WFLG_DRAGBAR |
    WFLG_SIZEBRIGHT |
    WFLG_ACTIVATE,
    0,				/* FirstGadget */
    0,				/* CheckMark */
    "CON:",			/* Title */
    0,				/* Screen */
    0,				/* Bitmap */
    100,			/* MinWidth */
    100,			/* MinHeight */
    32767,			/* MaxWidth */
    32767,			/* MaxHeight */
    WBENCHSCREEN		/* type */
};

static LONG open_con(struct conbase 	*conbase
        ,struct IOFileSys		*iofs)
{
    LONG err = 0;
    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;
#if DEBUG
    STRPTR filename = iofs->io_Union.io_OPEN.io_Filename;
    ULONG mode = iofs->io_Union.io_OPEN.io_FileMode;
#endif
    struct conTaskParams params;
    struct Task *contask;
    
    EnterFunc(bug("open_conh(filename=%s, mode=%d)\n",
    	filename, mode));

    if (fh)
    {
        /* DupLock */
	fh->usecount++;
    } else {

	params.conbase = conbase;
	params.iofs = iofs;
	params.parentTask = FindTask(NULL);
	params.initSignal = SIGF_DOS;

	SetSignal(0, SIGF_DOS);

	contask = createConTask(&params, conbase);
	if (contask)
	{
	    Wait(SIGF_DOS);
	    if (iofs->io_DosError) RemTask(contask);
	}

	err = iofs->io_DosError;
    }
    
    ReturnInt("open_conh", LONG, err);
}


static void close_con(struct conbase *conbase, struct IOFileSys *iofs)
{
    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;

    EnterFunc(bug("close_con(fh=%p)\n", fh));
    
    fh->usecount--;
    if (fh->usecount > 0)
    {
	if (iofs->IOFS.io_Message.mn_ReplyPort->mp_SigTask == fh->breaktask)
	{
	    fh->breaktask = 0;
	}
	
        iofs->io_DosError = 0;
	ReplyMsg(&iofs->IOFS.io_Message);
	return;
    }
    
    /* Abort all pending requests */
    if (!CheckIO( ioReq(fh->conreadio) ))
    	AbortIO( ioReq(fh->conreadio) );
	
    /* Wait for abort */
    WaitIO( ioReq(fh->conreadio) );
    
    /* Clean up */
    CloseDevice((struct IORequest *)fh->conreadio);
    CloseWindow(fh->window);
    DeleteIORequest( ioReq(fh->conreadio) );
    FreeMem(fh->conreadmp, sizeof (struct MsgPort) * 3);
    
    FreeMem(fh, sizeof (struct filehandle));
    if (fh->wintitle) FreeVec(fh->wintitle);
    
    iofs->io_DosError = 0;
    iofs->IOFS.io_Unit = NULL;    
    ReplyMsg(&iofs->IOFS.io_Message);
    
    /* let's kill ourselves */
    RemTask(FindTask(NULL));
}

static void con_read(struct conbase *conbase, struct IOFileSys *iofs)
{
    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;

    if (fh->flags & FHFLG_CANREAD)
    {
        ULONG readlen = (fh->canreadsize < iofs->io_Union.io_READ.io_Length) ? fh->canreadsize :
									       iofs->io_Union.io_READ.io_Length;
	/* we must correct io_READ.io_Length, because answer_read_request
	   would read until fh->inputsize if possible, but here it is allowed only
	   to read max. fh->canreadsize chars */
	   
	iofs->io_Union.io_READ.io_Length = readlen;	
	answer_read_request(conbase, fh, iofs);
	
	fh->canreadsize -= readlen;	
	if (fh->canreadsize == 0) fh->flags &= ~FHFLG_CANREAD;
	
    } else {    
	AddTail((struct List *)&fh->pendingReads, (struct Node *)iofs);
	fh->flags |= FHFLG_READPENDING;
    }
}

static void con_write(struct conbase *conbase, struct IOFileSys *iofs)
{
    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;

    EnterFunc(bug("con_write(fh=%p, buf=%s)\n", fh, iofs->io_Union.io_WRITE.io_Buffer));
    
    if ((fh->inputsize - fh->inputstart) == 0)
    {
        answer_write_request(conbase, fh, iofs);
    } else {
    	AddTail((struct List *)&fh->pendingWrites, (struct Node *)iofs);
	fh->flags |= FHFLG_WRITEPENDING;
    }
}

/************************ Library entry points ************************/

/************
** init()  **
************/

AROS_LH2(struct conbase *, init,
 AROS_LHA(struct conbase *, conbase, D0),
 AROS_LHA(BPTR,              segList,   A0),
	   struct ExecBase *, sysBase, 0, con_handler)
{
    AROS_LIBFUNC_INIT
 
    struct DeviceNode *dn;
    static char devnames[2][5] = { "\003CON", "\003RAW" };
    int i;


    /* Store arguments */
    conbase->sysbase = sysBase;
    conbase->seglist = segList;
    conbase->device.dd_Library.lib_OpenCnt = 1;    
  
    conbase->dosbase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    if (conbase->dosbase)
    {
    	conbase->intuibase = (IntuiBase *)OpenLibrary("intuition.library", 37);
	if (conbase->intuibase)
	{
	    /* Really bad hack, but con_handler is in ROM, intuition.library is
	       open, if intuition.library is open, then Input.Device must be
	       open, too, ... and I don't like to OpenDevice just for Peek-
	       Qualifier */
	       
	    #warning InputDevice open hack. Hope this is not a problem since it is only used for PeekQualifier
	    Forbid();
	    conbase->inputbase = (struct Device *)FindName(&conbase->sysbase->DeviceList, "input.device");
	    Permit();

	    /* Install CON: and RAW: handlers into device list
	     *
	     * KLUDGE: con-handler should create only one device node, depending on
	     * the startup packet it gets. The mountlists for CON:/RAW: should be into dos.library bootstrap
	     * routines.
	     */
	    for(i = 0; i < 2; i++)
	    {
		if((dn = AllocMem(sizeof (struct DeviceNode), MEMF_CLEAR|MEMF_PUBLIC)))
		{
		    dn->dn_Type		= DLT_DEVICE;
		    dn->dn_Unit		= NULL;
		    dn->dn_Device	= &conbase->device;
		    dn->dn_Handler	= NULL;
		    dn->dn_Startup	= NULL;
		    dn->dn_OldName	= MKBADDR(devnames[i]);
		    dn->dn_NewName	= &devnames[i][1];

		    if (AddDosEntry((struct DosList *)dn))
		    {
			if (i == 0)
			    continue;

			return conbase;
		    }

	   	    FreeMem(dn, sizeof (struct DeviceNode));
	    	}
	    }

	    CloseLibrary((struct Library *)conbase->intuibase);
 	
    	} /* if (intuition opened) */
	
	CloseLibrary((struct Library *)conbase->dosbase);
   	/* Alert(AT_DeadEnd|AG_NoMemory|AN_Unknown); */
	
    } /* if (dos opened) */

    return NULL;

    AROS_LIBFUNC_EXIT
}

/*************
**  open()  **
*************/

AROS_LH3(void, open,
 AROS_LHA(struct IOFileSys *, iofs, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct conbase *, conbase, 1, con_handler)
{
    AROS_LIBFUNC_INIT

    /* Keep compiler happy */
    unitnum=0;
    flags=0;

/*    if(conbase->dosbase == NULL)
    {
	conbase->dosbase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
	if( conbase->dosbase == NULL )
	{
	    iofs->IOFS.io_Error = IOERR_OPENFAIL;
	    return;
	}
    }

    
*/
   /* I have one more opener. */
    conbase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;

    /* Set returncode */
    iofs->IOFS.io_Error=0;

    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    AROS_LIBFUNC_EXIT
}

/**************
**  close()  **
**************/
AROS_LH1(BPTR, close,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct conbase *, conbase, 2, con_handler)
{
    AROS_LIBFUNC_INIT

    /* Let any following attemps to use the device crash hard. */
    iofs->IOFS.io_Device=(struct Device *)-1;
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************
**  expunge()  **
****************/
AROS_LH0(BPTR, expunge, struct conbase *, conbase, 3, con_handler)
{
    AROS_LIBFUNC_INIT

    /* Do not expunge the device. Set the delayed expunge flag and return. */
    conbase->device.dd_Library.lib_Flags|=LIBF_DELEXP;
    return 0;
    AROS_LIBFUNC_EXIT
}

/*************
**  null()  **
*************/
AROS_LH0I(int, null, struct conbase *, conbase, 4, con_handler)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************
**  beginio()  **
****************/
AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct conbase *, conbase, 5, con_handler)
{
    AROS_LIBFUNC_INIT

    LONG error = 0;
    BOOL request_queued = FALSE;

    EnterFunc(bug("conhandler_BeginIO(iofs=%p)\n", iofs));

    /* WaitIO will look into this */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_MESSAGE;

    /*
	Do everything quick no matter what. This is possible
	because I never need to Wait().
    */
    
    D(bug("Doing command %d\n", iofs->IOFS.io_Command));
    
    switch(iofs->IOFS.io_Command)
    {
	case FSA_OPEN_FILE:
	case FSA_OPEN:
	    error = open_con(conbase, iofs);
/*			    (struct filehandle **)&iofs->IOFS.io_Unit,
			    iofs->io_Union.io_OPEN.io_Filename,
			    iofs->io_Union.io_OPEN.io_FileMode);*/
	    break;
	    
	case FSA_CLOSE:	
        case FSA_READ:
	case FSA_WRITE:
	    iofs->IOFS.io_Flags	&= ~IOF_QUICK;
	    request_queued = TRUE;
	    
	    PutMsg(((struct filehandle *)iofs->IOFS.io_Unit)->contaskmp,
	           (struct Message *)iofs);		   
	    break;
	    
	case FSA_IS_INTERACTIVE:
	    iofs->io_Union.io_IS_INTERACTIVE.io_IsInteractive = TRUE;
	    error = 0;
	    break;

	case FSA_SEEK:
	case FSA_SET_FILE_SIZE:
	    error = ERROR_NOT_IMPLEMENTED;
	    break;

	case FSA_WAIT_CHAR:
	    /* We could manually wait for a character to arrive, but this is
	       currently not implemented. FIXME */
	case FSA_FILE_MODE:
#warning FIXME: not supported yet
	    error=ERROR_ACTION_NOT_KNOWN;
	    break;

	case FSA_SAME_LOCK: 
	case FSA_EXAMINE:
	case FSA_EXAMINE_NEXT:
	case FSA_EXAMINE_ALL:
	case FSA_EXAMINE_ALL_END:
	case FSA_CREATE_DIR:
	case FSA_CREATE_HARDLINK:
	case FSA_CREATE_SOFTLINK:
	case FSA_RENAME:
        case FSA_READ_SOFTLINK:
	case FSA_DELETE_OBJECT:
	case FSA_PARENT_DIR:
        case FSA_PARENT_DIR_POST:
	case FSA_SET_COMMENT:
	case FSA_SET_PROTECT:
	case FSA_SET_OWNER:
	case FSA_SET_DATE:
	case FSA_IS_FILESYSTEM:
	case FSA_MORE_CACHE:
	case FSA_FORMAT:
	case FSA_MOUNT_MODE:
	    error = ERROR_ACTION_NOT_KNOWN;

	default:
	    error=ERROR_ACTION_NOT_KNOWN;
	    break;
    }

    /* Set error code */
    iofs->io_DosError=error;

    /* If the quick bit is not set send the message to the port */
    if(!(iofs->IOFS.io_Flags&IOF_QUICK) && !request_queued)
	ReplyMsg(&iofs->IOFS.io_Message);
	
    ReturnVoid("conhandler_beginio");

    AROS_LIBFUNC_EXIT
}

/****************
**  abortio()  **
****************/
AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct conbase *, conbase, 6, con_handler)
{
    AROS_LIBFUNC_INIT
    /* Everything already done. */
    return 0;
    AROS_LIBFUNC_EXIT
}

/*******************
**  conTaskEntry  **
*******************/
VOID conTaskEntry(struct conTaskParams *param)
{
    struct conbase *conbase = param->conbase;
    
    struct filehandle *fh;
    struct IOFileSys *iofs = param->iofs;
#if DEBUG
    STRPTR filename = iofs->io_Union.io_OPEN.io_Filename;
#endif
    LONG err = 0;
    
    BOOL ok = FALSE;
    
    D(bug("conTaskEntry: taskparams = %x  conbase = %x  iofs = %x  filename = \"%s\"\n",
    			param, conbase, iofs, filename));
    
    fh = AllocMem(sizeof (struct filehandle), MEMF_PUBLIC | MEMF_CLEAR);
    if (fh)
    {
    	D(bug("contask: fh allocated\n"));

	fh->usecount = 1;

        fh->contask = FindTask(NULL);	
	fh->breaktask = param->parentTask;
	
	NEWLIST(&fh->pendingReads);
	NEWLIST(&fh->pendingWrites);
	
    	/* Create msgport for console.device communication
	   and for app <-> contask communication  */
	fh->conreadmp = AllocMem(sizeof (struct MsgPort) * 3, MEMF_PUBLIC|MEMF_CLEAR);
	if (fh->conreadmp)
	{
	
    	    D(bug("contask: mem for conreadmp, conwritemp and contaskmp allocated\n"));

	    fh->conreadmp->mp_Node.ln_Type = NT_MSGPORT;
	    fh->conreadmp->mp_Flags = PA_SIGNAL;
	    fh->conreadmp->mp_SigBit = AllocSignal(-1);
	    fh->conreadmp->mp_SigTask = fh->contask;
	    NEWLIST(&fh->conreadmp->mp_MsgList);

	    fh->conwritemp = fh->conreadmp + 1;
	    
	    fh->conwritemp->mp_Node.ln_Type = NT_MSGPORT;
	    fh->conwritemp->mp_Flags = PA_SIGNAL;
	    fh->conwritemp->mp_SigBit = AllocSignal(-1);
	    fh->conwritemp->mp_SigTask = fh->contask;
	    NEWLIST(&fh->conwritemp->mp_MsgList);
	    
	    fh->contaskmp = fh->conwritemp + 1;

	    fh->contaskmp->mp_Node.ln_Type = NT_MSGPORT;	    
	    fh->contaskmp->mp_Flags = PA_SIGNAL;
	    fh->contaskmp->mp_SigBit = AllocSignal(-1);
	    fh->contaskmp->mp_SigTask = fh->contask;
	    NEWLIST(&fh->contaskmp->mp_MsgList);
	    
	    fh->conreadio = (struct IOStdReq *)CreateIORequest(fh->conreadmp, sizeof (struct IOStdReq));
	    if (fh->conreadio)
	    {
	        struct TagItem win_tags [] =
		{
		    {WA_PubScreen	,0		},
		    {WA_AutoAdjust	,TRUE		},
		    {TAG_DONE				}
		};
		struct NewWindow nw = default_nw; 
		
    	    	D(bug("contask: conreadio created\n"));
		
		parse_filename(conbase, fh, iofs, &nw);
		
		fh->window = OpenWindowTagList(&nw, (struct TagItem *)win_tags);
		if (fh->window)
		{
    	    	    D(bug("contask: window opened\n"));
		    fh->conreadio->io_Data   = (APTR)fh->window;
		    fh->conreadio->io_Length = sizeof (struct Window);
	    	    if (0 == OpenDevice("console.device", CONU_STANDARD, ioReq(fh->conreadio), 0))
		    {
			const UBYTE lf_on[] = {0x9B, 0x32, 0x30, 0x68 }; /* Set linefeed mode    */
			
			D(bug("contask: device opened\n"));
			
			fh->conwriteio = *fh->conreadio;
			fh->conwriteio.io_Message.mn_ReplyPort = fh->conwritemp;
			
			/* Turn the console into LF+CR mode so that both
			   linefeed and carriage return is done on 
			*/
			fh->conwriteio.io_Command	= CMD_WRITE;
			fh->conwriteio.io_Data		= (APTR)lf_on;
			fh->conwriteio.io_Length	= 4;
			
			DoIO(ioReq(&fh->conwriteio));

			iofs->IOFS.io_Unit = (struct Unit *)fh;			
			ok = TRUE;    
		    } /* if (0 == OpenDevice("console.device", CONU_STANDARD, ioReq(fh->conreadio), 0)) */
		    else
		    {
		        err = ERROR_INVALID_RESIDENT_LIBRARY;
		    }
		    if (!ok) CloseWindow(fh->window);
		    
		} /* if (fh->window) */
		else
		{
		    err = ERROR_NO_FREE_STORE;
		}
		if (!ok) DeleteIORequest( ioReq(fh->conreadio) );
	    
	    } /* if (fh->conreadio) */
	    else
	    {
	    	err = ERROR_NO_FREE_STORE;
	    }
	    if (!ok) FreeMem(fh->conreadmp, sizeof(struct MsgPort) * 3);
	    
	} /* if (fh->conreadmp) */
	else
	{
	    err = ERROR_NO_FREE_STORE;
	}
	
	if (!ok) FreeMem(fh, sizeof (struct filehandle));
	
    } /* if (fh) */
    else
    	err = ERROR_NO_FREE_STORE;
	
    iofs->io_DosError = err;
    
    Signal(param->parentTask, param->initSignal);
    if (err)
    {
        D(bug("con task: initialization failed. waiting for parent task to kill me.\n"));
        /* parent Task will kill us */
        Wait(0);
    }
    
    D(bug("con task: initialization okay. entering main loop.\n"));
    
    /* Main Loop */
    
    /* Send first read request to console.device */
    
    fh->conreadio->io_Command = CMD_READ;
    fh->conreadio->io_Data    = fh->consolebuffer;
    fh->conreadio->io_Length  = CONSOLEBUFFER_SIZE;

    SendIO(ioReq(fh->conreadio));

    for(;;)
    {
        ULONG conreadmask = 1L << fh->conreadmp->mp_SigBit;
	ULONG contaskmask = 1L << fh->contaskmp->mp_SigBit;
	ULONG sigs;

	D(bug("contask: waiting for sigs %x\n",conreadmask | contaskmask));
	sigs = Wait(conreadmask | contaskmask);

	if (sigs & contaskmask)
	{
	    /* FSA mesages */
	    D(bug("contask: recevied contask signal\n"));
	    while((iofs = (struct IOFileSys *)GetMsg(fh->contaskmp)))
	    {
		switch(iofs->IOFS.io_Command)
		{
		    case FSA_CLOSE:
	    		close_con(conbase, iofs);
	    		break;

		    case FSA_READ:
 			con_read(conbase, iofs);
			break;

		    case FSA_WRITE:
			con_write(conbase, iofs);
			break;

		} /* switch(iofs->IOFS.io_Command) */

	    } /* while((iofs = (struct IOFileSys *)GetMsg(fh->contaskmp))) */

	} /* if (sigs & contaskmask) */

	if (sigs & conreadmask)
	{
	    UBYTE c;
	    WORD inp;

	    /* console.device read request completed */
	    D(bug("contask: received console device signal\n"));

	    GetMsg(fh->conreadmp);

	    fh->conbuffersize = fh->conreadio->io_Actual;
	    fh->conbufferpos = 0;

	    /* terminate with 0 char */
	    fh->consolebuffer[fh->conbuffersize] = '\0';

	    while((inp = scan_input(conbase, fh, &c)) != INP_DONE)
	    {
		D(bug("Input Code: %d\n",inp));

		switch(inp)
		{
		    case INP_CURSORLEFT:
			if (fh->inputpos > fh->inputstart)
			{
			    fh->inputpos--;
			    do_movecursor(conbase, fh, CUR_LEFT, 1);
			}
			break;

		    case INP_SHIFT_CURSORLEFT: /* move to beginning of line */
		    case INP_HOME:
			if (fh->inputpos > fh->inputstart)
			{
			    do_movecursor(conbase, fh, CUR_LEFT, fh->inputpos - fh->inputstart);
			    fh->inputpos = fh->inputstart;
			}
			break;

		    case INP_CURSORRIGHT:
			if (fh->inputpos < fh->inputsize)
			{
			    fh->inputpos++;
			    do_movecursor(conbase, fh, CUR_RIGHT, 1);
			}
			break;

		    case INP_SHIFT_CURSORRIGHT: /* move to end of line */
		    case INP_END:
			if (fh->inputpos != fh->inputsize)
			{
			    do_movecursor(conbase, fh, CUR_RIGHT, fh->inputsize - fh->inputpos);
			    fh->inputpos = fh->inputsize;
			}
			break;

		    case INP_CURSORUP: /* walk through cmd history */
		    case INP_CURSORDOWN:
		    case INP_SHIFT_CURSORUP:
		    case INP_SHIFT_CURSORDOWN:
			history_walk(conbase, fh, inp);
		    	break;
			
		    case INP_BACKSPACE:
			if (fh->inputpos > fh->inputstart)
			{
			    do_movecursor(conbase, fh, CUR_LEFT, 1);

			    if (fh->inputpos == fh->inputsize)
			    {
				do_deletechar(conbase, fh);

				fh->inputsize--;
				fh->inputpos--;
			    } else {
				WORD chars_right = fh->inputsize - fh->inputpos;

				fh->inputsize--;
				fh->inputpos--;

				do_cursorvisible(conbase, fh, FALSE);
				do_write(conbase, fh, &fh->inputbuffer[fh->inputpos + 1], chars_right);
				do_deletechar(conbase, fh);
				do_movecursor(conbase, fh, CUR_LEFT, chars_right);
				do_cursorvisible(conbase, fh, TRUE);

				memmove(&fh->inputbuffer[fh->inputpos], &fh->inputbuffer[fh->inputpos + 1], chars_right); 

			    }
			}
			break;

		    case INP_SHIFT_BACKSPACE:
		        if (fh->inputpos > fh->inputstart)
			{
			    do_movecursor(conbase, fh, CUR_LEFT, fh->inputpos - fh->inputstart);
			    if (fh->inputpos == fh->inputsize)
			    {
				do_eraseinline(conbase, fh);
								
			    	fh->inputpos = fh->inputsize = fh->inputstart;
			    } else {
			        WORD chars_right = fh->inputsize - fh->inputpos;
				
			        do_cursorvisible(conbase, fh, FALSE);
				do_write(conbase, fh, &fh->inputbuffer[fh->inputpos], chars_right);
				do_eraseinline(conbase, fh);
				do_movecursor(conbase, fh, CUR_LEFT, chars_right);
				do_cursorvisible(conbase, fh, TRUE);
								
				memmove(&fh->inputbuffer[fh->inputstart], &fh->inputbuffer[fh->inputpos], chars_right);

				fh->inputsize -= (fh->inputpos - fh->inputstart);
				fh->inputpos = fh->inputstart;	
			    }
			}
		    	break;
			
		    case INP_DELETE:
			if (fh->inputpos < fh->inputsize)
			{
			    fh->inputsize--;

			    if (fh->inputpos == fh->inputsize)
			    {
				do_deletechar(conbase, fh);
			    } else {
				WORD chars_right = fh->inputsize - fh->inputpos;

				do_cursorvisible(conbase, fh, FALSE);
				do_write(conbase, fh, &fh->inputbuffer[fh->inputpos + 1], chars_right);
				do_deletechar(conbase, fh);
				do_movecursor(conbase, fh, CUR_LEFT, chars_right);
				do_cursorvisible(conbase, fh, TRUE);

				memmove(&fh->inputbuffer[fh->inputpos], &fh->inputbuffer[fh->inputpos + 1], chars_right);
			    }
			}
			break;

		    case INP_SHIFT_DELETE:
		        if (fh->inputpos < fh->inputsize)
			{
			    fh->inputsize = fh->inputpos;
			    do_eraseinline(conbase, fh);
			}
		    	break;
			
		    case INP_CONTROL_X:
			if ((fh->inputsize - fh->inputstart) > 0)
			{
			    if (fh->inputpos > fh->inputstart)
			    {
				do_movecursor(conbase, fh, CUR_LEFT, fh->inputpos - fh->inputstart);
			    }
			    do_eraseinline(conbase, fh);

			    fh->inputpos = fh->inputsize = fh->inputstart;
			}
			break;

		    case INP_STRING:
			if (fh->inputsize < INPUTBUFFER_SIZE)
			{
			    do_write(conbase, fh, &c, 1);

			    if (fh->inputpos == fh->inputsize)
			    {
				fh->inputbuffer[fh->inputpos++] = c;
				fh->inputsize++;
			    } else {
				WORD chars_right = fh->inputsize - fh->inputpos;

				do_cursorvisible(conbase, fh, FALSE);
				do_write(conbase, fh, &fh->inputbuffer[fh->inputpos], chars_right);
				do_movecursor(conbase, fh, CUR_LEFT, chars_right);
				do_cursorvisible(conbase, fh, TRUE);

				memmove(&fh->inputbuffer[fh->inputpos + 1], &fh->inputbuffer[fh->inputpos], chars_right);		    
				fh->inputbuffer[fh->inputpos++] = c;
				fh->inputsize++;
			    }
			}
			break;

		    case INP_RETURN:
		        if (fh->inputsize < INPUTBUFFER_SIZE)
			{			    
		            c = '\n';
			    do_write(conbase, fh, &c, 1);
			    
			    add_to_history(conbase, fh);

			    fh->inputbuffer[fh->inputsize++] = '\n';
			    fh->inputstart = fh->inputsize;
			    fh->inputpos = fh->inputstart;
			    
			    if (fh->flags & FHFLG_READPENDING)
			    {			    
			        struct IOFileSys *iofs, *next_iofs;
    
				ForeachNodeSafe(&fh->pendingReads, iofs, next_iofs)
				{
        			    Remove((struct Node *)iofs);
				    answer_read_request(conbase, fh, iofs);
				    
				    if (fh->inputsize == 0) break;

				} /* ForeachNodeSafe(&fh->pendingReads, iofs, nextiofs) */

				if (IsListEmpty(&fh->pendingReads)) fh->flags &= ~FHFLG_READPENDING;
			    }
			    
			    if (fh->inputsize)
			    {
			        fh->flags |= FHFLG_CANREAD;
				fh->canreadsize = fh->inputsize;
			    }
			}
			break;

		    case INP_LINEFEED:
		        if (fh->inputsize < INPUTBUFFER_SIZE)
			{
			    c = '\n';
			    do_write(conbase, fh, &c, 1);
			    
			    add_to_history(conbase, fh);
			    
			    fh->inputbuffer[fh->inputsize++] = c;
			    fh->inputstart = fh->inputsize;
			    fh->inputpos = fh->inputsize;
			}
		    	break;
		
		    case INP_CTRL_C:
		    case INP_CTRL_D:
		    case INP_CTRL_E:
		    case INP_CTRL_F:
		        if (fh->breaktask)
			{
			    Signal(fh->breaktask, 1L << (12 + inp - INP_CTRL_C));
			}
		    	break;
			
		} /* switch(inp) */

	    } /* while((inp = scan_input(conbase, fh, &c)) != INP_DONE) */

	    /* wait for next input from console.device */

	    fh->conreadio->io_Command = CMD_READ;
	    fh->conreadio->io_Data    = fh->consolebuffer;
	    fh->conreadio->io_Length  = CONSOLEBUFFER_SIZE;

	    SendIO(ioReq(fh->conreadio));
	    
	    /* pending writes ? */
	    
	    if ((fh->flags & FHFLG_WRITEPENDING) && (fh->inputpos == fh->inputstart))
	    {
	        struct IOFileSys *iofs, *iofs_next;
		
		ForeachNodeSafe(&fh->pendingWrites, iofs, iofs_next)
		{
		    Remove((struct Node *)iofs);
		    
		    answer_write_request(conbase, fh, iofs);
		}
		
		fh->flags &= ~FHFLG_WRITEPENDING;
	    }
	    

	} /* if (sigs & conmask) */

    } /* for(;;) */

    /* this point must never be reached */
}

static const char end=0;
