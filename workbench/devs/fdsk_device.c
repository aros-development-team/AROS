/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************************/

#include <devices/trackdisk.h>
#include <exec/resident.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <aros/asmcall.h>
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#include "fdsk_device_gcc.h"
#endif

#define DEBUG 1
#include <aros/debug.h>

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
extern struct fdskbase *AROS_SLIB_ENTRY(init, fdsk)();
extern void 		AROS_SLIB_ENTRY(open, fdsk)();
extern BPTR 		AROS_SLIB_ENTRY(close, fdsk)();
extern BPTR 		AROS_SLIB_ENTRY(expunge, fdsk)();
extern int 		AROS_SLIB_ENTRY(null, fdsk)();
extern void 		AROS_SLIB_ENTRY(beginio, fdsk)();
extern LONG 		AROS_SLIB_ENTRY(abortio, fdsk)();
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

const char name[] = "fdsk.device";

const char version[] = "$VER: file-disk device 41.1 (10.9.96)\r\n";

const APTR inittabl[4] = 
{
    (APTR)sizeof(struct fdskbase), 
    (APTR)functable, 
    (APTR)&datatable, 
    &AROS_SLIB_ENTRY(init, fdsk)
};

void *const functable[] = 
{
    &AROS_SLIB_ENTRY(open, fdsk), 
    &AROS_SLIB_ENTRY(close, fdsk), 
    &AROS_SLIB_ENTRY(expunge, fdsk), 
    &AROS_SLIB_ENTRY(null, fdsk), 
    &AROS_SLIB_ENTRY(beginio, fdsk), 
    &AROS_SLIB_ENTRY(abortio, fdsk), 
    (void *)-1
};

const UBYTE datatable = 0;

/****************************************************************************************/

AROS_UFH3(struct fdskbase *, AROS_SLIB_ENTRY(init,fdsk),
 AROS_UFHA(struct fdskbase *, fdskbase, D0),
 AROS_UFHA(BPTR,             segList,   A0),
 AROS_UFHA(struct ExecBase *, sysbase,  A6)
)
{
    AROS_USERFUNC_INIT

    /* Store arguments */
    SysBase = sysbase;

    D(bug("fdsk_device: in libinit func\n"));

    fdskbase->seglist = segList;
    InitSemaphore(&fdskbase->sigsem);
    NEWLIST((struct List *)&fdskbase->units);
    fdskbase->port.mp_Node.ln_Type = NT_MSGPORT;
    fdskbase->port.mp_Flags = PA_SIGNAL;
    fdskbase->port.mp_SigBit = SIGB_SINGLE;
    NEWLIST((struct List *)&fdskbase->port.mp_MsgList);
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37);
    
    if(DOSBase != NULL)
    {
    	D(bug("fdsk_device: in libinit func. Returning %x (success) :-)\n", fdskbase));
	return fdskbase;
    }
    
    D(bug("fdsk_device: in libinit func. Returning NULL (failure) :-(\n"));

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
 AROS_LHA(ULONG,              unitnum, D0), 
 AROS_LHA(ULONG,              flags, D1), 
	   struct fdskbase *, fdskbase, 1, fdsk)
{
    AROS_LIBFUNC_INIT

