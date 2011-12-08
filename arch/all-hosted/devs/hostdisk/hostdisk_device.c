/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************************/

#define DEBUG 0
#define DCMD(x)
#define DOPEN(x)
#define DREAD(x)
#define DWRITE(x)
/* #define DUMP_DATA */

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <exec/errors.h>
#include <exec/rawfmt.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/hostlib.h>

#include LC_LIBDEFS_FILE

/****************************************************************************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR hdskBase)
{
    HostLibBase = OpenResource("hostlib.resource");
    D(bug("hostdisk: HostLibBase: 0x%p\n", HostLibBase));
    if (!HostLibBase)
        return FALSE;

    InitSemaphore(&hdskBase->sigsem);
    NEWLIST(&hdskBase->units);

   D(bug("hostdisk: in libinit func. Returning %x (success) :-)\n", hdskBase));
   return TRUE;
}

static int HostDisk_Cleanup(struct HostDiskBase *hdskBase)
{
    D(bug("hostdisk: Expunge(0x%p)\n", hdskBase));

    if (!HostLibBase)
        return TRUE;

    if (hdskBase->iface)
        HostLib_DropInterface((APTR *)hdskBase->iface);

    if (hdskBase->KernelHandle)
        HostLib_Close(hdskBase->KernelHandle, NULL);

    return TRUE;
}

/****************************************************************************************/

static void unitentry(struct IOExtTD *iotd);

static void freeUnit(struct unit *unit)
{
    if (unit->flags & UNIT_FREENAME)
        FreeVec(unit->n.ln_Name);
    
    FreeMem(unit, sizeof(struct unit));
}

/****************************************************************************************/

extern const char GM_UNIQUENAME(LibName)[];

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR hdskBase, struct IOExtTD *iotd, IPTR unitnum, ULONG flags)
{
    STRPTR unitname;
    struct unit *unit;
    UBYTE unitflags = 0;

    if (unitnum < 1024)
    {
        ULONG len = strlen(hdskBase->DiskDevice) + 5;

        unitname = AllocVec(len, MEMF_ANY);
        if (!unitname)
            return FALSE;

        unitflags = UNIT_FREENAME;
        NewRawDoFmt(hdskBase->DiskDevice, (VOID_FUNC)RAWFMTFUNC_STRING, unitname, unitnum + hdskBase->unitBase);
    }
    else
        unitname = (STRPTR)unitnum;

    D(bug("hostdisk: open unit %s\n", unitname));

    ObtainSemaphore(&hdskBase->sigsem);

    unit = (struct unit *)FindName(&hdskBase->units, unitname);

    if (unit)
    {
        unit->usecount++;
        ReleaseSemaphore(&hdskBase->sigsem);
            
        iotd->iotd_Req.io_Unit                    = (struct Unit *)unit;
        iotd->iotd_Req.io_Error                   = 0;
        iotd->iotd_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;

        DOPEN(bug("hostdisk: in libopen func. Yep. Unit is already open\n"));    
        return TRUE;
    }

    DOPEN(bug("hostdisk: in libopen func. No, it is not. So creating new unit ...\n"));

    unit = (struct unit *)AllocMem(sizeof(struct unit), MEMF_PUBLIC | MEMF_CLEAR);

    if (unit != NULL)
    {
        ULONG p = strlen(GM_UNIQUENAME(LibName));
        char taskName[p + strlen(unitname) + 2];
        struct Task *unitTask;

        DOPEN(bug("hostdisk: in libopen func. Allocation of unit memory okay. Setting up unit and calling CreateNewProc ...\n"));

        CopyMem(GM_UNIQUENAME(LibName), taskName, p);
        taskName[p] = ' ';
        strcpy(&taskName[p + 1], unitname);

        unit->n.ln_Name = unitname;
        unit->usecount  = 1;
        unit->hdskBase  = hdskBase;
        unit->flags     = unitflags;
        NEWLIST((struct List *)&unit->changeints);

        iotd->iotd_Req.io_Unit = (struct Unit *)unit;
        SetSignal(0, SIGF_SINGLE);
        unitTask = NewCreateTask(TASKTAG_PC  , unitentry,
                                 TASKTAG_NAME, taskName,
                                 TASKTAG_ARG1, iotd,
                                 TAG_DONE);

        DOPEN(bug("hostdisk: in libopen func. NewCreateTask() called. Task = 0x%p\n", unitTask));

        if (unitTask)
        {
            DOPEN(bug("hostdisk: in libopen func. Waiting for signal from unit task...\n"));
            Wait(SIGF_SINGLE);

            DOPEN(bug("hostdisk: in libopen func. Unit error %u, flags 0x%02X\n", iotd->iotd_Req.io_Error, unit->flags));
            if (!iotd->iotd_Req.io_Error)
            {
                AddTail((struct List *)&hdskBase->units, &unit->n);
                ReleaseSemaphore(&hdskBase->sigsem);
                return TRUE;
            }
        }
        else
            iotd->iotd_Req.io_Error = TDERR_NoMem;

        freeUnit(unit);
    }
    else
        iotd->iotd_Req.io_Error = TDERR_NoMem;

    ReleaseSemaphore(&hdskBase->sigsem);

    return FALSE;
}

