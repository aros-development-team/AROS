/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

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
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#include "fdsk_device_gcc.h"
#endif

#define DEBUG 0
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#define NEWSTYLE_DEVICE 1

#if NEWSTYLE_DEVICE
static const UWORD SupportedCommands[] =
{
    CMD_UPDATE,
    CMD_CLEAR,
    TD_MOTOR,
    CMD_READ,
    CMD_WRITE,
    TD_FORMAT,
    TD_CHANGENUM,
    TD_CHANGESTATE,
    TD_ADDCHANGEINT,
    TD_REMCHANGEINT,
    TD_GETGEOMETRY,
    TD_EJECT,
    TD_PROTSTATUS,
    ETD_READ,
    ETD_WRITE,
    ETD_UPDATE,
    ETD_CLEAR,
    ETD_MOTOR,
    ETD_FORMAT,
    TD_GETDRIVETYPE,
    NSCMD_DEVICEQUERY,
    0
};
#endif

/****************************************************************************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR fdskbase)
{
    D(bug("[FDSK  ] in libinit func\n"));

    InitSemaphore(&fdskbase->sigsem);
    NEWLIST((struct List *)&fdskbase->units);
    fdskbase->port.mp_Node.ln_Type = NT_MSGPORT;
    fdskbase->port.mp_Flags = PA_SIGNAL;
    fdskbase->port.mp_SigBit = SIGB_SINGLE;
    NEWLIST((struct List *)&fdskbase->port.mp_MsgList);

   D(bug("[FDSK  ] in libinit func. Returning %x (success) :-)\n", fdskbase));
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
    LIBBASETYPEPTR fdskbase,
    struct IOExtTD *iotd,
    ULONG unitnum,
    ULONG flags
)
{
    static const struct TagItem tags[] =
    {
        { NP_Name   , (IPTR)"File Disk Unit Process"},
        { NP_Input  , 0                     },
        { NP_Output , 0                     },
        { NP_Error  , 0                     },
        { NP_CurrentDir , 0                 },
        { NP_Priority   , 0                 },
        { NP_HomeDir    , 0                 },
        { NP_CopyVars   , 0                 },
        { NP_Entry  , (IPTR)unitentry       },
        { TAG_END   , 0                     }
    };
    struct unit *unit;

    D(bug("[FDSK%02ld] in libopen func.\n", unitnum));

    D(bug("[FDSK%02ld] in libopen func. Looking if unit%ld is already open\n", unitnum, unitnum));

    ObtainSemaphore(&fdskbase->sigsem);

    for(unit = (struct unit *)fdskbase->units.mlh_Head;
    unit->msg.mn_Node.ln_Succ != NULL;
    unit = (struct unit *)unit->msg.mn_Node.ln_Succ)
    if(unit->unitnum == unitnum)
    {
        unit->usecount++;
        ReleaseSemaphore(&fdskbase->sigsem);

        iotd->iotd_Req.io_Unit     = (struct Unit *)unit;
        iotd->iotd_Req.io_Error    = 0;
        iotd->iotd_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;

        D(bug("[FDSK%02ld] in libopen func. Yep. Unit is already open\n", unitnum));

        return TRUE;
    }

    D(bug("[FDSK%02ld] in libopen func. No, it is not. So creating new unit ...\n", unitnum));

    unit = (struct unit *)AllocMem(sizeof(struct unit),
        MEMF_PUBLIC | MEMF_CLEAR);
    if(unit != NULL)
    {
        D(bug("[FDSK%02ld] in libopen func. Allocation of unit memory okay. Setting up unit and calling CreateNewProc ...\n", unitnum));

    unit->usecount          = 1;
    unit->fdskbase          = fdskbase;
    unit->unitnum           = unitnum;
    unit->msg.mn_ReplyPort      = &fdskbase->port;
    unit->msg.mn_Length         = sizeof(struct unit);
    unit->port.mp_Node.ln_Type  = NT_MSGPORT;
    unit->port.mp_Flags         = PA_IGNORE;
    unit->port.mp_SigTask       = CreateNewProc((struct TagItem *)tags);
    NEWLIST((struct List *)&unit->changeints);

    D(bug("[FDSK%02ld] in libopen func. CreateNewProc called. Proc = %x\n", unitnum, unit->port.mp_SigTask));

    if(unit->port.mp_SigTask != NULL)
    {
        NEWLIST((struct List *)&unit->port.mp_MsgList);

        /* setup replyport to point to active task */
        fdskbase->port.mp_SigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);

        D(bug("[FDSK%02ld] in libopen func. Sending startup msg\n", unitnum));
        PutMsg(&((struct Process *)unit->port.mp_SigTask)->pr_MsgPort, &unit->msg);

        D(bug("[FDSK%02ld] in libopen func. Waiting for replymsg\n", unitnum));
        WaitPort(&fdskbase->port);
        (void)GetMsg(&fdskbase->port);
        D(bug("[FDSK%02ld] in libopen func. Received replymsg\n", unitnum));

        if(unit->file)
        {
            AddTail((struct List *)&fdskbase->units, &unit->msg.mn_Node);
            iotd->iotd_Req.io_Unit = (struct Unit *)unit;
            /* Set returncode */
            iotd->iotd_Req.io_Error = 0;
            ReleaseSemaphore(&fdskbase->sigsem);
            return TRUE;
        }else
            iotd->iotd_Req.io_Error = TDERR_NotSpecified;
    }else
        iotd->iotd_Req.io_Error = TDERR_NoMem;
    FreeMem(unit, sizeof(struct unit));
    }else
        iotd->iotd_Req.io_Error = TDERR_NoMem;

    ReleaseSemaphore(&fdskbase->sigsem);

    return FALSE;
}

