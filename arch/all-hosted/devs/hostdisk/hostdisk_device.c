/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

/****************************************************************************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR hdskBase)
{
    D(bug("hostdisk: in libinit func\n"));

    InitSemaphore(&hdskBase->sigsem);
    NEWLIST((struct List *)&hdskBase->units);
    hdskBase->port.mp_Node.ln_Type = NT_MSGPORT;
    hdskBase->port.mp_Flags = PA_SIGNAL;
    hdskBase->port.mp_SigBit = SIGB_SINGLE;
    NEWLIST((struct List *)&hdskBase->port.mp_MsgList);

   D(bug("hostdisk: in libinit func. Returning %x (success) :-)\n", hdskBase));
   return TRUE;
}

/****************************************************************************************/

AROS_UFP3(LONG, unitentry,
 AROS_UFPA(STRPTR, argstr, A0),
 AROS_UFPA(ULONG, arglen, D0),
 AROS_UFPA(struct ExecBase *, SysBase, A6));
 
/****************************************************************************************/

static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR hdskBase,
    struct IOExtTD *iotd,
    ULONG unitnum,
    ULONG flags
)
{
    static const struct TagItem tags[] = 
    {
    	{ NP_Name	, (IPTR)"Host Disk Unit Process"}, 
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

    D(bug("hostdisk: in libopen func.\n"));

    D(bug("hostdisk: in libopen func. Looking if unit is already open\n"));

    ObtainSemaphore(&hdskBase->sigsem);

    for(unit = (struct unit *)hdskBase->units.mlh_Head;
	unit->msg.mn_Node.ln_Succ != NULL;
	unit = (struct unit *)unit->msg.mn_Node.ln_Succ)
	if(unit->unitnum == unitnum)
	{
	    unit->usecount++;
	    ReleaseSemaphore(&hdskBase->sigsem);
	    
	    iotd->iotd_Req.io_Unit 		      = (struct Unit *)unit;
	    iotd->iotd_Req.io_Error 		      = 0;
	    iotd->iotd_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
   	   
	    D(bug("hostdisk: in libopen func. Yep. Unit is already open\n"));
	    
	    return TRUE;
	}

    D(bug("hostdisk: in libopen func. No, it is not. So creating new unit ...\n"));

    unit = (struct unit *)AllocMem(sizeof(struct unit),
        MEMF_PUBLIC | MEMF_CLEAR);
    if(unit != NULL)
    {
        D(bug("hostdisk: in libopen func. Allocation of unit memory okay. Setting up unit and calling CreateNewProc ...\n"));

	unit->usecount 			= 1;
	unit->hdskBase 			= hdskBase;
	unit->unitnum 			= unitnum;
	unit->msg.mn_ReplyPort 		= &hdskBase->port;
	unit->msg.mn_Length 		= sizeof(struct unit);
	unit->port.mp_Node.ln_Type 	= NT_MSGPORT;
	unit->port.mp_Flags 	 	= PA_IGNORE;
	unit->port.mp_SigTask 		= CreateNewProc((struct TagItem *)tags);
	NEWLIST((struct List *)&unit->changeints);

        D(bug("hostdisk: in libopen func. CreateNewProc called. Proc = %x\n", unit->port.mp_SigTask));
	
	if(unit->port.mp_SigTask != NULL)
	{
	    NEWLIST((struct List *)&unit->port.mp_MsgList);

	    /* setup replyport to point to active task */
	    hdskBase->port.mp_SigTask = FindTask(NULL);
    	    SetSignal(0, SIGF_SINGLE);
	    
    	    D(bug("hostdisk: in libopen func. Sending startup msg\n"));
	    PutMsg(&((struct Process *)unit->port.mp_SigTask)->pr_MsgPort, &unit->msg);

    	    D(bug("hostdisk: in libopen func. Waiting for replymsg\n"));
	    WaitPort(&hdskBase->port);
	    (void)GetMsg(&hdskBase->port);
    	    D(bug("hostdisk: in libopen func. Received replymsg\n"));
	    
	    if (unit->file != INVALID_HANDLE_VALUE)
	    {
		AddTail((struct List *)&hdskBase->units, &unit->msg.mn_Node);
		iotd->iotd_Req.io_Unit = (struct Unit *)unit;
		/* Set returncode */
		iotd->iotd_Req.io_Error = 0;
		ReleaseSemaphore(&hdskBase->sigsem);
		return TRUE;
	    }else
		iotd->iotd_Req.io_Error = TDERR_NotSpecified;
	}else
	    iotd->iotd_Req.io_Error = TDERR_NoMem;
	FreeMem(unit, sizeof(struct unit));
    }else
	iotd->iotd_Req.io_Error = TDERR_NoMem;

    ReleaseSemaphore(&hdskBase->sigsem);

    return FALSE;
}

/****************************************************************************************/

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR hdskBase,
    struct IOExtTD *iotd
)
{
    struct unit *unit;

    ObtainSemaphore(&hdskBase->sigsem);
    unit = (struct unit *)iotd->iotd_Req.io_Unit;
    if(!--unit->usecount)
    {
	Remove(&unit->msg.mn_Node);
	hdskBase->port.mp_SigTask = FindTask(NULL);
	SetSignal(0, SIGF_SINGLE);
	PutMsg(&unit->port, &unit->msg);
	WaitPort(&hdskBase->port);
	(void)GetMsg(&hdskBase->port);
	FreeMem(unit, sizeof(struct unit));
    }
    ReleaseSemaphore(&hdskBase->sigsem);

    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

/****************************************************************************************/

AROS_LH1(void, beginio, 
 AROS_LHA(struct IOExtTD *, iotd, A1), 
	   struct HostDiskBase *, hdskBase, 5, Hostdisk)
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
	case TD_ADDCHANGEINT:
	case TD_REMCHANGEINT:
	case TD_GETGEOMETRY:
	case TD_EJECT:
	case TD_PROTSTATUS:
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
	   struct HostDiskBase *, hdskBase, 6, Hostdisk)
{
    AROS_LIBFUNC_INIT
    return IOERR_NOCMD;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

static LONG read(struct unit *unit, struct IOExtTD *iotd)
{
    STRPTR 	buf;
    LONG 	size, subsize;
    ULONG	ioerr;

    D(bug("hostdisk/read: offset = %d  size = %d\n", iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Length));
    
#if 0
    if(iotd->iotd_SecLabel)
    {
        D(bug("hostdisk/read: iotd->iotd_SecLabel is != NULL -> returning IOERR_NOCMD\n"));
	return IOERR_NOCMD;
    }
#endif

    ioerr = Host_Seek(unit, iotd->iotd_Req.io_Offset);
    if (ioerr)
    {
        D(bug("hostdisk/read: Seek to offset %d failed\n", iotd->iotd_Req.io_Offset));
	return ioerr;
    }
    
    buf  = iotd->iotd_Req.io_Data;
    size = iotd->iotd_Req.io_Length;
    iotd->iotd_Req.io_Actual = size;
    
    while(size)
    {
	subsize = Host_Read(unit, buf, size, &ioerr);
	if(!subsize)
	{
	     iotd->iotd_Req.io_Actual -= size;
             D(bug("hostdisk.device/read: Host_Read() returned 0. Returning IOERR_BADLENGTH\n"));	     
	     return IOERR_BADLENGTH;
	}
	if(subsize == -1)
	{
	    iotd->iotd_Req.io_Actual -= size;
            D(bug("hostdisk.device/read: Host_Read() returned -1. Returning error number %d\n", ioerr));
	    return ioerr;
	}
	buf  += subsize;
	size -= subsize;
    }

#if DEBUG
    buf = iotd->iotd_Req.io_Data;
    D(bug("hostdisk/read: returning 0. First 4 buffer bytes = [%c%c%c%c]\n", buf[0], buf[1], buf[2], buf[3]));
#endif

    return 0;
}

/****************************************************************************************/

static LONG write(struct unit *unit, struct IOExtTD *iotd)
{
    STRPTR 	buf;
    LONG 	size, subsize;
    ULONG	ioerr;

    if(!unit->writable)
	return TDERR_WriteProt;
#if 0
    if(iotd->iotd_SecLabel)
	return IOERR_NOCMD;
#endif
    ioerr = Host_Seek(unit, iotd->iotd_Req.io_Offset);
    if (ioerr)
	return ioerr;

    buf  = iotd->iotd_Req.io_Data;
    size = iotd->iotd_Req.io_Length;
    iotd->iotd_Req.io_Actual = size;
    
    while(size)
    {
        ULONG ioerr;

	subsize = Host_Write(unit, buf, size, &ioerr);
	if(subsize == -1)
	{
	    iotd->iotd_Req.io_Actual -= size;
	    return ioerr;
	}
	buf  += subsize;
	size -= subsize;
    }
    
    return 0;
}

/**************************************************************************/

static void addchangeint(struct unit *unit, struct IOExtTD *iotd) {
    Forbid();
    AddTail((struct List *)&unit->changeints, (struct Node *)iotd);
    Permit();
}

/**************************************************************************/

static void remchangeint(struct unit *unit, struct IOExtTD *iotd) {
    Forbid();
    Remove((struct Node *)iotd);
    Permit();
}

/**************************************************************************/

void eject(struct unit *unit, BOOL eject)
{
    struct IOExtTD *iotd;
    ULONG err;

    if (eject)
    {
        Host_Close(unit);
        unit->file = (BPTR)NULL;
    }
    else
    {
        err = Host_Open(unit);
        if (err)
            return;
    }

    unit->changecount++;

    ForeachNode(&unit->changeints, iotd)
    {
        Cause((struct Interrupt *)iotd->iotd_Req.io_Data);
    }
}

/**************************************************************************/

AROS_UFH2(void, putchr, 
    AROS_UFHA(UBYTE, chr, D0), 
    AROS_UFHA(STRPTR *, p, A3)
)
{
    AROS_USERFUNC_INIT
    *(*p)++ = chr;
    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

AROS_UFH3(LONG, unitentry,
 AROS_UFHA(STRPTR, argstr, A0),
 AROS_UFHA(ULONG, arglen, D0),
 AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    
    UBYTE 		buf[10 + sizeof(LONG) * 8 * 301 / 1000 + 1];
    STRPTR 		ptr = buf;
    struct Process 	*me;
    LONG 		err = 0L;
    struct IOExtTD 	*iotd;
    struct unit 	*unit;

    D(bug("hostdisk/unitentry: just started\n"));
    
    me = (struct Process *)FindTask(NULL);

    WaitPort(&me->pr_MsgPort);
    unit = (struct unit *)GetMsg(&me->pr_MsgPort);
    unit->port.mp_SigBit = AllocSignal(-1);
    unit->port.mp_Flags = PA_SIGNAL;

    RawDoFmt(unit->hdskBase->DiskDevice, &unit->unitnum, (VOID_FUNC)putchr, &ptr);

    D(bug("hostdisk/unitentry: Trying to open \"%s\" ...\n", buf));

    unit->filename = buf;
    err = Host_Open(unit);
    if(err)
    {
/*
#warning FIXME: Next line will produce a segfault -- uninitialized variable iotd
	iotd->iotd_Req.io_Error = err;
*/
        D(bug("hostdisk/unitentry: open failed :-( Replying startup msg.\n"));

	ReplyMsg(&unit->msg);
	return 0;
    }

    D(bug("hostdisk/unitentry: open okay :-) Replying startup msg.\n"));

    ReplyMsg(&unit->msg);

    D(bug("hostdisk/unitentry: Now entering main loop\n"));

    for(;;)
    {
	while((iotd = (struct IOExtTD *)GetMsg(&unit->port)) != NULL)
	{
	    if(&iotd->iotd_Req.io_Message == &unit->msg)
	    {
    		D(bug("hostdisk/unitentry: Received EXIT message.\n"));

		Host_Close(unit);
		Forbid();
		ReplyMsg(&unit->msg);
		return 0;
	    }

 	    switch(iotd->iotd_Req.io_Command)
 	    {
 		case CMD_READ:
     		    D(bug("hostdisk/unitentry: received CMD_READ.\n"));
		    err = read(unit, iotd);
 		    break;
		    
 		case CMD_WRITE:
 		case TD_FORMAT:
    		    D(bug("hostdisk/unitentry: received %s\n", (iotd->iotd_Req.io_Command == CMD_WRITE) ? "CMD_WRITE" : "TD_FORMAT"));
 		    err = write(unit, iotd);
 		    break;
		case TD_CHANGENUM:
		    err = 0;
		    iotd->iotd_Req.io_Actual = unit->changecount;
		    break;
		case TD_CHANGESTATE:
		    err = 0;
		    iotd->iotd_Req.io_Actual = unit->file == (BPTR)NULL;
		    break;
		case TD_ADDCHANGEINT:
		    addchangeint(unit, iotd);
		    err = 0;
		    break;
		case TD_REMCHANGEINT:
		    remchangeint(unit, iotd);
		    err = 0;
		    break;
		case TD_GETGEOMETRY:
		    err = Host_GetGeometry(unit, (struct DriveGeometry *)iotd->iotd_Req.io_Data);
		    break;
		case TD_EJECT:
		    eject(unit, iotd->iotd_Req.io_Length);
		    err = 0;
		    break;
		case TD_PROTSTATUS:
		    iotd->iotd_Req.io_Actual = !unit->writable;
		    err = 0;
		    break;
		    
 	    } /* switch(iotd->iotd_Req.io_Command) */
	    
 	    iotd->iotd_Req.io_Error = err;
 	    ReplyMsg(&iotd->iotd_Req.io_Message);
	    
	} /* while((iotd = (struct IOExtTD *)GetMsg(&unit->port)) != NULL) */
	
	WaitPort(&unit->port);
	
    } /* for(;;) */
    AROS_USERFUNC_EXIT
}

/****************************************************************************************/