/****************************************************************************************/

static int GM_UNIQUENAME(Close)(LIBBASETYPEPTR hdskBase, struct IOExtTD *iotd)
{
    struct unit *unit = (struct unit *)iotd->iotd_Req.io_Unit;

    D(bug("hostdisk: close unit %s\n", unit->n.ln_Name));

    ObtainSemaphore(&hdskBase->sigsem);

    if (!--unit->usecount)
    {
        Remove(&unit->n);

        /* The task will free its unit structure itself */
        Signal(unit->port->mp_SigTask, SIGBREAKF_CTRL_C);
    }

    ReleaseSemaphore(&hdskBase->sigsem);

    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), -5)
ADD2EXPUNGELIB(HostDisk_Cleanup, -5);
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

/****************************************************************************************/

static const UWORD NSDSupported[] = {
//  CMD_RESET,
    CMD_READ,
    CMD_WRITE,
    CMD_UPDATE,
    CMD_CLEAR,
//  CMD_STOP,
//  CMD_START,
    CMD_FLUSH,
    TD_MOTOR,
    TD_SEEK,
    TD_FORMAT,
//  TD_REMOVE,
    TD_CHANGENUM,
    TD_CHANGESTATE,
    TD_PROTSTATUS,
//  TD_GETNUMTRACKS,
    TD_ADDCHANGEINT,
    TD_REMCHANGEINT,
    TD_GETGEOMETRY,
//  TD_EJECT,
    TD_READ64,
    TD_WRITE64,
    TD_SEEK64,
    TD_FORMAT64,
    TD_GETDRIVETYPE,
    NSCMD_DEVICEQUERY,
    NSCMD_TD_READ64,
    NSCMD_TD_WRITE64,
    NSCMD_TD_SEEK64,
    NSCMD_TD_FORMAT64,
    0
};

AROS_LH1(void, beginio, 
 AROS_LHA(struct IOExtTD *, iotd, A1), 
           struct HostDiskBase *, hdskBase, 5, Hostdisk)
{
    AROS_LIBFUNC_INIT

    struct NSDeviceQueryResult *nsdq;
 