/****************************************************************************************/

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR fdskbase,
    struct IOExtTD *iotd
)
{
    struct unit *unit;

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

    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

/****************************************************************************************/

AROS_LH1(void, beginio, 
 AROS_LHA(struct IOExtTD *, iotd, A1),
          struct fdskbase *, fdskbase, 5, Fdsk)
{
    AROS_LIBFUNC_INIT

    switch(iotd->iotd_Req.io_Command)
    {
#if NEWSTYLE_DEVICE
        case NSCMD_DEVICEQUERY:
            D(bug("[FDSK  ] NSCMD_DEVICEQUERY\n"));
            if(iotd->iotd_Req.io_Length < ((LONG)OFFSET(NSDeviceQueryResult, SupportedCommands)) + sizeof(UWORD *))
            {
                iotd->iotd_Req.io_Error = IOERR_BADLENGTH;
            }
            else
            {
                struct NSDeviceQueryResult *d;

                d = (struct NSDeviceQueryResult *)iotd->iotd_Req.io_Data;

                d->DevQueryFormat       = 0;
                d->SizeAvailable        = sizeof(struct NSDeviceQueryResult);
                d->DeviceType           = NSDEVTYPE_TRACKDISK;
                d->DeviceSubType        = 0;
                d->SupportedCommands    = (UWORD *)SupportedCommands;

                iotd->iotd_Req.io_Actual = sizeof(struct NSDeviceQueryResult);
                iotd->iotd_Req.io_Error  = 0;
            }
            break;
        case TD_GETDRIVETYPE:
            iotd->iotd_Req.io_Actual = DRIVE_NEWSTYLE;
            break;
#endif
        case ETD_UPDATE:
        case ETD_CLEAR:
        case ETD_MOTOR:
        case CMD_UPDATE:
        case CMD_CLEAR:
        case TD_MOTOR:
            /* Ignore but don't fail */
            iotd->iotd_Req.io_Error = 0;
            break;

        case ETD_READ:
        case ETD_WRITE:
        case ETD_FORMAT:
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
          struct fdskbase *, fdskbase, 6, Fdsk)
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

    D(bug("[FDSK%02ld] read32: offset = %08x  size = %08x\n", unit->unitnum, iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Length));

#if 0
    if(iotd->iotd_SecLabel)
    {
        D(bug("[FDSK%02ld] read32: iotd->iotd_SecLabel is != NULL -> returning IOERR_NOCMD\n", unit->unitnum));
        return IOERR_NOCMD;
    }
#endif

    if(Seek(unit->file, iotd->iotd_Req.io_Offset, OFFSET_BEGINNING) == -1)
    {
        D(bug("[FDSK%02ld] read32: Seek to offset %d failed. Returning TDERR_SeekError\n", unit->unitnum, iotd->iotd_Req.io_Offset));
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
            D(bug("[FDSK%02ld] read32: Read() returned 0. Returning IOERR_BADLENGTH\n", unit->unitnum));
            return IOERR_BADLENGTH;
        }
        if(subsize == -1)
        {
            iotd->iotd_Req.io_Actual -= size;
            D(bug("[FDSK%02ld] read32: Read() returned -1. Returning error number %d\n", unit->unitnum, error(IoErr())));
            return error(IoErr());
        }
        buf  += subsize;
        size -= subsize;
    }

#if DEBUG
    buf = iotd->iotd_Req.io_Data;
    D(bug("[FDSK%02ld] read32: returning 0. First 4 buffer bytes = [%c%c%c%c]\n", unit->unitnum, buf[0], buf[1], buf[2], buf[3]));
#endif

    return 0;
}

/****************************************************************************************/