    static const struct TagItem tags[] = 
    {
    	{ NP_Name	, (IPTR)"File Disk Unit Process"}, 
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

    D(bug("fdsk_device: in libopen func.\n"));

    /* Keep compiler happy */
    flags = 0;

    /* I have one more opener. */
    fdskbase->device.dd_Library.lib_OpenCnt++;
    fdskbase->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;

    D(bug("fdsk_device: in libopen func. Looking if unit is already open\n"));

    ObtainSemaphore(&fdskbase->sigsem);

    for(unit = (struct unit *)fdskbase->units.mlh_Head;
	unit->msg.mn_Node.ln_Succ != NULL;
	unit = (struct unit *)unit->msg.mn_Node.ln_Succ)
	if(unit->unitnum == unitnum)
	{
	    unit->usecount++;
	    ReleaseSemaphore(&fdskbase->sigsem);
	    
	    iotd->iotd_Req.io_Unit 		      = (struct Unit *)unit;
	    iotd->iotd_Req.io_Error 		      = 0;
	    iotd->iotd_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
   	   
	    D(bug("fdsk_device: in libopen func. Yep. Unit is already open\n"));
	    
	    return;
	}

    D(bug("fdsk_device: in libopen func. No, it is not. So creating new unit ...\n"));

    unit = (struct unit *)AllocMem(sizeof(struct unit), MEMF_PUBLIC);
    if(unit != NULL)
    {
        D(bug("fdsk_device: in libopen func. Allocation of unit memory okay. Setting up unit and calling CreateNewProc ...\n"));

	unit->usecount 			= 1;
	unit->fdskbase 			= fdskbase;
	unit->unitnum 			= unitnum;
	unit->msg.mn_ReplyPort 		= &fdskbase->port;
	unit->msg.mn_Length 		= sizeof(struct unit);
	unit->port.mp_Node.ln_Type 	= NT_MSGPORT;
	unit->port.mp_Flags 	 	= PA_IGNORE;
	unit->port.mp_SigTask 		= CreateNewProc((struct TagItem *)tags);

        D(bug("fdsk_device: in libopen func. CreateNewProc called. Proc = %x\n", unit->port.mp_SigTask));
	
	if(unit->port.mp_SigTask != NULL)
	{
	    NEWLIST((struct List *)&unit->port.mp_MsgList);

	    /* setup replyport to point to active task */
	    fdskbase->port.mp_SigTask = FindTask(NULL);
    	    SetSignal(0, SIGF_SINGLE);
	    
    	    D(bug("fdsk_device: in libopen func. Sending startup msg\n"));
	    PutMsg(&((struct Process *)unit->port.mp_SigTask)->pr_MsgPort, &unit->msg);

    	    D(bug("fdsk_device: in libopen func. Waiting for replymsg\n"));
	    WaitPort(&fdskbase->port);
	    (void)GetMsg(&fdskbase->port);
    	    D(bug("fdsk_device: in libopen func. Received replymsg\n"));
	    
	    if(unit->file)
	    {
		AddTail((struct List *)&fdskbase->units, &unit->msg.mn_Node);
		iotd->iotd_Req.io_Unit = (struct Unit *)unit;
		/* Set returncode */
		iotd->iotd_Req.io_Error = 0;
		ReleaseSemaphore(&fdskbase->sigsem);
		fdskbase->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;
		return;
	    }else
		iotd->iotd_Req.io_Error = TDERR_NotSpecified;
	}else
	    iotd->iotd_Req.io_Error = TDERR_NoMem;
	FreeMem(unit, sizeof(struct unit));
    }else
	iotd->iotd_Req.io_Error = TDERR_NoMem;

    ReleaseSemaphore(&fdskbase->sigsem);

    fdskbase->device.dd_Library.lib_OpenCnt--;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(BPTR, close, 
 AROS_LHA(struct IOExtTD *, iotd, A1), 
	   struct fdskbase *, fdskbase, 2, fdsk)
{
    AROS_LIBFUNC_INIT
    struct unit *unit;

    /* Let any following attemps to use the device crash hard. */
    iotd->iotd_Req.io_Device = (struct Device *)-1;

    ObtainSemaphore(&fdskbase->sigsem);
    unit = (struct unit *)iotd->iotd_Req.io_Unit;
    if(!--unit->usecount)
    {
	Remove(&unit->msg.mn_Node);
	fdskbase->port.mp_SigTask = FindTask(NULL);
	SetSignal(0, SIGF_SINGLE);
	PutMsg(&unit->port, &unit->msg);
	WaitPort(&fdskbase->port);
	(void)GetMsg(&fdskbase->port);
	FreeMem(unit, sizeof(struct unit));
    }
    ReleaseSemaphore(&fdskbase->sigsem);

    /* I have one fewer opener. */
    if(!--fdskbase->device.dd_Library.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(fdskbase->device.dd_Library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the device */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, expunge, struct fdskbase *, fdskbase, 3, fdsk)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(fdskbase->device.dd_Library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	fdskbase->device.dd_Library.lib_Flags |= LIBF_DELEXP;
	return 0;
    }

    /* Free resources */
    CloseLibrary((struct Library *)DOSBase);

    /* Get rid of the device. Remove it from the list. */
    Remove(&fdskbase->device.dd_Library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret = fdskbase->seglist;

    /* Free the memory. */
    FreeMem((char *)fdskbase-fdskbase->device.dd_Library.lib_NegSize, 
	    fdskbase->device.dd_Library.lib_NegSize+fdskbase->device.dd_Library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0I(int, null, struct fdskbase *, fdskbase, 4, fdsk)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(void, beginio, 
 AROS_LHA(struct IOExtTD *, iotd, A1), 
	   struct fdskbase *, fdskbase, 5, fdsk)
{
    AROS_LIBFUNC_INIT

    switch(iotd->iotd_Req.io_Command)
    {
	case CMD_UPDATE:
	case CMD_CLEAR:
	case TD_MOTOR:
	    /* Ignore but don't fail */
	    iotd->iotd_Req.io_Error = 0;
	    break;
	    
	case CMD_READ:
	case CMD_WRITE:
	case TD_FORMAT:
	case TD_CHANGENUM:
	case TD_CHANGESTATE:
	case TD_GETGEOMETRY:
	    /* Forward to unit thread */
	    PutMsg(&((struct unit *)iotd->iotd_Req.io_Unit)->port, 
		   &iotd->iotd_Req.io_Message);
	    /* Not done quick */
	    iotd->iotd_Req.io_Flags &= ~IOF_QUICK;
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
	   struct fdskbase *, fdskbase, 6, fdsk)
{
    AROS_LIBFUNC_INIT
    return IOERR_NOCMD;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

#define fdskbase unit->fdskbase

/****************************************************************************************/

static LONG error(LONG error)
{
    switch(error)
    {
	case ERROR_SEEK_ERROR:
	    return TDERR_SeekError;
	    
	case ERROR_DISK_WRITE_PROTECTED:
	case ERROR_WRITE_PROTECTED:
	    return TDERR_WriteProt;
	    
	case ERROR_NO_DISK:
	    return TDERR_DiskChanged;
	    
	default:
	    return TDERR_NotSpecified;
    }
}

/****************************************************************************************/

static LONG read(struct unit *unit, struct IOExtTD *iotd)
{
    STRPTR 	buf;
    LONG 	size, subsize;
    
    D(bug("fdsk_device/read: offset = %d  size = %d\n", iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Length));
    
#if 0
    if(iotd->iotd_SecLabel)
    {
        D(bug("fdsk_device/read: iotd->iotd_SecLabel is != NULL -> returning IOERR_NOCMD\n"));
	return IOERR_NOCMD;
    }
#endif
    
    if(Seek(unit->file, iotd->iotd_Req.io_Offset, OFFSET_BEGINNING) == -1)
    {
        D(bug("fdsk_device/read: Seek to offset %d failed. Returning TDERR_SeekError\n", iotd->iotd_Req.io_Offset));
	return TDERR_SeekError;
    }
    
    buf  = iotd->iotd_Req.io_Data;
    size = iotd->iotd_Req.io_Length;
    iotd->iotd_Req.io_Actual = size;
    
    while(size)
    {
	subsize = Read(unit->file, buf, size);
	if(!subsize)
	{
	     iotd->iotd_Req.io_Actual -= size;
             D(bug("fdsk_device/read: Read() returned 0. Returning IOERR_BADLENGTH\n"));	     
	     return IOERR_BADLENGTH;
	}
	if(subsize == -1)
	{
	    iotd->iotd_Req.io_Actual -= size;
            D(bug("fdsk_device/read: Read() returned -1. Returning error number %d\n", error(IoErr())));
	    return error(IoErr());
	}
	buf  += subsize;
	size -= subsize;
    }

#if DEBUG
    buf = iotd->iotd_Req.io_Data;
    D(bug("fdsk_device/read: returning 0. First 4 buffer bytes = [%c%c%c%c]\n", buf[0], buf[1], buf[2], buf[3]));
#endif

    return 0;
}

/****************************************************************************************/

static LONG write(struct unit *unit, struct IOExtTD *iotd)
{
    STRPTR 	buf;
    LONG 	size, subsize;
#if 0
    if(iotd->iotd_SecLabel)
	return IOERR_NOCMD;
#endif
    if(Seek(unit->file, iotd->iotd_Req.io_Offset, OFFSET_BEGINNING) == -1)
	return TDERR_SeekError;
	
    buf  = iotd->iotd_Req.io_Data;
    size = iotd->iotd_Req.io_Length;
    iotd->iotd_Req.io_Actual = size;
    
    while(size)
    {
	subsize = Write(unit->file, buf, size);
	if(subsize == -1)
	{
	    iotd->iotd_Req.io_Actual -= size;
	    return error(IoErr());
	}
	buf  += subsize;
	size -= subsize;
    }
    
    return 0;
}

/****************************************************************************************/
void getgeometry(struct unit *unit, struct DriveGeometry *dg) {
struct FileInfoBlock fib;

    Examine(unit->file, &fib);
    dg->dg_SectorSize = 512;
    dg->dg_Heads = 16;
    dg->dg_TrackSectors = 63;
    dg->dg_TotalSectors = fib.fib_Size / dg->dg_SectorSize;
    /* in case of links or block devices with emul_handler we get the wrong size */
    if (dg->dg_TotalSectors == 0)
	dg->dg_TotalSectors = dg->dg_Heads*dg->dg_TrackSectors*5004;
    dg->dg_Cylinders = dg->dg_TotalSectors / (dg->dg_Heads * dg->dg_TrackSectors);
    dg->dg_CylSectors = dg->dg_Heads * dg->dg_TrackSectors;
    dg->dg_BufMemType = MEMF_PUBLIC;
    dg->dg_DeviceType = DG_DIRECT_ACCESS;
    dg->dg_Flags = 0;
}
/****************************************************************************************/

AROS_UFH2(void, putchr, 
    AROS_UFHA(UBYTE, chr, D0), 
    AROS_UFHA(STRPTR *, p, A3)
)
{
    AROS_LIBFUNC_INIT
    *(*p)++ = chr;
    AROS_LIBFUNC_EXIT
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
    UBYTE 		buf[10 + sizeof(LONG) * 8 * 301 / 1000 + 1];
    STRPTR 		ptr = buf;
    struct Process 	*me;
    LONG 		err = 0L;
    struct IOExtTD 	*iotd;
    struct unit 	*unit;

#ifdef _AMIGA
    SysBase = *((struct ExecBase **)4);
#endif

    D(bug("fdsk_device/unitentry: just started\n"));
    
    me = (struct Process *)FindTask(NULL);

    WaitPort(&me->pr_MsgPort);
    unit = (struct unit *)GetMsg(&me->pr_MsgPort);
    unit->port.mp_SigBit = AllocSignal(-1);
    unit->port.mp_Flags = PA_SIGNAL;

    (void)RawDoFmt("FDSK:Unit%ld", &unit->unitnum, (VOID_FUNC)putchr, &ptr);

    D(bug("fdsk_device/unitentry: Trying to open \"%s\" ...\n", buf));

    unit->file = Open(buf, MODE_OLDFILE);
    if(!unit->file)
    {
/*
#warning FIXME: Next line will produce a segfault -- uninitialized variable iotd
	iotd->iotd_Req.io_Error = error(IoErr());
*/
        D(bug("fdsk_device/unitentry: open failed ioerr = %d:-( Replying startup msg.\n", IoErr()));

	Forbid();
	ReplyMsg(&unit->msg);
	return 0;
    }

    D(bug("fdsk_device/unitentry: open okay :-) Replying startup msg.\n"));

    ReplyMsg(&unit->msg);

    D(bug("fdsk_device/unitentry: Now entering main loop\n"));

    for(;;)
    {
	while((iotd = (struct IOExtTD *)GetMsg(&unit->port)) != NULL)
	{
	    if(&iotd->iotd_Req.io_Message == &unit->msg)
	    {
    		D(bug("fdsk_device/unitentry: Recevied EXIT message.\n"));

		Close(unit->file);
		Forbid();
		ReplyMsg(&unit->msg);
		return 0;
	    }

 	    switch(iotd->iotd_Req.io_Command)
 	    {
 		case CMD_READ:
     		    D(bug("fdsk_device/unitentry: received CMD_READ.\n"));
		    err = read(unit, iotd);
 		    break;
		    
 		case CMD_WRITE:
 		case TD_FORMAT:
    		    D(bug("fdsk_device/unitentry: received %s\n", (iotd->iotd_Req.io_Command == CMD_WRITE) ? "CMD_WRITE" : "TD_FORMAT"));
 		    err = write(unit, iotd);
 		    break;
		case TD_CHANGENUM:
		case TD_CHANGESTATE:
		    err = 0;
		    iotd->iotd_Req.io_Actual = 0;
		    break;
		case TD_GETGEOMETRY:
		    getgeometry(unit, (struct DriveGeometry *)iotd->iotd_Req.io_Data);
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

const char end = 0;

/****************************************************************************************/
