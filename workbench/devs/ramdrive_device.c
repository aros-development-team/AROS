/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************************/

#define NUM_HEADS   2
#define NUM_CYL     80
#define NUM_SECS    11
#define BLOCKSIZE   512

#define DISKSIZE    (NUM_HEADS * NUM_CYL * NUM_SECS * BLOCKSIZE)
#define NUM_TRACKS  (NUM_CYL * NUM_HEADS)

/****************************************************************************************/

#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <exec/resident.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/initializers.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <aros/asmcall.h>
#include <aros/macros.h>
#include <string.h>

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#include "ramdrive_device_gcc.h"
#endif

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

#define NEWSTYLE_DEVICE 1

/****************************************************************************************/

#define NEWLIST(l)                          	\
((l)->lh_Head = (struct Node *)&(l)->lh_Tail, 	\
 (l)->lh_Tail = NULL,                         	\
 (l)->lh_TailPred = (struct Node *)(l))

/****************************************************************************************/

extern const char 	name[];
extern const char 	version[];
extern const APTR 	inittabl[4];
extern void *const 	functable[];
extern const UBYTE 	datatable;
extern struct ramdrivebase *AROS_SLIB_ENTRY(init, ramdrive)();
extern void 		AROS_SLIB_ENTRY(open, ramdrive)();
extern BPTR 		AROS_SLIB_ENTRY(close, ramdrive)();
extern BPTR 		AROS_SLIB_ENTRY(expunge, ramdrive)();
extern int 		AROS_SLIB_ENTRY(null, ramdrive)();
extern void 		AROS_SLIB_ENTRY(beginio, ramdrive)();
extern LONG 		AROS_SLIB_ENTRY(abortio, ramdrive)();
extern STRPTR 		AROS_SLIB_ENTRY(killrad0, ramdrive)();
extern STRPTR 		AROS_SLIB_ENTRY(killrad, ramdrive)();
extern const char 	end;

/****************************************************************************************/