    DCMD(bug("hostdisk: command %u\n", iotd->iotd_Req.io_Command)); 
    switch(iotd->iotd_Req.io_Command)
    {
        case CMD_UPDATE:
        case CMD_CLEAR:
        case CMD_FLUSH:
        case TD_MOTOR:
            /* Ignore but don't fail */
            iotd->iotd_Req.io_Error = 0;
            break;

        case CMD_READ:
        case CMD_WRITE:
        case TD_SEEK:
        case TD_FORMAT:
        case TD_READ64:
        case TD_WRITE64:
        case TD_FORMAT64:
        case TD_SEEK64:
        case NSCMD_TD_READ64:
        case NSCMD_TD_WRITE64:
        case NSCMD_TD_FORMAT64:
        case NSCMD_TD_SEEK64:
        case TD_CHANGENUM:
        case TD_CHANGESTATE:
        case TD_ADDCHANGEINT:
        case TD_REMCHANGEINT:
        case TD_GETGEOMETRY:
        case TD_EJECT:
        case TD_PROTSTATUS:
            /* Forward to unit thread */
            PutMsg(((struct unit *)iotd->iotd_Req.io_Unit)->port, &iotd->iotd_Req.io_Message);
            /* Not done quick */
            iotd->iotd_Req.io_Flags &= ~IOF_QUICK;
            return;

        /*
            New Style Devices query. Introduce self as trackdisk and provide list of
            commands supported
        */
        case NSCMD_DEVICEQUERY:
            nsdq = iotd->iotd_Req.io_Data;

            nsdq->DevQueryFormat    = 0;
            nsdq->SizeAvailable     = sizeof(struct NSDeviceQueryResult);
            nsdq->DeviceType        = NSDEVTYPE_TRACKDISK;
            nsdq->DeviceSubType     = 0;
            nsdq->SupportedCommands = (UWORD *)NSDSupported;

            iotd->iotd_Req.io_Actual = sizeof(struct NSDeviceQueryResult);
            iotd->iotd_Req.io_Error  = 0;
            break;

        /*
            New Style Devices report here the 'NSTY' - only if such value is
            returned here, the NSCMD_DEVICEQUERY might be called. Otherwice it should
            report error.
        */
        case TD_GETDRIVETYPE:
            iotd->iotd_Req.io_Actual = DRIVE_NEWSTYLE;
            iotd->iotd_Req.io_Error  = 0;
            break;
            
        default:
            /* Not supported */
            DCMD(bug("hostdisk: command not supported\n"));
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
    STRPTR      buf;
    LONG        size, subsize;
    ULONG       ioerr;
    
    buf  = iotd->iotd_Req.io_Data;
    size = iotd->iotd_Req.io_Length;

    iotd->iotd_Req.io_Actual = 0;
    while (size)
    {
        subsize = Host_Read(unit, buf, size, &ioerr);
        if (!subsize)
        {
             DREAD(bug("hostdisk.device/read: Host_Read() returned 0. Returning IOERR_BADLENGTH\n"));        
             return IOERR_BADLENGTH;
        }
        if (subsize == -1)
        {
            DREAD(bug("hostdisk.device/read: Host_Read() returned -1. Returning error number %d\n", ioerr));
            return ioerr;
        }
        
        iotd->iotd_Req.io_Actual += subsize;
        buf  += subsize;
        size -= subsize;
    }

#ifdef DUMP_DATA
    buf = iotd->iotd_Req.io_Data;
    bug("hostdisk/read: returning 0. First 4 buffer bytes = [%c%c%c%c]\n", buf[0], buf[1], buf[2], buf[3]);
#endif

    return 0;
}

/****************************************************************************************/

static LONG write(struct unit *unit, struct IOExtTD *iotd)
{
    STRPTR      buf;
    LONG        size, subsize;
    ULONG       ioerr;

    if (unit->flags & UNIT_READONLY)
        return TDERR_WriteProt;

    buf  = iotd->iotd_Req.io_Data;
    size = iotd->iotd_Req.io_Length;

    iotd->iotd_Req.io_Actual = 0;
    while(size)
    {
        subsize = Host_Write(unit, buf, size, &ioerr);
        if(subsize == -1)
        {
            return ioerr;
        }
        iotd->iotd_Req.io_Actual += subsize;
        buf  += subsize;
        size -= subsize;
    }
    
    return 0;
}

/**************************************************************************/

static void addchangeint(struct unit *unit, struct IOExtTD *iotd)
{
    Forbid();
    AddTail((struct List *)&unit->changeints, (struct Node *)iotd);
    Permit();
}

/**************************************************************************/

static void remchangeint(struct unit *unit, struct IOExtTD *iotd)
{
    Forbid();
    Remove((struct Node *)iotd);
    Permit();
}

/**************************************************************************/

static ULONG getgeometry(struct unit *Unit, struct DriveGeometry *dg)
{
    /*
     * First set some common defaults.
     * We can work with image file, which is LBA-oriented by nature.
     * LBA addressing can be represented by flattened geometry, where Heads == TrackSectors == 1,
     * i. e. one cylinder == one block (sector).
     * Sector size is assumed to be 512 bytes (a common size for hard disk drives)
     * Host-specific code can override this if possible.
     *
     * This gives us a limitation: we can's handle disks longer than 2TB.
     * It's general AmigaOS limitation, currently inherited by all Amiga family
     * of operating systems, this is determined by maximum block number that can
     * fit into ULONG. In order to overcome this we need 64-bit block number.
     * Perhaps we should completely go LBA in such a case.
     */
    dg->dg_SectorSize   = 512;
    dg->dg_Heads        = 1;
    dg->dg_TrackSectors = 1;
    dg->dg_CylSectors   = 1;    /* Heads * TrackSectors */
    dg->dg_BufMemType   = MEMF_PUBLIC;
    dg->dg_DeviceType   = DG_DIRECT_ACCESS;
    dg->dg_Flags        = 0;

    /* Call host-specific processing */
    return Host_GetGeometry(Unit, dg);
}

/**************************************************************************/

void eject(struct unit *unit, BOOL eject)
{
    struct IOExtTD *iotd;
    ULONG err;

    if (eject)
    {
        Host_Close(unit);
        unit->file = INVALID_HANDLE_VALUE;
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

/****************************************************************************************/

static void unitentry(struct IOExtTD *iotd)
{
    LONG err = 0;
    struct Task *me = FindTask(NULL);
    struct Task *parent = me->tc_UnionETask.tc_ETask->et_Parent;
    struct unit *unit = (struct unit *)iotd->iotd_Req.io_Unit;

    D(bug("%s: just started\n", me->tc_Node.ln_Name));

    unit->port = CreateMsgPort();
    if (!unit->port)
    {
        Signal(parent, SIGF_SINGLE);
        return;
    }

    D(bug("%s: Trying to open \"%s\" ...\n", me->tc_Node.ln_Name, unit->n.ln_Name));

    err = Host_Open(unit);
    if (err)
    {
        D(bug("%s: open failed :-(\n", me->tc_Node.ln_Name));

        iotd->iotd_Req.io_Error = err;

        Signal(parent, SIGF_SINGLE);
        return;
    }

    D(bug("%s: open okay :-)\n", me->tc_Node.ln_Name));

    iotd->iotd_Req.io_Error = 0;
    Signal(parent, SIGF_SINGLE);

    D(bug("%s: Now entering main loop\n", me->tc_Node.ln_Name));

    for(;;)
    {
        ULONG portsig = 1 << unit->port->mp_SigBit;
        ULONG sigs = Wait(portsig | SIGBREAKF_CTRL_C);

        if (sigs & portsig)
        {
            while((iotd = (struct IOExtTD *)GetMsg(unit->port)) != NULL)
            {
                switch(iotd->iotd_Req.io_Command)
                {
                /*
                 * In fact these two commands make a little sense, but they exist,
                 * so we honestly process them.
                 */
                case TD_SEEK:
                    DCMD(bug("%s: received CMD_SEEK.\n", me->tc_Node.ln_Name));
                    err = Host_Seek(unit, iotd->iotd_Req.io_Offset);
                    break;

                case TD_SEEK64:
                case NSCMD_TD_SEEK64:
                    DCMD(bug("%s: received CMD_SEEK64.\n", me->tc_Node.ln_Name));
                    err = Host_Seek64(unit, iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Actual);
                    break;
 
                case CMD_READ:
                    DCMD(bug("%s: received CMD_READ.\n", me->tc_Node.ln_Name));
                    DREAD(bug("hostdisk/CMD_READ: offset = %u (0x%08X)  size = %d\n", iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Length));
                    
                    err = Host_Seek(unit, iotd->iotd_Req.io_Offset);
                    if (!err)
                        err = read(unit, iotd);
                    DREAD(else bug("CMD_READ: Seek failed\n");)
                    break;

                case TD_READ64:
                case NSCMD_TD_READ64:
                    DREAD(bug("hostdisk/TD_READ64: offset = 0x%08X%08X  size = %d\n",
                              iotd->iotd_Req.io_Actual, iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Length));

                    err = Host_Seek64(unit, iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Actual);
                    if (!err)
                        err = read(unit, iotd);
                    DREAD(else bug("CMD_READ64: Seek failed\n");)
                    break;

                case CMD_WRITE:
                case TD_FORMAT:
                    DCMD(bug("%s: received %s\n", me->tc_Node.ln_Name, (iotd->iotd_Req.io_Command == CMD_WRITE) ? "CMD_WRITE" : "TD_FORMAT"));
                    DWRITE(bug("hostdisk/CMD_WRITE: offset = %u (0x%08X)  size = %d\n", iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Length));

                    err = Host_Seek(unit, iotd->iotd_Req.io_Offset);
                    if (!err)
                        err = write(unit, iotd);
                    break;

                case TD_WRITE64:
                case TD_FORMAT64:
                case NSCMD_TD_WRITE64:
                case NSCMD_TD_FORMAT64:
                    DCMD(bug("%s: received TD_WRITE64\n", me->tc_Node.ln_Name));
                    DWRITE(bug("hostdisk/TD_WRITE64: offset = 0x%08X%08X  size = %d\n",
                               iotd->iotd_Req.io_Actual, iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Length));

                    err = Host_Seek64(unit, iotd->iotd_Req.io_Offset, iotd->iotd_Req.io_Actual);
                    if (!err)
                        err = write(unit, iotd);
                    break;

                case TD_CHANGENUM:
                    err = 0;
                    iotd->iotd_Req.io_Actual = unit->changecount;
                    break;

                case TD_CHANGESTATE:
                    err = 0;
                    iotd->iotd_Req.io_Actual = (unit->file == INVALID_HANDLE_VALUE);
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
                    DCMD(bug("%s: received TD_GETGEOMETRY\n", me->tc_Node.ln_Name));

                    err = getgeometry(unit, (struct DriveGeometry *)iotd->iotd_Req.io_Data);
                    break;

                case TD_EJECT:
                    eject(unit, iotd->iotd_Req.io_Length);
                    err = 0;
                    break;

                case TD_PROTSTATUS:
                    iotd->iotd_Req.io_Actual = (unit->flags & UNIT_READONLY) ? TRUE : FALSE;
                    err = 0;
                    break;
                    
                } /* switch(iotd->iotd_Req.io_Command) */
            
                iotd->iotd_Req.io_Error = err;
                ReplyMsg(&iotd->iotd_Req.io_Message);
            
            } /* while((iotd = (struct IOExtTD *)GetMsg(&unit->port)) != NULL) */

        }

        /* Process quit signal after our MsgPort is empty */
        if (sigs & SIGBREAKF_CTRL_C)
        {
            D(bug("%s: Received EXIT signal.\n", me->tc_Node.ln_Name));

            Host_Close(unit);

            freeUnit(unit);
            return;
        }

    } /* for(;;) */
}

/****************************************************************************************/
