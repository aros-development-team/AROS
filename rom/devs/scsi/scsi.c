/*
    Copyright © 2019, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/oop.h>

#include <exec/exec.h>
#include <exec/resident.h>
#include <hidd/hidd.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <oop/oop.h>
#include <dos/bptr.h>

//#include <devices/scsi.h>

#include "timer.h"
#include "scsi.h"

#include LC_LIBDEFS_FILE

#define DINIT(x)

//---------------------------IO Commands---------------------------------------

/* Invalid comand does nothing, complains only. */
static void cmd_Invalid(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[SCSI%02ld] %s(%d)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__, io->io_Command));
    io->io_Error = IOERR_NOCMD;
}

/* Don't need to reset the drive? */
static void cmd_Reset(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    IOStdReq(io)->io_Actual = 0;
}

/* CMD_READ implementation */
static void cmd_Read32(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct scsi_Unit *unit = (struct scsi_Unit *)IOStdReq(io)->io_Unit;

    if (AF_Removable == (unit->su_Flags & (AF_Removable | AF_DiscPresent)))
    {
        D(bug("[SCSI%02ld] %s: USUALLY YOU'D WANT TO CHECK IF DISC IS PRESENT FIRST\n", unit->su_UnitNum, __func__));
        io->io_Error = TDERR_DiskChanged;
        return;
    }

    ULONG block = IOStdReq(io)->io_Offset;
    ULONG count = IOStdReq(io)->io_Length;

    D(bug("[SCSI%02ld] %s(%08x, %08x)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__, block, count));

    ULONG mask = (1 << unit->su_SectorShift) - 1;

    /*
        During this IO call it should be sure that both offset and
        length are already aligned properly to sector boundaries.
    */
    if ((block & mask) | (count & mask))
    {
        D(bug("[SCSI%02ld] %s: offset or length not sector-aligned.\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));
        cmd_Invalid(io, LIBBASE);
    }
    else
    {
        block >>= unit->su_SectorShift;
        count >>= unit->su_SectorShift;
        ULONG cnt = 0;

        if ((0 == (unit->su_XferModes & AF_XFER_PACKET)) && ((block + count) > unit->su_Capacity))
        {
            bug("[SCSI%02ld] %s: Requested block (%lx;%ld) outside disk range (%lx)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__, block, count, unit->su_Capacity);
            io->io_Error = IOERR_BADADDRESS;
            return;
        }

        /* Call the Unit's access funtion */
        io->io_Error = unit->su_Read32(unit, block, count,
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
    struct scsi_Unit *unit = (struct scsi_Unit *)IOStdReq(io)->io_Unit;

    if (AF_Removable == (unit->su_Flags & (AF_Removable | AF_DiscPresent)))
    {
        D(bug("[SCSI%02ld] %s: USUALLY YOU'D WANT TO CHECK IF DISC IS PRESENT FIRST\n", unit->su_UnitNum, __func__));
        io->io_Error = TDERR_DiskChanged;
        return;
    }

    UQUAD block = IOStdReq(io)->io_Offset | (UQUAD)(IOStdReq(io)->io_Actual) << 32;
    ULONG count = IOStdReq(io)->io_Length;

    D(bug("[SCSI%02ld] %s(%08x-%08x, %08x)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__, IOStdReq(io)->io_Actual, IOStdReq(io)->io_Offset, count));

    ULONG mask = (1 << unit->su_SectorShift) - 1;

    if ((block & (UQUAD)mask) | (count & mask) | (count == 0))
    {
        D(bug("[SCSI%02ld] %s: offset or length not sector-aligned.\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));
        cmd_Invalid(io, LIBBASE);
    }
    else
    {
        block >>= unit->su_SectorShift;
        count >>= unit->su_SectorShift;
        ULONG cnt = 0;

        /*
            If the sum of sector offset and the sector count doesn't overflow
            the 28-bit LBA address, use 32-bit access for speed and simplicity.
            Otherwise do the 48-bit LBA addressing.
        */
        if ((block + count) < 0x0fffffff)
        {
            if ((0 == (unit->su_XferModes & AF_XFER_PACKET)) && ((block + count) > unit->su_Capacity))
            {
                bug("[SCSI%02ld] %s: Requested block (%lx;%ld) outside disk range (%lx)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__, block, count, unit->su_Capacity);
                io->io_Error = IOERR_BADADDRESS;
                return;
            }
            io->io_Error = unit->su_Read32(unit, (ULONG)(block & 0x0fffffff), count, IOStdReq(io)->io_Data, &cnt);
        }
        else
        {
            if ((0 == (unit->su_XferModes & AF_XFER_PACKET)) && ((block + count) > unit->su_Capacity48))
            {
                bug("[SCSI%02ld] %s: Requested block (%lx:%08lx;%ld) outside disk range (%lx:%08lx)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__, block>>32, block&0xfffffffful, count, unit->su_Capacity48>>32, unit->su_Capacity48 & 0xfffffffful);
                io->io_Error = IOERR_BADADDRESS;
                return;
            }

            io->io_Error = unit->su_Read64(unit, block, count, IOStdReq(io)->io_Data, &cnt);
        }

        IOStdReq(io)->io_Actual = cnt;
    }
}

/* CMD_WRITE implementation */
static void cmd_Write32(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct scsi_Unit *unit = (struct scsi_Unit *)IOStdReq(io)->io_Unit;

    if (AF_Removable == (unit->su_Flags & (AF_Removable | AF_DiscPresent)))
    {
        D(bug("[SCSI%02ld] %s: USUALLY YOU'D WANT TO CHECK IF DISC IS PRESENT FIRST\n", unit->su_UnitNum, __func__));
        io->io_Error = TDERR_DiskChanged;
        return;
    }

    ULONG block = IOStdReq(io)->io_Offset;
    ULONG count = IOStdReq(io)->io_Length;

    D(bug("[SCSI%02ld] %s(%08x, %08x)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__, block, count));

    ULONG mask = (1 << unit->su_SectorShift) - 1;

    /*
        During this IO call it should be sure that both offset and
        length are already aligned properly to sector boundaries.
    */
    if ((block & mask) | (count & mask))
    {
        D(bug("[SCSI%02ld] %s: offset or length not sector-aligned.\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));
        cmd_Invalid(io, LIBBASE);
    }
    else
    {
        block >>= unit->su_SectorShift;
        count >>= unit->su_SectorShift;
        ULONG cnt = 0;

        if ((0 == (unit->su_XferModes & AF_XFER_PACKET))
            && ((block + count) > unit->su_Capacity))
        {
            bug("[SCSI%02ld] %s: Requested block (%lx;%ld) outside disk range (%lx)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum,
                __func__,
                block, count, unit->su_Capacity);
            io->io_Error = IOERR_BADADDRESS;
            return;
        }

        /* Call the Unit's access funtion */
        io->io_Error = unit->su_Write32(unit, block, count,
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
    struct scsi_Unit *unit = (struct scsi_Unit *)IOStdReq(io)->io_Unit;

    if (AF_Removable == (unit->su_Flags & (AF_Removable | AF_DiscPresent)))
    {
        D(bug("[SCSI%02ld] %s: USUALLY YOU'D WANT TO CHECK IF DISC IS PRESENT FIRST\n", unit->su_UnitNum, __func__));
        io->io_Error = TDERR_DiskChanged;
        return;
    }

    UQUAD block = IOStdReq(io)->io_Offset | (UQUAD)(IOStdReq(io)->io_Actual) << 32;
    ULONG count = IOStdReq(io)->io_Length;

    D(bug("[SCSI%02ld] %s(%08x-%08x, %08x)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__, IOStdReq(io)->io_Actual, IOStdReq(io)->io_Offset, count));

    ULONG mask = (1 << unit->su_SectorShift) - 1;

    if ((block & mask) | (count & mask) | (count==0))
    {
        D(bug("[SCSI%02ld] %s: offset or length not sector-aligned.\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));
        cmd_Invalid(io, LIBBASE);
    }
    else
    {
        block >>= unit->su_SectorShift;
        count >>= unit->su_SectorShift;
        ULONG cnt = 0;

        /*
            If the sum of sector offset and the sector count doesn't overflow
            the 28-bit LBA address, use 32-bit access for speed and simplicity.
            Otherwise do the 48-bit LBA addressing.
        */
        if ((block + count) < 0x0fffffff)
        {
            if ((0 == (unit->su_XferModes & AF_XFER_PACKET))
                && ((block + count) > unit->su_Capacity))
            {
                bug("[SCSI%02ld] %s: Requested block (%lx;%ld) outside disk range "
                    "(%lx)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__, block, count, unit->su_Capacity);
                io->io_Error = IOERR_BADADDRESS;
                return;
            }
            io->io_Error = unit->su_Write32(unit, (ULONG)(block & 0x0fffffff),
                count, IOStdReq(io)->io_Data, &cnt);
        }
        else
        {
            if ((0 == (unit->su_XferModes & AF_XFER_PACKET))
                && ((block + count) > unit->su_Capacity48))
            {
                bug("[SCSI%02ld] %s: Requested block (%lx:%08lx;%ld) outside disk "
                    "range (%lx:%08lx)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum,
                     __func__,
                     block>>32, block&0xfffffffful,
                     count, unit->su_Capacity48>>32,
                     unit->su_Capacity48 & 0xfffffffful);
                io->io_Error = IOERR_BADADDRESS;
                return;
            }

            io->io_Error = unit->su_Write64(unit, block, count,
                IOStdReq(io)->io_Data, &cnt);
        }
        IOStdReq(io)->io_Actual = cnt;
   }
}


/* use CMD_FLUSH to force all IO waiting commands to abort */
static void cmd_Flush(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct IORequest *msg;
    struct scsi_Bus *bus = ((struct scsi_Unit *)io->io_Unit)->su_Bus;

    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));
 
    Forbid();

    while((msg = (struct IORequest *)GetMsg((struct MsgPort *)bus->sb_MsgPort)))
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
    struct scsi_Unit *unit = (struct scsi_Unit *)io->io_Unit;
    struct IORequest *msg;

    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

    if ((unit->su_XferModes & AF_XFER_PACKET) && (unit->su_Flags & AF_Removable))
    {
#if (0)
        atapi_TestUnitOK(unit);
#endif
        if (unit->su_Flags & AF_DiscChanged)
        {
            unit->su_ChangeNum++;

            Forbid();

            /* old-fashioned RemoveInt call first */
            if (unit->su_RemoveInt)
                Cause(unit->su_RemoveInt);

            /* And now the whole list of possible calls */
            ForeachNode(&unit->su_SoftList, msg)
            {
                Cause((struct Interrupt *)IOStdReq(msg)->io_Data);
            }

            unit->su_Flags &= ~AF_DiscChanged;

            Permit();
        }
    }
}

static void cmd_Update(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    /* Do nothing now. In near future there should be drive cache flush though */
    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));
}

static void cmd_Remove(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct scsi_Unit *unit = (struct scsi_Unit *)io->io_Unit;

    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

    if (unit->su_RemoveInt)
        io->io_Error = TDERR_DriveInUse;
    else
        unit->su_RemoveInt = IOStdReq(io)->io_Data;
}

static void cmd_ChangeNum(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

    IOStdReq(io)->io_Actual = ((struct scsi_Unit *)io->io_Unit)->su_ChangeNum;
}

static void cmd_ChangeState(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct scsi_Unit *unit = (struct scsi_Unit *)io->io_Unit;

    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

    if (unit->su_Flags & AF_DiscPresent)
        IOStdReq(io)->io_Actual = 0;
    else
        IOStdReq(io)->io_Actual = 1;

    D(bug("[SCSI%02ld] %s: Media %s\n", unit->su_UnitNum, __func__, IOStdReq(io)->io_Actual ? "ABSENT" : "PRESENT"));
}

static void cmd_ProtStatus(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct scsi_Unit *unit = (struct scsi_Unit *)io->io_Unit;

    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

    if (unit->su_DevType)
        IOStdReq(io)->io_Actual = -1;
    else
        IOStdReq(io)->io_Actual = 0;

}

static void cmd_GetNumTracks(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

    IOStdReq(io)->io_Actual = ((struct scsi_Unit *)io->io_Unit)->su_Cylinders;
}

static void cmd_AddChangeInt(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct scsi_Unit *unit = (struct scsi_Unit *)io->io_Unit;

    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

    Forbid();
    AddHead(&unit->su_SoftList, (struct Node *)io);
    Permit();

    io->io_Flags &= ~IOF_QUICK;
    unit->su_Unit.unit_flags &= ~UNITF_ACTIVE;
}

static void cmd_RemChangeInt(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

    Forbid();
    Remove((struct Node *)io);
    Permit();
}

static void cmd_Eject(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct scsi_Unit *unit = (struct scsi_Unit *)io->io_Unit;

    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

    IOStdReq(io)->io_Error = unit->su_Eject(unit);
    cmd_TestChanged(io, LIBBASE);
}

static void cmd_GetGeometry(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct scsi_Unit *unit = (struct scsi_Unit *)io->io_Unit;

    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

    if (IOStdReq(io)->io_Length == sizeof(struct DriveGeometry))
    {
        struct DriveGeometry *dg = (struct DriveGeometry *)IOStdReq(io)->io_Data;

        dg->dg_SectorSize       = 1 << unit->su_SectorShift;

        if (unit->su_Capacity48 != 0)
        {
            if ((unit->su_Capacity48 >> 32) != 0)
                dg->dg_TotalSectors     = 0xffffffff;
            else
                dg->dg_TotalSectors     = unit->su_Capacity48;
        }
        else
            dg->dg_TotalSectors         = unit->su_Capacity;

        dg->dg_Cylinders                = unit->su_Cylinders;
        dg->dg_CylSectors               = unit->su_Sectors * unit->su_Heads;
        dg->dg_Heads                    = unit->su_Heads;
        dg->dg_TrackSectors             = unit->su_Sectors;
        dg->dg_BufMemType               = MEMF_PUBLIC;
        dg->dg_DeviceType               = unit->su_DevType;
        if (dg->dg_DeviceType != DG_DIRECT_ACCESS)
            dg->dg_Flags                    = (unit->su_Flags & AF_Removable) ? DGF_REMOVABLE : 0;
        else
            dg->dg_Flags                = 0;
        dg->dg_Reserved                 = 0;

        IOStdReq(io)->io_Actual = sizeof(struct DriveGeometry);
    }
    else if (IOStdReq(io)->io_Length == 514)
    {
        CopyMemQuick(unit->su_Drive, IOStdReq(io)->io_Data, 512);
    }
    else io->io_Error = TDERR_NotSpecified;
}

static void cmd_DirectScsi(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct scsi_Unit *unit = (struct scsi_Unit *)io->io_Unit;

    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

    IOStdReq(io)->io_Actual = sizeof(struct SCSICmd);
#if (0)
    if (unit->su_XferModes & AF_XFER_PACKET)
    {
        io->io_Error = unit->su_DirectSCSI(unit, (struct SCSICmd *)IOStdReq(io)->io_Data);
    }
    else if (unit->su_DevType == DG_DIRECT_ACCESS)
    {
        io->io_Error = SCSIEmu(unit, (struct SCSICmd *)IOStdReq(io)->io_Data);
    }
    else io->io_Error = IOERR_BADADDRESS;
#else
    io->io_Error = IOERR_BADADDRESS;
#endif
}

static BOOL ValidSMARTCmd(struct IORequest *io)
{
#if (0)
    if ((IOStdReq(io)->io_Reserved1) != (IOStdReq(io)->io_Reserved2))
        return FALSE;

    switch (IOStdReq(io)->io_Reserved1)
    {
        case SMARTC_TEST_AVAIL:
        case SMARTC_READ_VALUES:
        case SMARTC_READ_THRESHOLDS:
                    if (!IOStdReq(io)->io_Data) 
                    {
                        D(bug("[SCSI%02ld] %s: invalid io_Data (%p)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__, IOStdReq(io)->io_Data));
                        io->io_Error = IOERR_BADADDRESS;
                        return FALSE;
                    }
                    if ((IOStdReq(io)->io_Offset != SMARTC_TEST_AVAIL) && (IOStdReq(io)->io_Length != SMART_DSCSI_LENGTH))
                    {
                        D(bug("[SCSI%02ld] %s: invalid io_Length (%d)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__, IOStdReq(io)->io_Length));
                        io->io_Error = IOERR_BADLENGTH;
                        return FALSE;
                    }
                    break;

            case SMARTC_ENABLE:
            case SMARTC_DISABLE:
            case SMARTC_STATUS:
                    break;

            default:
                    D(bug("[SCSI%02ld] %s: invalid SMART command (%d)\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__, IOStdReq(io)->io_Offset));
                    io->io_Error = IOERR_NOCMD;
                    return FALSE;
        }
#endif
        return TRUE;
}

static void cmd_SMART(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct scsi_Unit *unit = (struct scsi_Unit *)io->io_Unit;
#if (0)
    struct scsi_Bus *bus = unit->su_Bus;
    UBYTE u;
#endif
    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

    if (unit->su_Flags & AF_DiscPresent)
    {
        io->io_Error = IOERR_OPENFAIL;
        return;
    }

    if (!ValidSMARTCmd(io))
        return;
#if (0)
    u = unit->su_UnitNum & 1;

    if (bus->sb_Dev[u] == DEV_ATA || bus->sb_Dev[u] == DEV_SATA)
    {
        if (IOStdReq(io)->io_Reserved1 == ATAFEATURE_TEST_AVAIL)
        {
            if (IOStdReq(io)->io_Length >= sizeof(ULONG))
            {
                *((ULONG *)IOStdReq(io)->io_Data) = SMART_MAGIC_ID;
                IOStdReq(io)->io_Actual = sizeof(ULONG);
            }
            io->io_Error = 0;
            return;
        }
        scsi_SMARTCmd(IOStdReq(io));
    }
    else
#endif
        io->io_Error = IOERR_NOCMD;
}

static void cmd_TRIM(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct scsi_Unit *unit = (struct scsi_Unit *)io->io_Unit;

    D(bug("[SCSI%02ld] %s()\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

    if ((unit->su_Drive->id_SCSIVersion >= 7) && (unit->su_Drive->id_DSManagement & 1))
    {
        D(bug("[SCSI%02ld] %s: Unit supports TRIM\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));
#if (0)
        if (IOStdReq(io)->io_Reserved1 == ATAFEATURE_TEST_AVAIL)
        {
            if (IOStdReq(io)->io_Length >= sizeof(ULONG))
            {
                *((ULONG *)IOStdReq(io)->io_Data) = TRIM_MAGIC_ID;
                IOStdReq(io)->io_Actual = sizeof(ULONG);
            }
            io->io_Error = 0;
            return;
        }
        scsi_TRIMCmd(IOStdReq(io));
#endif
    }
    else
        io->io_Error = IOERR_NOCMD;
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
    [HD_SCSICMD]    = cmd_DirectScsi,
    [HD_SCSICMD+1]  = cmd_TestChanged
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
static void HandleIO(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    io->io_Error = 0;

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

        /*
            Call all other commands using the command pointer tables for 32- and
            64-bit accesses. If requested function is defined call it, otherwise
            make the function cmd_Invalid.
        */
        default:
            if (io->io_Command <= (HD_SCSICMD+1))
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
}


static const ULONG IMMEDIATE_COMMANDS = 0x803ff1e3; // 10000000001111111111000111100011

/* See whether the command can be done quick */
static BOOL isSlow(struct IORequest *io)
{
    BOOL slow = TRUE;   /* Assume always slow command */

    /* For commands with numbers <= 31 check the mask */
    if (io->io_Command <= 31)
    {
        if (IMMEDIATE_COMMANDS & (1 << io->io_Command))
            slow = FALSE;
    }
#if(0)
    else if ((io->io_Command >= HD_SMARTCMD && io->io_Command <= HD_TRIMCMD) &&
                (IOStdReq(io)->io_Reserved1 == ATAFEATURE_TEST_AVAIL)) slow = FALSE;
#endif
    else if (io->io_Command == NSCMD_TD_SEEK64 || io->io_Command == NSCMD_DEVICEQUERY) slow = FALSE;

    return slow;
}

/*
    Try to do IO commands. All commands which require talking with scsi devices
    will be handled slow, that is they will be passed to bus task which will
    execute them as soon as hardware will be free.
*/
AROS_LH1(void, BeginIO,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 5, scsi)
{
    AROS_LIBFUNC_INIT

    struct scsi_Unit *unit = (struct scsi_Unit *)io->io_Unit;

    io->io_Message.mn_Node.ln_Type = NT_MESSAGE;

    /* Disable interrupts for a while to modify message flags */
    Disable();

    D(bug("[SCSI%02ld] %s: Executing IO Command %lx\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__, io->io_Command));

    /*
        If the command is not-immediate, or presence of disc is still unknown,
        let the bus task do the job.
    */
    if (isSlow(io))
    {
        unit->su_Unit.unit_flags |= UNITF_ACTIVE | UNITF_INTASK;
        io->io_Flags &= ~IOF_QUICK;
        Enable();

        /* Put the message to the bus */
        PutMsg(unit->su_Bus->sb_MsgPort, (struct Message *)io);
    }
    else
    {
        D(bug("[SCSI%02ld] %s: ->Fast command\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));

        /* Immediate command. Mark unit as active and do the command directly */
        unit->su_Unit.unit_flags |= UNITF_ACTIVE;
        Enable();
        HandleIO(io, LIBBASE);

        unit->su_Unit.unit_flags &= ~UNITF_ACTIVE;

        /*
            If the command was not intended to be immediate and it was not the
            TD_ADDCHANGEINT, reply to confirm command execution now.
        */
        if (!(io->io_Flags & IOF_QUICK) && (io->io_Command != TD_ADDCHANGEINT))
        {
            ReplyMsg((struct Message *)io);
        }
    }

    D(bug("[SCSI%02ld] %s: Done\n", ((struct scsi_Unit*)io->io_Unit)->su_UnitNum, __func__));
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 6, scsi)
{
    AROS_LIBFUNC_INIT

    /* Cannot Abort IO */
    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, GetRdskLba,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 7, scsi)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, GetBlkSize,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 8, scsi)
{
    AROS_LIBFUNC_INIT

    return Unit(io)->su_SectorShift;

    AROS_LIBFUNC_EXIT
}

/*
 * The daemon of scsi.device first opens all ATAPI devices and then enters
 * endless loop. Every 2 seconds it tells ATAPI units to check the media
 * presence. In case of any state change they will rise user-specified
 * functions.
 * The check is done by sending HD_SCSICMD+1 command (internal testchanged
 * command). ATAPI units should already handle the command further.
 */
void DaemonCode(LIBBASETYPEPTR LIBBASE)
{
    struct IORequest *timer;	// timer
    UBYTE b = 0;
    ULONG sigs;

    D(bug("[SCSI**] You woke up DAEMON\n"));

    /*
     * Prepare message ports and timer.device's request
     */
    timer  = scsi_OpenTimer(LIBBASE);
    if (!timer)
    {
        D(bug("[SCSI++] Failed to open timer!\n"));

        Forbid();
        Signal(LIBBASE->daemonParent, SIGF_SINGLE);
        return;
    }

    /* Calibrate 400ns delay */
    if (!scsi_Calibrate(timer, LIBBASE))
    {
        scsi_CloseTimer(timer);
        Forbid();
        Signal(LIBBASE->daemonParent, SIGF_SINGLE);
        return;
    }

    /* This also signals that we have initialized successfully */
    LIBBASE->scsi_Daemon = FindTask(NULL);
    Signal(LIBBASE->daemonParent, SIGF_SINGLE);

    D(bug("[SCSI++] Starting sweep medium presence detection\n"));

    /*
     * Endless loop
     */
    do
    {
        /*
         * call separate IORequest for every ATAPI device
         * we're calling HD_SCSICMD+1 command here -- anything like test unit ready?
         * FIXME: This is not a very nice approach in terms of performance.
         * This inserts own command into command queue every 2 seconds, so
         * this would give periodic performance drops under high loads.
         * It would be much better if unit tasks ping their devices by themselves,
         * when idle. This would also save us from lots of headaches with dealing
         * with list of these requests. Additionally i start disliking all these
         * semaphores.
         */
        if (0 == (b & 1))
        {
            struct IOStdReq *ios;

            DB2(bug("[SCSI++] Detecting media presence\n"));
            ObtainSemaphore(&LIBBASE->DaemonSem);

            ForeachNode(&LIBBASE->Daemon_ios, ios)
            {
                /* Using the request will clobber its Node. Save links. */
                struct Node *s = ios->io_Message.mn_Node.ln_Succ;
                struct Node *p = ios->io_Message.mn_Node.ln_Pred;

                DoIO((struct IORequest *)ios);

                ios->io_Message.mn_Node.ln_Succ = s;
                ios->io_Message.mn_Node.ln_Pred = p;
            }

            ReleaseSemaphore(&LIBBASE->DaemonSem);
        }

        /*
         * And then hide and wait for 1 second
         */
        DB2(bug("[SCSI++] 1 second delay, timer 0x%p...\n", timer));
        sigs = scsi_WaitTO(timer, 1, 0, SIGBREAKF_CTRL_C);

        DB2(bug("[SCSI++] Delay completed\n"));
        b++;
    } while (!sigs);

    D(bug("[SCSI++] Daemon quits\n"));

    scsi_CloseTimer(timer);

    Forbid();
    Signal(LIBBASE->daemonParent, SIGF_SINGLE);
}

/*
    Bus task body. It doesn't really do much. It receives simply all IORequests
    in endless loop and calls proper handling function. The IO is Semaphore-
    protected within a bus.
*/
void BusTaskCode(struct scsi_Bus *bus, LIBBASETYPEPTR LIBBASE)
{
    ULONG sig;
    int iter;
    struct IORequest *msg;
    OOP_Object *unitObj;
    struct scsi_Unit *unit;

    DINIT(bug("[SCSI**] Task started (bus: %u)\n", bus->sb_BusNum));

    bus->sb_Timer = scsi_OpenTimer(LIBBASE);
    bus->sb_BounceBufferPool = CreatePool(MEMF_CLEAR | MEMF_31BIT, 131072, 65536);

    /* Get the signal used for sleeping */
    bus->sb_Task = FindTask(0);
    bus->sb_SleepySignal = AllocSignal(-1);
    /* Failed to get it? Use SIGBREAKB_CTRL_E instead */
    if (bus->sb_SleepySignal < 0)
        bus->sb_SleepySignal = SIGBREAKB_CTRL_E;

    sig = 1L << bus->sb_MsgPort->mp_SigBit;

    for (iter = 0; iter < MAX_BUSUNITS; ++iter)
    {
        DINIT(bug("[SCSI**] Device %u type %d\n", iter, bus->sb_Dev[iter]));

        if (bus->sb_Dev[iter] > DEV_UNKNOWN)
        {
            unitObj = OOP_NewObject(LIBBASE->unitClass, NULL, NULL);
            if (unitObj)
            {
                unit = OOP_INST_DATA(LIBBASE->unitClass, unitObj);
                scsi_init_unit(bus, unit, iter);
                if (scsi_setup_unit(bus, unit))
                {
                    /*
                     * Add unit to the bus.
                     * At this point it becomes visible to OpenDevice().
                     */
                    bus->sb_Units[iter] = unitObj;

                    if (unit->su_XferModes & AF_XFER_PACKET)
                    {
                        scsi_RegisterVolume(0, 0, unit);

                        /* For ATAPI device we also submit media presence detection request */
                        unit->DaemonReq = (struct IOStdReq *)CreateIORequest(LIBBASE->DaemonPort, sizeof(struct IOStdReq));
                        if (unit->DaemonReq)
                        {
                            /*
                             * We don't want to keep stalled open count of 1, so we
                             * don't call OpenDevice() here. Instead we fill in the needed
                             * fields manually.
                             */
                            unit->DaemonReq->io_Device = &LIBBASE->scsi_Device;
                            unit->DaemonReq->io_Unit   = &unit->su_Unit;
                            unit->DaemonReq->io_Command = HD_SCSICMD+1;

                            ObtainSemaphore(&LIBBASE->DaemonSem);
                            AddTail((struct List *)&LIBBASE->Daemon_ios,
                                    &unit->DaemonReq->io_Message.mn_Node);
                            ReleaseSemaphore(&LIBBASE->DaemonSem);
                        }
                    }
                    else
                    {
                        scsi_RegisterVolume(0, unit->su_Cylinders - 1, unit);
                    }
                }
                else
                {
                    /* Destroy unit that couldn't be initialised */
                    OOP_DisposeObject((OOP_Object *)unit);
                    bus->sb_Dev[iter] = DEV_NONE;
                }
            }
        }
    }

    D(bug("[SCSI--] Bus %u scan finished\n", bus->sb_BusNum));
    ReleaseSemaphore(&LIBBASE->DetectionSem);

    /* Wait forever and process messages */
    for (;;)
    {
        Wait(sig);

        /* Even if you get new signal, do not process it until Unit is not active */
        if (!(bus->sb_Flags & UNITF_ACTIVE))
        {
            bus->sb_Flags |= UNITF_ACTIVE;

            /* Empty the request queue */
            while ((msg = (struct IORequest *)GetMsg(bus->sb_MsgPort)))
            {
                /* And do IO's */
                HandleIO(msg, LIBBASE);
                /* TD_ADDCHANGEINT doesn't require reply */
                if (msg->io_Command != TD_ADDCHANGEINT)
                {
                    ReplyMsg((struct Message *)msg);
                }
            }

            bus->sb_Flags &= ~(UNITF_INTASK | UNITF_ACTIVE);
        }
    }
}