int entry(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

/****************************************************************************************/

const struct Resident resident = 
{
    RTC_MATCHWORD, 
    (struct Resident *)&resident, 
    (APTR)&end, 
    RTF_AUTOINIT, 
    41, 
    NT_DEVICE, 
    0, 
    (char *)name, 
    (char *)&version[6], 
    (ULONG *)inittabl
};

const char name[] = "ramdrive.device";

const char version[] = "$VER: ramdrive.device 41.1 (19.07.2001)\r\n";

const APTR inittabl[4] = 
{
    (APTR)sizeof(struct ramdrivebase), 
    (APTR)functable, 
    (APTR)&datatable, 
    &AROS_SLIB_ENTRY(init, ramdrive)
};

void *const functable[] = 
{
    &AROS_SLIB_ENTRY(open, ramdrive), 
    &AROS_SLIB_ENTRY(close, ramdrive), 
    &AROS_SLIB_ENTRY(expunge, ramdrive), 
    &AROS_SLIB_ENTRY(null, ramdrive), 
    &AROS_SLIB_ENTRY(beginio, ramdrive), 
    &AROS_SLIB_ENTRY(abortio, ramdrive), 
    &AROS_SLIB_ENTRY(killrad0, ramdrive), 
    &AROS_SLIB_ENTRY(killrad, ramdrive), 
    (void *)-1
};

const UBYTE datatable = 0;

/****************************************************************************************/

#if NEWSTYLE_DEVICE

static const UWORD SupportedCommands[] =
{
    CMD_FLUSH,
    CMD_READ,
    CMD_WRITE,
    CMD_UPDATE,
    CMD_CLEAR,
    TD_CHANGENUM,
    TD_CHANGESTATE,
    TD_PROTSTATUS,
    TD_REMOVE,
    TD_ADDCHANGEINT,
    TD_REMCHANGEINT,
    TD_GETDRIVETYPE,
    TD_GETNUMTRACKS,    
    TD_FORMAT,
    TD_RAWREAD,
    TD_RAWWRITE,
    TD_SEEK,
    TD_MOTOR,
    ETD_READ,
    ETD_WRITE,
    ETD_UPDATE,
    ETD_CLEAR,
    ETD_MOTOR,
    ETD_SEEK,
    ETD_FORMAT,
    ETD_RAWREAD,
    ETD_RAWWRITE,
    NSCMD_DEVICEQUERY,
    0
};

#endif

/****************************************************************************************/

static void FormatOFS(UBYTE *mem, ULONG number, struct unit *unit);

/****************************************************************************************/

AROS_UFH3(struct ramdrivebase *, AROS_SLIB_ENTRY(init,ramdrive),
    AROS_UFHA(struct ramdrivebase *, ramdrivebase, D0),
    AROS_UFHA(BPTR, segList, A0),
    AROS_UFHA(struct ExecBase *, sysbase, A6)
)
{
    AROS_USERFUNC_INIT

    /* Store arguments */
    SysBase = sysbase;

    D(bug("ramdrive_device: in libinit func\n"));

    ramdrivebase->seglist = segList;
    InitSemaphore(&ramdrivebase->sigsem);
    NEWLIST((struct List *)&ramdrivebase->units);
    ramdrivebase->port.mp_Node.ln_Type = NT_MSGPORT;
    ramdrivebase->port.mp_Flags = PA_SIGNAL;
    ramdrivebase->port.mp_SigBit = SIGB_SINGLE;
    NEWLIST((struct List *)&ramdrivebase->port.mp_MsgList);
    
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37);
    
    if(DOSBase != NULL)
    {
    	D(bug("ramdrive_device: in libinit func. Returning %x (success) :-)\n", ramdrivebase));
    	return ramdrivebase;
    }
    
    D(bug("ramdrive_device: in libinit func. Returning NULL (failure) :-(\n"));
    
    return NULL;
    
    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

AROS_UFP3(LONG, unitentry,
    AROS_UFPA(STRPTR, argstr, A0),
    AROS_UFPA(ULONG, arglen, D0),
    AROS_UFPA(struct ExecBase *, SysBase, A6));
 
/****************************************************************************************/

AROS_LH3(void, open, 
    AROS_LHA(struct IOExtTD *, iotd, A1), 
    AROS_LHA(ULONG, unitnum, D0), 
    AROS_LHA(ULONG, flags, D1), 
    struct ramdrivebase *, ramdrivebase, 1, ramdrive)
{
    AROS_LIBFUNC_INIT

    static const struct TagItem tags[] = 
    {
    	{ NP_Name	, (IPTR)"Ram Drive Unit Process"}, 
    	{ NP_Input	, 0 				}, 
    	{ NP_Output	, 0 				}, 
    	{ NP_Error	, 0 				}, 
    	{ NP_CurrentDir	, 0 				}, 
    	{ NP_Priority	, 0 				}, 
    	{ NP_HomeDir	, 0 				}, 
    	{ NP_CopyVars	, 0 				}, 
    	{ NP_Entry	, (IPTR)unitentry		}, 
    	{ TAG_END	, 0 				}
    };
    struct unit *unit;

    D(bug("ramdrive_device: in libopen func.\n"));

    if (iotd->iotd_Req.io_Message.mn_Length < sizeof(struct IOExtTD))
    {
	D(bug("ramdrive.device/open: IORequest structure passed to OpenDevice is too small!\n"));
	iotd->iotd_Req.io_Error = IOERR_OPENFAIL;
	return;
    }

    /* Keep compiler happy */
    flags = 0;

    /* I have one more opener. */
    ramdrivebase->device.dd_Library.lib_OpenCnt++;
    ramdrivebase->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;

    D(bug("ramdrive_device: in libopen func. Looking if unit is already open\n"));

    ObtainSemaphore(&ramdrivebase->sigsem);

    for(unit = (struct unit *)ramdrivebase->units.mlh_Head;
	unit->msg.mn_Node.ln_Succ != NULL;
	unit = (struct unit *)unit->msg.mn_Node.ln_Succ)
	if(unit->unitnum == unitnum)
	{
	    unit->usecount++;
	    ReleaseSemaphore(&ramdrivebase->sigsem);
	    
	    iotd->iotd_Req.io_Unit 		      = (struct Unit *)unit;
	    iotd->iotd_Req.io_Error 		      = 0;
	    iotd->iotd_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
   	   
	    D(bug("ramdrive_device: in libopen func. Yep. Unit is already open\n"));
	    
	    return;
	}

    D(bug("ramdrive_device: in libopen func. No, it is not. So creating new unit ...\n"));

    unit = (struct unit *)AllocMem(sizeof(struct unit), MEMF_PUBLIC);
    if(unit != NULL)
    {
        D(bug("ramdrive_device: in libopen func. Allocation of unit memory okay. Setting up unit and calling CreateNewProc ...\n"));

	unit->usecount 			= 1;
	unit->ramdrivebase 		= ramdrivebase;
	unit->unitnum 			= unitnum;
	unit->msg.mn_ReplyPort 		= &ramdrivebase->port;
	unit->msg.mn_Length 		= sizeof(struct unit);
	unit->port.mp_Node.ln_Type 	= NT_MSGPORT;
	unit->port.mp_Flags 	 	= PA_IGNORE;
	unit->port.mp_SigTask 		= CreateNewProc((struct TagItem *)tags);

        D(bug("ramdrive_device: in libopen func. CreateNewProc called. Proc = %x\n", unit->port.mp_SigTask));
	
	if(unit->port.mp_SigTask != NULL)
	{
	    NEWLIST((struct List *)&unit->port.mp_MsgList);

	    /* setup replyport to point to active task */
	    ramdrivebase->port.mp_SigTask = FindTask(NULL);
    	    SetSignal(0, SIGF_SINGLE);
	    
    	    D(bug("ramdrive_device: in libopen func. Sending startup msg\n"));
	    PutMsg(&((struct Process *)unit->port.mp_SigTask)->pr_MsgPort, &unit->msg);

    	    D(bug("ramdrive_device: in libopen func. Waiting for replymsg\n"));
	    WaitPort(&ramdrivebase->port);
	    (void)GetMsg(&ramdrivebase->port);
    	    D(bug("ramdrive_device: in libopen func. Received replymsg\n"));
	    
	    if(unit->mem)
	    {
		AddTail((struct List *)&ramdrivebase->units, &unit->msg.mn_Node);
		iotd->iotd_Req.io_Unit = (struct Unit *)unit;
		/* Set returncode */
		iotd->iotd_Req.io_Error = 0;
		ReleaseSemaphore(&ramdrivebase->sigsem);
		ramdrivebase->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;
		return;
	    }else
		iotd->iotd_Req.io_Error = TDERR_NotSpecified;
	}else
	    iotd->iotd_Req.io_Error = TDERR_NoMem;
	FreeMem(unit, sizeof(struct unit));
    }else
	iotd->iotd_Req.io_Error = TDERR_NoMem;

    ReleaseSemaphore(&ramdrivebase->sigsem);

    ramdrivebase->device.dd_Library.lib_OpenCnt--;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(BPTR, close, 
    AROS_LHA(struct IOExtTD *, iotd, A1), 
    struct ramdrivebase *, ramdrivebase, 2, ramdrive)
{
    AROS_LIBFUNC_INIT
    struct unit *unit;

    /* Let any following attemps to use the device crash hard. */
    iotd->iotd_Req.io_Device = (struct Device *)-1;

    ObtainSemaphore(&ramdrivebase->sigsem);
    unit = (struct unit *)iotd->iotd_Req.io_Unit;
    if(!--unit->usecount)
    {
	Remove(&unit->msg.mn_Node);
	ramdrivebase->port.mp_SigTask = FindTask(NULL);
	SetSignal(0, SIGF_SINGLE);
	PutMsg(&unit->port, &unit->msg);
	WaitPort(&ramdrivebase->port);
	(void)GetMsg(&ramdrivebase->port);
	FreeMem(unit, sizeof(struct unit));
    }
    ReleaseSemaphore(&ramdrivebase->sigsem);

    /* I have one fewer opener. */
    if(!--ramdrivebase->device.dd_Library.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(ramdrivebase->device.dd_Library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the device */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, expunge, struct ramdrivebase *, ramdrivebase, 3, ramdrive)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(ramdrivebase->device.dd_Library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	ramdrivebase->device.dd_Library.lib_Flags |= LIBF_DELEXP;
	return 0;
    }

    /* Free resources */
    CloseLibrary((struct Library *)DOSBase);

    /* Get rid of the device. Remove it from the list. */
    Remove(&ramdrivebase->device.dd_Library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret = ramdrivebase->seglist;

    /* Free the memory. */
    FreeMem((char *)ramdrivebase-ramdrivebase->device.dd_Library.lib_NegSize, 
	    ramdrivebase->device.dd_Library.lib_NegSize+ramdrivebase->device.dd_Library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0I(int, null, struct ramdrivebase *, ramdrivebase, 4, ramdrive)
{
    AROS_LIBFUNC_INIT
    
    return 0;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(void, beginio, 
    AROS_LHA(struct IOExtTD *, iotd, A1), 
    struct ramdrivebase *, ramdrivebase, 5, ramdrive)
{
    AROS_LIBFUNC_INIT

    switch(iotd->iotd_Req.io_Command)
    {
#if NEWSTYLE_DEVICE
      	case NSCMD_DEVICEQUERY:
	    if(iotd->iotd_Req.io_Length < ((LONG)OFFSET(NSDeviceQueryResult, SupportedCommands)) + sizeof(UWORD *))
	    {
		iotd->iotd_Req.io_Error = IOERR_BADLENGTH;
	    }
	    else
	    {
		struct NSDeviceQueryResult *d;

		d = (struct NSDeviceQueryResult *)iotd->iotd_Req.io_Data;

		d->DevQueryFormat 	    = 0;
		d->SizeAvailable 	    = sizeof(struct NSDeviceQueryResult);
		d->DeviceType 	    = NSDEVTYPE_TRACKDISK;
		d->DeviceSubType 	    = 0;
		d->SupportedCommands    = (UWORD *)SupportedCommands;

		iotd->iotd_Req.io_Actual = sizeof(struct NSDeviceQueryResult);
		iotd->iotd_Req.io_Error  = 0;

	    }
	    break;
#endif
 
    	case TD_CHANGENUM:
	    /* result: io_Actual = disk change counter */
	    
	case TD_CHANGESTATE:
	    /* result: io_Actual = disk presence indicator (0 = disk is in drive) */
	    
	case TD_PROTSTATUS:
	    /* result: io_Actual = disk protection status (0 = not protected) */
	    
	    iotd->iotd_Req.io_Actual = 0;
	    iotd->iotd_Req.io_Error = 0;
	    break;
	    
	case ETD_UPDATE:
	case CMD_UPDATE:
	case ETD_CLEAR:
	case CMD_CLEAR:
	case TD_REMOVE:
	case TD_ADDCHANGEINT:
	case TD_REMCHANGEINT:
	    /* Ignore but don't fail */
	    iotd->iotd_Req.io_Error = 0;
	    break;
	
	case TD_GETDRIVETYPE:
	    iotd->iotd_Req.io_Actual = DRIVE3_5;
	    iotd->iotd_Req.io_Error = 0;
	    break;
	    
	case TD_GETNUMTRACKS:
	    iotd->iotd_Req.io_Actual = NUM_TRACKS;
	    iotd->iotd_Req.io_Error = 0;
	    break;
	    
	case CMD_FLUSH:
	    {
		struct IOExtTD *flushed_iotd;
    	    	struct unit    *u =(struct unit *)iotd->iotd_Req.io_Unit;
		Forbid();
		while((flushed_iotd = (struct IOExtTD *)GetMsg(&u->port)))
		{
	    	    flushed_iotd->iotd_Req.io_Error = IOERR_ABORTED;
		    ReplyMsg(&flushed_iotd->iotd_Req.io_Message);
		}
		Permit();
	    }
	    break;
	   
	case ETD_READ:
	case CMD_READ:
	case ETD_WRITE:
	case CMD_WRITE:
	case ETD_FORMAT:
	case TD_FORMAT:
	case ETD_RAWREAD:
	case TD_RAWREAD:
	case ETD_RAWWRITE:
	case TD_RAWWRITE:
	case ETD_SEEK:
	case TD_SEEK:
	case ETD_MOTOR:
	case TD_MOTOR:
	    /* Not done quick */
	    iotd->iotd_Req.io_Flags &= ~IOF_QUICK;

	    /* Forward to unit thread */
	    PutMsg(&((struct unit *)iotd->iotd_Req.io_Unit)->port, 
		   &iotd->iotd_Req.io_Message);
	    return;
	    
	default:
	    /* Not supported */
	    iotd->iotd_Req.io_Error = IOERR_NOCMD;
	    break;
	    
    } /* switch(iotd->iotd_Req.io_Command) */

    /* WaitIO will look into this */
    iotd->iotd_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;

    /* Finish message */
    if(!(iotd->iotd_Req.io_Flags&IOF_QUICK))
	ReplyMsg(&iotd->iotd_Req.io_Message);

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(LONG, abortio, 
 AROS_LHA(struct IOExtTD *, iotd, A1), 
	   struct ramdrivebase *, ramdrivebase, 6, ramdrive)
{
    AROS_LIBFUNC_INIT
    
    return IOERR_NOCMD;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(STRPTR, killrad0, 
	   struct ramdrivebase *, ramdrivebase, 7, ramdrive)
{
    AROS_LIBFUNC_INIT

#warning KillRAD0 not implemented yet

    return 0;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(STRPTR, killrad, 
 AROS_LHA(ULONG, unit, D0), 
	   struct ramdrivebase *, ramdrivebase, 8, ramdrive)
{
    AROS_LIBFUNC_INIT

#warning KillRAD not implemented yet
 
    return 0;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

#define ramdrivebase unit->ramdrivebase

/****************************************************************************************/

static LONG read(struct unit *unit, struct IOExtTD *iotd)
{
    STRPTR 	buf;
    ULONG 	offset, size;
    
    D(bug("ramdrive_device/read: offset = %d  size = %d\n", iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Length));
    
    if(iotd->iotd_SecLabel)
    {
        D(bug("ramdrive_device/read: iotd->iotd_SecLabel is != NULL -> returning IOERR_NOCMD\n"));
	return IOERR_NOCMD;
    }

    buf    = iotd->iotd_Req.io_Data;
    offset = iotd->iotd_Req.io_Offset;
    size   = iotd->iotd_Req.io_Length;

    unit->headpos = offset;
    
    if (offset + size > DISKSIZE)
    {
        D(bug("ramdrive_device/read: Seek to offset %d failed. Returning TDERR_SeekError\n", offset));
	return TDERR_SeekError;
    }
    
    CopyMem(&unit->mem[offset], buf, size);
	    
    iotd->iotd_Req.io_Actual = size;

#if DEBUG
    buf = iotd->iotd_Req.io_Data;
    D(bug("ramdrive_device/read: returning 0. First 4 buffer bytes = [%c%c%c%c]\n", buf[0], buf[1], buf[2], buf[3]));
#endif

    return 0;
}

/****************************************************************************************/

static LONG write(struct unit *unit, struct IOExtTD *iotd)
{
    STRPTR 	buf;
    ULONG 	offset, size;
    
    if(iotd->iotd_SecLabel)
	return IOERR_NOCMD;
	
    buf    = iotd->iotd_Req.io_Data;
    offset = iotd->iotd_Req.io_Offset;
    size   = iotd->iotd_Req.io_Length;
    
    unit->headpos = offset;
    
    if (offset + size > DISKSIZE)
    {
        D(bug("ramdrive_device/write: Seek to offset %d failed. Returning TDERR_SeekError\n", offset));
	return TDERR_SeekError;
    }

    iotd->iotd_Req.io_Actual = size;
    
    CopyMem(buf, &unit->mem[offset], size);
    
    return 0;
}

/****************************************************************************************/

#ifdef SysBase
#undef SysBase
#endif

/****************************************************************************************/

AROS_UFH3(LONG, unitentry,
 AROS_UFHA(STRPTR, argstr, A0),
 AROS_UFHA(ULONG, arglen, D0),
 AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    struct Process 	*me;
    LONG 		err = 0L;
    struct IOExtTD 	*iotd;
    struct unit 	*unit;

#ifdef _AMIGA
    SysBase = *((struct ExecBase **)4);
#endif

    D(bug("ramdrive_device/unitentry: just started\n"));
    
    me = (struct Process *)FindTask(NULL);

    WaitPort(&me->pr_MsgPort);
    unit = (struct unit *)GetMsg(&me->pr_MsgPort);
    unit->port.mp_SigBit = AllocSignal(-1);
    unit->port.mp_Flags = PA_SIGNAL;

    D(bug("ramdrive_device/unitentry: Trying to allocate memory disk\n"));

    unit->mem = AllocVec(DISKSIZE, MEMF_PUBLIC | MEMF_CLEAR);
    if(!unit->mem)
    {
        D(bug("ramdrive_device/unitentry: Memory allocation failed :-( Replying startup msg.\n"));

	Forbid();
	ReplyMsg(&unit->msg);
	return 0;
    }

    D(bug("ramdrive_device/unitentry: Memory allocation okay :-) Replying startup msg.\n"));

    FormatOFS(unit->mem, unit->unitnum, unit);
    
    ReplyMsg(&unit->msg);

    D(bug("ramdrive_device/unitentry: Now entering main loop\n"));

    for(;;)
    {
	while((iotd = (struct IOExtTD *)GetMsg(&unit->port)) != NULL)
	{
	    if(&iotd->iotd_Req.io_Message == &unit->msg)
	    {
    		D(bug("ramdrive_device/unitentry: Recevied EXIT message.\n"));

		FreeVec(unit->mem);
		Forbid();
		ReplyMsg(&unit->msg);
		return 0;
	    }

 	    switch(iotd->iotd_Req.io_Command)
 	    {
	    	case ETD_RAWREAD:
	    	case TD_RAWREAD:
	    	    /*
		    ** same as CMD_READ, but offset does not have to be multiple of
		    ** BLOCKSIZE
		    **
		    ** fall through
		    */
		
		case ETD_READ:	    
 		case CMD_READ:
     		    D(bug("ramdrive_device/unitentry: received CMD_READ.\n"));
		    err = read(unit, iotd);
 		    break;
		
		case ETD_RAWWRITE:    
	    	case TD_RAWWRITE:
	    	    /*
		    ** same as CMD_WRITE, but offset does not have to be multiple of
		    ** BLOCKSIZE
		    **
		    ** fall through
		    */
		
		case ETD_WRITE:
 		case CMD_WRITE:
		case ETD_FORMAT:
 		case TD_FORMAT:
    		    D(bug("ramdrive_device/unitentry: received %s\n", (iotd->iotd_Req.io_Command == CMD_WRITE) ? "CMD_WRITE" : "TD_FORMAT"));
 		    err = write(unit, iotd);
 		    break;
		    
		case ETD_MOTOR:
		case TD_MOTOR:
		    /*
		    ** DOS wants the previous state in io_Actual.
		    ** We return "!io_Actual"
		    */
		    
		    iotd->iotd_Req.io_Actual = (iotd->iotd_Req.io_Actual == 1) ? 0 : 1;
		    err = 0;
		    break;
		    
		case ETD_SEEK:
		case TD_SEEK:
		    unit->headpos = iotd->iotd_Req.io_Actual;
		    err = 0;
		    break;
		    
 	    } /* switch(iotd->iotd_Req.io_Command) */
	    
 	    iotd->iotd_Req.io_Error = err;
 	    ReplyMsg(&iotd->iotd_Req.io_Message);
	    
	} /* while((iotd = (struct IOExtTD *)GetMsg(&unit->port)) != NULL) */
	
	WaitPort(&unit->port);
	
    } /* for(;;) */
}

/****************************************************************************************/

/* The following routines are based on TurboDevice by Thomas Dreibholz */

/****************************************************************************************/

static ULONG CalcRootBlock(void)
{
    return NUM_CYL * NUM_HEADS * NUM_SECS / 2;
}

/****************************************************************************************/

static ULONG CalcBitMap(void)
{
    return CalcRootBlock() + 1;
}

/****************************************************************************************/

VOID RootBlockCheckSum(UBYTE *buf)
{
    LONG checksum, *long_ptr;
    LONG  i;
    
    long_ptr = (ULONG *)buf;
    checksum = 0;
    
    for(i = 0; i < TD_SECTOR / 4; i++)
    {
        checksum += AROS_BE2LONG(long_ptr[i]);
    }
    long_ptr[5] = AROS_LONG2BE(-checksum);
}

/****************************************************************************************/

VOID CalcBitMapCheckSum(UBYTE *buf)
{
    LONG checksum, i;
    LONG *long_ptr = (LONG *)buf;

    for(i = 1, checksum = 0; i < TD_SECTOR / 4; i++)
    {
	checksum += AROS_BE2LONG(long_ptr[i]);
    }
    long_ptr[0] = AROS_LONG2BE(-checksum);
}

/****************************************************************************************/

VOID InstallRootBlock(UBYTE *buf, STRPTR diskname, ULONG bitmap,
    	    	      struct unit *unit)
{
    struct DateStamp ds;    
    ULONG   	     *long_ptr;
    LONG    	     i;
    
    long_ptr 	  = (ULONG *)buf;
    long_ptr[0]   = AROS_LONG2BE(2);
    long_ptr[3]   = AROS_LONG2BE(72);
    long_ptr[78]  = AROS_LONG2BE(-1);
    long_ptr[79]  = AROS_LONG2BE(bitmap);
    long_ptr[127] = AROS_LONG2BE(1);
    
    DateStamp(&ds);
    
    long_ptr[121] = AROS_LONG2BE(ds.ds_Days);
    long_ptr[122] = AROS_LONG2BE(ds.ds_Minute);
    long_ptr[123] = AROS_LONG2BE(ds.ds_Tick);

    long_ptr[105] = AROS_LONG2BE(ds.ds_Days);
    long_ptr[106] = AROS_LONG2BE(ds.ds_Minute);
    long_ptr[107] = AROS_LONG2BE(ds.ds_Tick);
    
    buf[432] = (UBYTE)strlen(diskname);
    
    for(i = 0; i < strlen(diskname); i++)
    {
	buf[433+i] = diskname[i];
    }
    
    RootBlockCheckSum(buf);
}

/****************************************************************************************/

static ULONG CalcBlocks(void)
{
    return NUM_CYL * NUM_HEADS * NUM_SECS - 1;
}

/****************************************************************************************/

static void AllocBitMapBlock(LONG block, UBYTE *buf)
{
    ULONG *long_ptr = (ULONG *)buf;
    LONG  longword, bit;
    LONG  old_long, new_long;
    
    longword = (block - 2) / 32;
    bit = block - 2 - longword * 32;
    old_long = AROS_BE2LONG(long_ptr[longword + 1]);
    new_long = old_long & (0xFFFFFFFF - (1L << bit));
    
    long_ptr[longword + 1] = AROS_LONG2BE(new_long);
}

/****************************************************************************************/

static void FreeBitMapBlock(LONG block, UBYTE *buf)
{
    ULONG *long_ptr = (ULONG *)buf;
    LONG  longword, bit;
    LONG  old_long, new_long;
    
    longword = (block - 2) / 32;
    bit = block - 2 - longword * 32;
    old_long = AROS_BE2LONG(long_ptr[longword + 1]);
    new_long = old_long | (1L << bit);
    
    long_ptr[longword + 1] = AROS_LONG2BE(new_long);
}

/****************************************************************************************/

static void FormatOFS(UBYTE *mem, ULONG number, struct unit *unit)
{
    ULONG a,b,c,d;
    UBYTE *cmem;
    UBYTE Name[6];

    mem[0]='D';
    mem[1]='O';
    mem[2]='S';
    mem[3]=0x00;

    a = CalcRootBlock();
    b = CalcBitMap();

    cmem = mem + (a * TD_SECTOR);
    strcpy(Name, "RAM_#");
    Name[4] = '0' + number;

    InstallRootBlock(cmem, Name, b, unit);
    cmem = mem + (b * TD_SECTOR);
    d = CalcBlocks();
    for(c = 2; c <= d; c++)
    {
	FreeBitMapBlock(c, cmem);
    }
    
    AllocBitMapBlock(a, cmem);
    AllocBitMapBlock(b, cmem);
    
    CalcBitMapCheckSum(cmem);
}

/****************************************************************************************/

const char end = 0;

/****************************************************************************************/