static LONG write(struct unit *unit, struct IOExtTD *iotd)
{
    STRPTR  buf;
    LONG    size, subsize;

    D(bug("[FDSK%02ld] write32: offset = %08x  size = %08x\n", unit->unitnum, iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Length));

    if(!unit->writable)
        return TDERR_WriteProt;
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

void getgeometry(struct unit *unit, struct DriveGeometry *dg) {
struct FileInfoBlock fib;

    ExamineFH(unit->file, &fib);
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
    dg->dg_Flags = DGF_REMOVABLE;
}

/**************************************************************************/

static LONG eject(struct unit *unit, struct IOExtTD *iotd)
{
    BOOL eject = iotd->iotd_Req.io_Length;
    struct FileInfoBlock fib;
    if ((eject) && (unit->file))
    {
        Close(unit->file);
        unit->file = (BPTR)NULL;
        goto quiteject;
    } else if ((eject) && (unit->file == NULL)) {
        return ERROR_NO_DISK;
    }

    if ((!eject) && (unit->file == NULL))
    {
        unit->file = Open(unit->filename, MODE_OLDFILE);
        if (unit->file == (BPTR) NULL)
            return ERROR_OBJECT_NOT_FOUND;
        ExamineFH(unit->file, &fib);
        unit->writable = !(fib.fib_Protection & FIBF_WRITE);
    } else {
        return 0;
    }
quiteject:
    unit->changecount++;

    ForeachNode(&unit->changeints, iotd)
    {
        Cause((struct Interrupt *)((struct IOExtTD *)iotd->iotd_Req.io_Data));
    }
    return 0;
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

    UBYTE       buf[10 + sizeof(LONG) * 8 * 301 / 1000 + 1];
    STRPTR      ptr = buf;
    struct Process  *me;
    LONG            err = 0L;
    struct IOExtTD  *iotd;
    struct unit     *unit;
    APTR            win;
    struct FileInfoBlock fib;

    me = (struct Process *)FindTask(NULL);

    WaitPort(&me->pr_MsgPort);
    unit = (struct unit *)GetMsg(&me->pr_MsgPort);
    unit->port.mp_SigBit = AllocSignal(-1);
    unit->port.mp_Flags = PA_SIGNAL;

    /* disable DOS error requesters. save the old pointer so we can put it
     * back later */
    win = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR) -1;

    (void)RawDoFmt("FDSK:Unit%ld", &unit->unitnum, (VOID_FUNC)putchr, &ptr);

    D(bug("[FDSK%02ld] Trying to open \"%s\" ...\n", unit->unitnum, buf));

    unit->filename = buf;
    unit->file = Open(buf, MODE_OLDFILE);
    if(!unit->file)
    {
/*
#warning FIXME: Next line will produce a segfault -- uninitialized variable iotd
        iotd->iotd_Req.io_Error = error(IoErr());
*/
        D(bug("[FDSK%02ld] open failed ioerr = %d :-( Replying startup msg!\n", unit->unitnum, IoErr()));

        ReplyMsg(&unit->msg);
        return DOSFALSE;
    }

    ExamineFH(unit->file, &fib);
    unit->writable = !(fib.fib_Protection & FIBF_WRITE);

    /* enable requesters */
    me->pr_WindowPtr = win;

    D(bug("[FDSK%02ld] open okay :-) Replying startup msg.\n", unit->unitnum));

    ReplyMsg(&unit->msg);

    D(bug("[FDSK%02ld] now entering main loop.\n", unit->unitnum));

    for(;;)
    {
        while((iotd = (struct IOExtTD *)GetMsg(&unit->port)) != NULL)
        {
            if(&iotd->iotd_Req.io_Message == &unit->msg)
            {
                D(bug("[FDSK%02ld] received EXIT message.\n", unit->unitnum));

                Close(unit->file);
                Forbid();
                ReplyMsg(&unit->msg);
                return 0;
            }
            switch(iotd->iotd_Req.io_Command)
            {
                case ETD_READ:
                case CMD_READ:
                    D(bug("[FDSK%02ld] received CMD_READ.\n", unit->unitnum));
                    err = read(unit, iotd);
                    break;
                case ETD_WRITE:
                case CMD_WRITE:
                case TD_FORMAT:
                case ETD_FORMAT:
                    D(bug("[FDSK%02ld] received %s\n", unit->unitnum, (
                        (iotd->iotd_Req.io_Command == ETD_WRITE) ||
                        (iotd->iotd_Req.io_Command == CMD_WRITE)) ? "CMD_WRITE" : "TD_FORMAT"));
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
                    getgeometry(unit, (struct DriveGeometry *)iotd->iotd_Req.io_Data);
                    err = 0;
                    break;
                case TD_EJECT:
                    err = eject(unit, iotd);
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
