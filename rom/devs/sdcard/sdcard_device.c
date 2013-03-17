/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <exec/types.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <dos/bptr.h>
#include <devices/trackdisk.h>
#include <devices/scsidisk.h>
#include <devices/newstyle.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <oop/oop.h>

#include "sdcard_base.h"
#include "sdcard_unit.h"
#include "timer.h"

#include LC_LIBDEFS_FILE

//---------------------------IO Commands---------------------------------------

/* Invalid comand does nothing, complains only. */
static void cmd_Invalid(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[SDCard%02ld] %s: %d\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__, io->io_Command));
    io->io_Error = IOERR_NOCMD;
}

/* Don't need to reset the drive? */
static void cmd_Reset(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[SDCard%02ld] %s()\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));
    IOStdReq(io)->io_Actual = 0;
}

/* CMD_READ implementation */
static void cmd_Read32(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct sdcard_Unit *unit = (struct sdcard_Unit *)IOStdReq(io)->io_Unit;
    ULONG block = IOStdReq(io)->io_Offset;
    ULONG count = IOStdReq(io)->io_Length;
    ULONG mask;

    D(bug("[SDCard%02ld] %s(%08x, %08x)\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__, block, count));

    if (!(unit->sdcu_Bus->sdcb_BusFlags & AF_Bus_MediaPresent))
    {
        bug("[SDCard%02ld] %s: Error: No Media present\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__);
        io->io_Error = TDERR_DiskChanged;
        return;
    }

    mask = (1 << unit->sdcu_Bus->sdcb_SectorShift) - 1;

    /*
        During this IO call it should be sure that both offset and
        length are already aligned properly to sector boundaries.
    */
    if ((block & mask) | (count & mask))
    {
        bug("[SDCard%02ld] %s: offset or length not sector-aligned.\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__);
        cmd_Invalid(io, LIBBASE);
    }
    else
    {
        block >>= unit->sdcu_Bus->sdcb_SectorShift;
        count >>= unit->sdcu_Bus->sdcb_SectorShift;
        ULONG cnt = 0;

        if (((block + count) > unit->sdcu_Capacity))
        {
            bug("[SDCard%02ld] %s: Requested block (%lx;%ld) outside disk range (%lx)\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__, block, count, unit->sdcu_Capacity);
            io->io_Error = IOERR_BADADDRESS;
            return;
        }

        /* Call the Unit's access funtion */
        io->io_Error = unit->sdcu_Read32(unit, block, count,
            IOStdReq(io)->io_Data, &cnt);

        IOStdReq(io)->io_Actual = cnt;
    }
}

/*
    NSCMD_TD_READ64, TD_READ64 implementation. Basically the same, just packs
    the 64 bit offset in both io_Offset (31:0) and io_Actual (63:32)
*/
static void cmd_Read64(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct sdcard_Unit *unit = (struct sdcard_Unit *)IOStdReq(io)->io_Unit;
    UQUAD block = IOStdReq(io)->io_Offset | (UQUAD)(IOStdReq(io)->io_Actual) << 32;
    ULONG count = IOStdReq(io)->io_Length;
    ULONG mask;

    D(bug("[SDCard%02ld] %s(%08x-%08x, %08x)\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__, IOStdReq(io)->io_Actual, IOStdReq(io)->io_Offset, count));

    if (!(unit->sdcu_Bus->sdcb_BusFlags & AF_Bus_MediaPresent))
    {
        bug("[SDCard%02ld] %s: Error: No Media present\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__);
        io->io_Error = TDERR_DiskChanged;
        return;
    }

    mask = (1 << unit->sdcu_Bus->sdcb_SectorShift) - 1;

    if ((block & (UQUAD)mask) | (count & mask) | (count == 0))
    {
        D(bug("[SDCard%02ld] %s: offset or length not sector-aligned.\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));
        cmd_Invalid(io, LIBBASE);
    }
    else
    {
        block >>= unit->sdcu_Bus->sdcb_SectorShift;
        count >>= unit->sdcu_Bus->sdcb_SectorShift;
        ULONG cnt = 0;

        if (((block + count) > unit->sdcu_Capacity))
        {
            bug("[SDCard%02ld] %s: Requested block (%lx;%ld) outside disk range (%lx)\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__, block, count, unit->sdcu_Capacity);
            io->io_Error = IOERR_BADADDRESS;
            return;
        }
        io->io_Error = unit->sdcu_Read64(unit, block, count, IOStdReq(io)->io_Data, &cnt);

        IOStdReq(io)->io_Actual = cnt;
    }
}

/* CMD_WRITE implementation */
static void cmd_Write32(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct sdcard_Unit *unit = (struct sdcard_Unit *)IOStdReq(io)->io_Unit;
    ULONG block = IOStdReq(io)->io_Offset;
    ULONG count = IOStdReq(io)->io_Length;
    ULONG mask;

    D(bug("[SDCard%02ld] %s(%08x, %08x)\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__, block, count));

    if (!(unit->sdcu_Bus->sdcb_BusFlags & AF_Bus_MediaPresent))
    {
        bug("[SDCard%02ld] %s: Error: No Media present\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__);
        io->io_Error = TDERR_DiskChanged;
        return;
    }

    if (unit->sdcu_Flags & (AF_Card_WriteProtect|AF_Card_Locked))
    {
        bug("[SDCard%02ld] %s: Error: Card is Locked/Write Protected\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__);
        io->io_Error = IOERR_ABORTED;
        return;
    }

    mask = (1 << unit->sdcu_Bus->sdcb_SectorShift) - 1;

    /*
        During this IO call it should be sure that both offset and
        length are already aligned properly to sector boundaries.
    */
    if ((block & mask) | (count & mask))
    {
        D(bug("[SDCard%02ld] %s: offset or length not sector-aligned.\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));
        cmd_Invalid(io, LIBBASE);
    }
    else
    {
        block >>= unit->sdcu_Bus->sdcb_SectorShift;
        count >>= unit->sdcu_Bus->sdcb_SectorShift;
        ULONG cnt = 0;

        if (((block + count) > unit->sdcu_Capacity))
        {
            bug("[SDCard%02ld] %s: Requested block (%lx;%ld) outside disk range (%lx)\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__,
                block, count, unit->sdcu_Capacity);
            io->io_Error = IOERR_BADADDRESS;
            return;
        }

        /* Call the Unit's access funtion */
        io->io_Error = unit->sdcu_Write32(unit, block, count,
            IOStdReq(io)->io_Data, &cnt);

        IOStdReq(io)->io_Actual = cnt;
    }
}

/*
    NSCMD_TD_WRITE64, TD_WRITE64 implementation. Basically the same, just packs
    the 64 bit offset in both io_Offset (31:0) and io_Actual (63:32)
*/
static void cmd_Write64(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct sdcard_Unit *unit = (struct sdcard_Unit *)IOStdReq(io)->io_Unit;
    UQUAD block = IOStdReq(io)->io_Offset | (UQUAD)(IOStdReq(io)->io_Actual) << 32;
    ULONG count = IOStdReq(io)->io_Length;
    ULONG mask;

    D(bug("[SDCard%02ld] %s(%08x-%08x, %08x)\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__, IOStdReq(io)->io_Actual, IOStdReq(io)->io_Offset, count));

    if (!(unit->sdcu_Bus->sdcb_BusFlags & AF_Bus_MediaPresent))
    {
        bug("[SDCard%02ld] %s: Error: No Media present\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__);
        io->io_Error = TDERR_DiskChanged;
        return;
    }

    if (unit->sdcu_Flags & (AF_Card_WriteProtect|AF_Card_Locked))
    {
        bug("[SDCard%02ld] %s: Error: Card is Locked/Write Protected\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__);
        io->io_Error = IOERR_ABORTED;
        return;
    }

    mask = (1 << unit->sdcu_Bus->sdcb_SectorShift) - 1;

    if ((block & mask) | (count & mask) | (count==0))
    {
        D(bug("[SDCard%02ld] %s: offset or length not sector-aligned.\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));
        cmd_Invalid(io, LIBBASE);
    }
    else
    {
        block >>= unit->sdcu_Bus->sdcb_SectorShift;
        count >>= unit->sdcu_Bus->sdcb_SectorShift;
        ULONG cnt = 0;

        if (((block + count) > unit->sdcu_Capacity))
        {
            bug("[SDCard%02ld] %s: Requested block (%lx:%08lx;%ld) outside disk "
                "range (%lx:%08lx)\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__,
                 block>>32, block&0xfffffffful,
                 count, unit->sdcu_Capacity>>32,
                 unit->sdcu_Capacity & 0xfffffffful);
            io->io_Error = IOERR_BADADDRESS;
            return;
        }

        io->io_Error = unit->sdcu_Write64(unit, block, count,
            IOStdReq(io)->io_Data, &cnt);

        IOStdReq(io)->io_Actual = cnt;
   }
}


/* use CMD_FLUSH to force all IO waiting commands to abort */
static void cmd_Flush(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct IORequest *msg;
    D(bug("[SDCard%02ld] %s()\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));

    Forbid();

    while((msg = (struct IORequest *)GetMsg((struct MsgPort *)((struct sdcard_Unit *)io->io_Unit)->sdcu_Bus->sdcb_MsgPort)))
    {
        msg->io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *)msg);
    }

    Permit();
}

/*
    Internal command used to check whether the media in drive has been changed
    since last call. If so, the handlers given by user are called.
*/
static void cmd_TestChanged(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct sdcard_Unit *unit = (struct sdcard_Unit *)io->io_Unit;
    struct IORequest *msg;

    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    if (unit->sdcu_Bus->sdcb_BusFlags & AF_Bus_MediaChanged)
    {
        unit->sdcu_ChangeNum++;

        Forbid();

        /* old-fashioned RemoveInt call first */
        if (unit->sdcu_RemoveInt)
            Cause(unit->sdcu_RemoveInt);

        /* And now the whole list of possible calls */
        ForeachNode(&unit->sdcu_SoftList, msg)
        {
            Cause((struct Interrupt *)IOStdReq(msg)->io_Data);
        }

        unit->sdcu_Bus->sdcb_BusFlags &= ~AF_Bus_MediaChanged;

        Permit();
    }
}

static void cmd_Update(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[SDCard%02ld] %s()\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));

    /* Do nothing now. In near future there should be drive cache flush though */
}

static void cmd_Remove(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct sdcard_Unit *unit = (struct sdcard_Unit *)io->io_Unit;

    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    if (unit->sdcu_RemoveInt)
        io->io_Error = TDERR_DriveInUse;
    else
        unit->sdcu_RemoveInt = IOStdReq(io)->io_Data;
}

static void cmd_ChangeNum(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[SDCard%02ld] %s()\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));

    IOStdReq(io)->io_Actual = ((struct sdcard_Unit *)io->io_Unit)->sdcu_ChangeNum;
}

static void cmd_ChangeState(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct sdcard_Unit *unit = (struct sdcard_Unit *)io->io_Unit;

    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    if (unit->sdcu_Bus->sdcb_BusFlags & AF_Bus_MediaPresent)
        IOStdReq(io)->io_Actual = 0;
    else
        IOStdReq(io)->io_Actual = 1;

    D(bug("[SDCard%02ld] %s: Media %s\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__, IOStdReq(io)->io_Actual ? "ABSENT" : "PRESENT"));
}

static void cmd_ProtStatus(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct sdcard_Unit *unit = (struct sdcard_Unit *)io->io_Unit;

    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    if ((unit->sdcu_Flags & (AF_Card_WriteProtect|AF_Card_Locked)) != 0)
        IOStdReq(io)->io_Actual = -1;
    else
        IOStdReq(io)->io_Actual = 0;
}

static void cmd_GetNumTracks(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[SDCard%02ld] %s()\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));

    IOStdReq(io)->io_Actual = ((struct sdcard_Unit *)io->io_Unit)->sdcu_Cylinders;
}

static void cmd_AddChangeInt(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct sdcard_Unit *unit = (struct sdcard_Unit *)io->io_Unit;

    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    Forbid();
    AddHead(&unit->sdcu_SoftList, (struct Node *)io);
    Permit();

    io->io_Flags &= ~IOF_QUICK;
    unit->sdcu_Unit.unit_flags &= ~UNITF_ACTIVE;
}

static void cmd_RemChangeInt(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[SDCard%02ld] %s()\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));

    Forbid();
    Remove((struct Node *)io);
    Permit();
}

static void cmd_Eject(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct sdcard_Unit *unit = (struct sdcard_Unit *)io->io_Unit;

    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    IOStdReq(io)->io_Error = unit->sdcu_Eject(unit);
    cmd_TestChanged(io, LIBBASE);
}

static void cmd_GetGeometry(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct sdcard_Unit *unit = (struct sdcard_Unit *)io->io_Unit;

    D(bug("[SDCard%02ld] %s()\n", unit->sdcu_UnitNum, __PRETTY_FUNCTION__));

    if (IOStdReq(io)->io_Length == sizeof(struct DriveGeometry))
    {
        struct DriveGeometry *dg = (struct DriveGeometry *)IOStdReq(io)->io_Data;

        dg->dg_SectorSize       = 1 << unit->sdcu_Bus->sdcb_SectorShift;

        if ((unit->sdcu_Capacity >> 32) != 0)
            dg->dg_TotalSectors     = 0xffffffff;
        else
            dg->dg_TotalSectors     = unit->sdcu_Capacity;

        dg->dg_Cylinders                = unit->sdcu_Cylinders;
        dg->dg_CylSectors               = unit->sdcu_Sectors * unit->sdcu_Heads;
        dg->dg_Heads                    = unit->sdcu_Heads;
        dg->dg_TrackSectors             = unit->sdcu_Sectors;
        dg->dg_BufMemType               = MEMF_PUBLIC;
        dg->dg_DeviceType               = DG_DIRECT_ACCESS;
        dg->dg_Flags                    = DGF_REMOVABLE;
        dg->dg_Reserved                 = 0;

        IOStdReq(io)->io_Actual = sizeof(struct DriveGeometry);
    }
    else io->io_Error = TDERR_NotSpecified;
}

static void cmd_DirectSCSI(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[SDCard%02ld] %s()\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));

    IOStdReq(io)->io_Actual = sizeof(struct SCSICmd);
    io->io_Error = IOERR_BADADDRESS;
}

//-----------------------------------------------------------------------------

/*
    command translation tables - used to call proper IO functions.
*/

#define N_TD_READ64     0
#define N_TD_WRITE64    1
#define N_TD_SEEK64     2
#define N_TD_FORMAT64   3

typedef void (*mapfunc)(struct IORequest *, LIBBASETYPEPTR);

static mapfunc const map64[]= {
    [N_TD_READ64]   = cmd_Read64,
    [N_TD_WRITE64]  = cmd_Write64,
    [N_TD_SEEK64]   = cmd_Reset,
    [N_TD_FORMAT64] = cmd_Write64
};

static mapfunc const map32[] = {
    [CMD_INVALID]   = cmd_Invalid,
    [CMD_RESET]     = cmd_Reset,
    [CMD_READ]      = cmd_Read32,
    [CMD_WRITE]     = cmd_Write32,
    [CMD_UPDATE]    = cmd_Update,
    [CMD_CLEAR]     = cmd_Reset,
    [CMD_STOP]      = cmd_Reset,
    [CMD_START]     = cmd_Reset,
    [CMD_FLUSH]     = cmd_Flush,
    [TD_MOTOR]      = cmd_Reset,
    [TD_SEEK]       = cmd_Reset,
    [TD_FORMAT]     = cmd_Write32,
    [TD_REMOVE]     = cmd_Remove,
    [TD_CHANGENUM]  = cmd_ChangeNum,
    [TD_CHANGESTATE]= cmd_ChangeState,
    [TD_PROTSTATUS] = cmd_ProtStatus,
    [TD_RAWREAD]    = cmd_Invalid,
    [TD_RAWWRITE]   = cmd_Invalid,
    [TD_GETNUMTRACKS]       = cmd_GetNumTracks,
    [TD_ADDCHANGEINT]       = cmd_AddChangeInt,
    [TD_REMCHANGEINT]       = cmd_RemChangeInt,
    [TD_GETGEOMETRY]= cmd_GetGeometry,
    [TD_EJECT]      = cmd_Eject,
    [TD_READ64]     = cmd_Read64,
    [TD_WRITE64]    = cmd_Write64,
    [TD_SEEK64]     = cmd_Reset,
    [TD_FORMAT64]   = cmd_Write64,
    [HD_SCSICMD]    = cmd_DirectSCSI,
    [HD_SCSICMD+1]  = cmd_TestChanged,
};

static UWORD const NSDSupported[] = {
    CMD_RESET,
    CMD_READ,
    CMD_WRITE,
    CMD_UPDATE,
    CMD_CLEAR,
    CMD_STOP,
    CMD_START,
    CMD_FLUSH,
    TD_MOTOR,
    TD_SEEK,
    TD_FORMAT,
    TD_REMOVE,
    TD_CHANGENUM,
    TD_CHANGESTATE,
    TD_PROTSTATUS,
    TD_GETNUMTRACKS,
    TD_ADDCHANGEINT,
    TD_REMCHANGEINT,
    TD_GETGEOMETRY,
    TD_EJECT,
    TD_READ64,
    TD_WRITE64,
    TD_SEEK64,
    TD_FORMAT64,
    HD_SCSICMD,
    TD_GETDRIVETYPE,
    NSCMD_DEVICEQUERY,
    NSCMD_TD_READ64,
    NSCMD_TD_WRITE64,
    NSCMD_TD_SEEK64,
    NSCMD_TD_FORMAT64,
    0
};

/*
    Do proper IO actions depending on the request. It's called from the bus
    tasks and from BeginIO in case of immediate commands.
*/
BOOL FNAME_SDC(HandleIO)(struct IORequest *io)
{
    BOOL retval = TRUE;
    LIBBASETYPEPTR LIBBASE = ((struct sdcard_Unit*)io->io_Unit)->sdcu_Bus->sdcb_DeviceBase;

    io->io_Error = 0;

    D(bug("[SDCard%02ld] %s()\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));

    /* Handle few commands directly here */
    switch (io->io_Command)
    {
        /*
            New Style Devices query. Introduce self as trackdisk and provide list of
            commands supported
        */
        case NSCMD_DEVICEQUERY:
            {
                struct NSDeviceQueryResult *nsdq = (struct NSDeviceQueryResult *)IOStdReq(io)->io_Data;
                nsdq->DevQueryFormat    = 0;
                nsdq->SizeAvailable     = sizeof(struct NSDeviceQueryResult);
                nsdq->DeviceType        = NSDEVTYPE_TRACKDISK;
                nsdq->DeviceSubType     = 0;
                nsdq->SupportedCommands = (UWORD *)NSDSupported;
            }
            IOStdReq(io)->io_Actual = sizeof(struct NSDeviceQueryResult);
            break;

        /*
            New Style Devices report here the 'NSTY' - only if such value is
            returned here, the NSCMD_DEVICEQUERY might be called. Otherwice it should
            report error.
        */
        case TD_GETDRIVETYPE:
            IOStdReq(io)->io_Actual = DRIVE_NEWSTYLE;
            break;

        case TD_ADDCHANGEINT:
            retval = FALSE;
        /*
            Call all other commands using the command pointer tables for 32- and
            64-bit accesses. If requested function is defined call it, otherwise
            make the function cmd_Invalid.
        */
        default:
            if (io->io_Command <= (HD_SCSICMD + 1))
            {
                if (map32[io->io_Command])
                    map32[io->io_Command](io, LIBBASE);
                else
                    cmd_Invalid(io, LIBBASE);
            }
            else if (io->io_Command >= NSCMD_TD_READ64 && io->io_Command <= NSCMD_TD_FORMAT64)
            {
                if (map64[io->io_Command - NSCMD_TD_READ64])
                    map64[io->io_Command - NSCMD_TD_READ64](io, LIBBASE);
                else
                    cmd_Invalid(io, LIBBASE);
            }
            else cmd_Invalid(io, LIBBASE);
            break;
    }
    return retval;
}


static const ULONG IMMEDIATE_COMMANDS = 0x803ff1e3; // 10000000001111111111000111100011

/* See whether the command can be done quick */
BOOL isSlow(ULONG comm)
{
    BOOL slow = TRUE;   /* Assume always slow command */

    /* For commands with numbers <= 31 check the mask */
    if (comm <= 31)
    {
        if (IMMEDIATE_COMMANDS & (1 << comm))
            slow = FALSE;
    }
    else if (comm == NSCMD_TD_SEEK64) slow = FALSE;

    return slow;
}

/*
    Try to do IO commands. All commands which require talking with mci devices
    will be handled slow, that is they will be passed to bus task which will
    execute them as soon as hardware will be free.
*/
AROS_LH1(void, BeginIO,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 5, sdcard)
{
    AROS_LIBFUNC_INIT

    struct sdcard_Unit *unit = (struct sdcard_Unit *)io->io_Unit;

    io->io_Message.mn_Node.ln_Type = NT_MESSAGE;

    /* Disable interrupts for a while to modify message flags */
    Disable();

    D(bug("[SDCard%02ld] %s: Executing IO Command %lx\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__, io->io_Command));

    /*
        If the command is not-immediate, or presence of disc is still unknown,
        let the bus task do the job.
    */
    if (isSlow(io->io_Command))
    {
        unit->sdcu_Unit.unit_flags |= UNITF_ACTIVE | UNITF_INTASK;
        io->io_Flags &= ~IOF_QUICK;
        Enable();

        /* Put the message to the bus */
        PutMsg(unit->sdcu_Bus->sdcb_MsgPort, (struct Message *)io);
    }
    else
    {
        D(bug("[SDCard%02ld] %s: Fast command\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));
    
        /* Immediate command. Mark unit as active and do the command directly */
        unit->sdcu_Unit.unit_flags |= UNITF_ACTIVE;
        Enable();
        if (FNAME_SDC(HandleIO)(io))
        {
            unit->sdcu_Unit.unit_flags &= ~UNITF_ACTIVE;
            if (!(io->io_Flags & IOF_QUICK))
                ReplyMsg((struct Message *)io);
        }
        else
            unit->sdcu_Unit.unit_flags &= ~UNITF_ACTIVE;
    }

    D(bug("[SDCard%02ld] %s: Done\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 6, sdcard)
{
    AROS_LIBFUNC_INIT

    D(bug("[SDCard%02ld] %s()\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));

    /* Cannot Abort IO */
    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, GetRdskLba,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 7, sdcard)
{
    AROS_LIBFUNC_INIT

    D(bug("[SDCard%02ld] %s()\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, GetBlkSize,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 8, sdcard)
{
    AROS_LIBFUNC_INIT

    D(bug("[SDCard%02ld] %s()\n", ((struct sdcard_Unit*)io->io_Unit)->sdcu_UnitNum, __PRETTY_FUNCTION__));

    return Unit(io)->sdcu_Bus->sdcb_SectorShift;

    AROS_LIBFUNC_EXIT
}
