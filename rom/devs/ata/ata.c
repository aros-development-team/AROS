/*
    Copyright © 2004-2009, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/
/*
 * PARTIAL CHANGELOG:
 * DATE        NAME                ENTRY
 * ----------  ------------------  -------------------------------------------------------------------
 * 2008-01-25  T. Wiszkowski       Rebuilt, rearranged and partially fixed 60% of the code here
 *                                 Enabled implementation to scan for other PCI IDE controllers
 *                                 Implemented ATAPI Packet Support for both read and write
 *                                 Corrected ATAPI DMA handling
 *                                 Fixed major IDE enumeration bugs severely handicapping transfers with more than one controller
 *                                 Compacted source and implemented major ATA support procedure
 *                                 Improved DMA and Interrupt management
 *                                 Removed obsolete code
 * 2008-03-30  T. Wiszkowski       Added workaround for interrupt collision handling; fixed SATA in LEGACY mode.
 *                                 nForce and Intel SATA chipsets should now be operational.
 * 2008-04-03  T. Wiszkowski       Fixed IRQ flood issue, eliminated and reduced obsolete / redundant code
 * 2008-04-05  T. Wiszkowski       Improved IRQ management
 * 2008-04-07  T. Wiszkowski       Changed bus timeout mechanism
 * 2008-04-20  T. Wiszkowski       Corrected the flaw in drive identification routines leading to ocassional system hangups
 * 2008-05-11  T. Wiszkowski       Remade the ata trannsfers altogether, corrected the pio/irq handling
 *                                 medium removal, device detection, bus management and much more
 * 2009-03-05  T. Wiszkowski       remade timeouts, added timer-based and benchmark-based delays.
 * 2012-02-12  T. Wilen            Put "USUALLY YOU'D WANT TO CHECK IF DISC IS PRESENT FIRST" inside D()
 */

#include <aros/debug.h>

#include <exec/types.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <oop/oop.h>
#include "timer.h"

#include <dos/bptr.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include "ata.h"
#include LC_LIBDEFS_FILE

//---------------------------IO Commands---------------------------------------

/* Invalid comand does nothing, complains only. */
static void cmd_Invalid(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[ATA%02ld] cmd_Invalid: %d\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum, io->io_Command));
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
    struct ata_Unit *unit = (struct ata_Unit *)IOStdReq(io)->io_Unit;

	if (AF_Removable == (unit->au_Flags & (AF_Removable | AF_DiscPresent)))
	{
		D(bug("[ATA%02ld] cmd_Read32: USUALLY YOU'D WANT TO CHECK IF DISC IS PRESENT FIRST\n", unit->au_UnitNum));
		io->io_Error = TDERR_DiskChanged;
		return;
	}

    ULONG block = IOStdReq(io)->io_Offset;
    ULONG count = IOStdReq(io)->io_Length;

    D(bug("[ATA%02ld] cmd_Read32(%08x, %08x)\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum, block, count));

    ULONG mask = (1 << unit->au_SectorShift) - 1;

    /*
        During this IO call it should be sure that both offset and
        length are already aligned properly to sector boundaries.
    */
    if ((block & mask) | (count & mask))
    {
        D(bug("[ATA%02ld] cmd_Read32: offset or length not sector-aligned.\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));
        cmd_Invalid(io, LIBBASE);
    }
    else
    {
        block >>= unit->au_SectorShift;
        count >>= unit->au_SectorShift;
        ULONG cnt = 0;

        if ((0 == (unit->au_XferModes & AF_XFER_PACKET)) && ((block + count) > unit->au_Capacity))
        {
            bug("[ATA%02ld] cmd_Read32: Requested block (%lx;%ld) outside disk range (%lx)\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum, block, count, unit->au_Capacity);
            io->io_Error = IOERR_BADADDRESS;
            return;
        }

        /* Call the Unit's access funtion */
        io->io_Error = unit->au_Read32(unit, block, count,
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
    struct ata_Unit *unit = (struct ata_Unit *)IOStdReq(io)->io_Unit;

	if (AF_Removable == (unit->au_Flags & (AF_Removable | AF_DiscPresent)))
	{
		D(bug("[ATA%02ld] cmd_Read64: USUALLY YOU'D WANT TO CHECK IF DISC IS PRESENT FIRST\n", unit->au_UnitNum));
		io->io_Error = TDERR_DiskChanged;
		return;
	}

    UQUAD block = IOStdReq(io)->io_Offset | (UQUAD)(IOStdReq(io)->io_Actual) << 32;
    ULONG count = IOStdReq(io)->io_Length;

    D(bug("[ATA%02ld] cmd_Read64(%08x-%08x, %08x)\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum, IOStdReq(io)->io_Actual, IOStdReq(io)->io_Offset, count));

    ULONG mask = (1 << unit->au_SectorShift) - 1;

    if ((block & (UQUAD)mask) | (count & mask) | (count == 0))
    {
        D(bug("[ATA%02ld] cmd_Read64: offset or length not sector-aligned.\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));
        cmd_Invalid(io, LIBBASE);
    }
    else
    {
        block >>= unit->au_SectorShift;
        count >>= unit->au_SectorShift;
        ULONG cnt = 0;

        /*
            If the sum of sector offset and the sector count doesn't overflow
            the 28-bit LBA address, use 32-bit access for speed and simplicity.
            Otherwise do the 48-bit LBA addressing.
        */
        if ((block + count) < 0x0fffffff)
        {
            if ((0 == (unit->au_XferModes & AF_XFER_PACKET)) && ((block + count) > unit->au_Capacity))
            {
                bug("[ATA%02ld] cmd_Read64: Requested block (%lx;%ld) outside disk range (%lx)\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum, block, count, unit->au_Capacity);
                io->io_Error = IOERR_BADADDRESS;
                return;
            }
            io->io_Error = unit->au_Read32(unit, (ULONG)(block & 0x0fffffff), count, IOStdReq(io)->io_Data, &cnt);
        }
        else
        {
            if ((0 == (unit->au_XferModes & AF_XFER_PACKET)) && ((block + count) > unit->au_Capacity48))
            {
                bug("[ATA%02ld] cmd_Read64: Requested block (%lx:%08lx;%ld) outside disk range (%lx:%08lx)\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum, block>>32, block&0xfffffffful, count, unit->au_Capacity48>>32, unit->au_Capacity48 & 0xfffffffful);
                io->io_Error = IOERR_BADADDRESS;
                return;
            }

            io->io_Error = unit->au_Read64(unit, block, count, IOStdReq(io)->io_Data, &cnt);
        }

        IOStdReq(io)->io_Actual = cnt;
    }
}

/* CMD_WRITE implementation */
static void cmd_Write32(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)IOStdReq(io)->io_Unit;

	if (AF_Removable == (unit->au_Flags & (AF_Removable | AF_DiscPresent)))
	{
		D(bug("[ATA%02ld] cmd_Write32: USUALLY YOU'D WANT TO CHECK IF DISC IS PRESENT FIRST\n", unit->au_UnitNum));
		io->io_Error = TDERR_DiskChanged;
		return;
	}

    ULONG block = IOStdReq(io)->io_Offset;
    ULONG count = IOStdReq(io)->io_Length;

    D(bug("[ATA%02ld] cmd_Write32(%08x, %08x)\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum, block, count));

    ULONG mask = (1 << unit->au_SectorShift) - 1;

    /*
        During this IO call it should be sure that both offset and
        length are already aligned properly to sector boundaries.
    */
    if ((block & mask) | (count & mask))
    {
        D(bug("[ATA%02ld] cmd_Write32: offset or length not sector-aligned.\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));
        cmd_Invalid(io, LIBBASE);
    }
    else
    {
        block >>= unit->au_SectorShift;
        count >>= unit->au_SectorShift;
        ULONG cnt = 0;

        if ((0 == (unit->au_XferModes & AF_XFER_PACKET))
            && ((block + count) > unit->au_Capacity))
        {
            bug("[ATA%02ld] cmd_Write32: Requested block (%lx;%ld) outside disk range (%lx)\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum,
                block, count, unit->au_Capacity);
            io->io_Error = IOERR_BADADDRESS;
            return;
        }

        /* Call the Unit's access funtion */
        io->io_Error = unit->au_Write32(unit, block, count,
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
    struct ata_Unit *unit = (struct ata_Unit *)IOStdReq(io)->io_Unit;

	if (AF_Removable == (unit->au_Flags & (AF_Removable | AF_DiscPresent)))
	{
		D(bug("[ATA%02ld] cmd_Write64: USUALLY YOU'D WANT TO CHECK IF DISC IS PRESENT FIRST\n", unit->au_UnitNum));
		io->io_Error = TDERR_DiskChanged;
		return;
	}


    UQUAD block = IOStdReq(io)->io_Offset | (UQUAD)(IOStdReq(io)->io_Actual) << 32;
    ULONG count = IOStdReq(io)->io_Length;

    D(bug("[ATA%02ld] cmd_Write64(%08x-%08x, %08x)\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum, IOStdReq(io)->io_Actual, IOStdReq(io)->io_Offset, count));

    ULONG mask = (1 << unit->au_SectorShift) - 1;

    if ((block & mask) | (count & mask) | (count==0))
    {
        D(bug("[ATA%02ld] cmd_Write64: offset or length not sector-aligned.\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));
        cmd_Invalid(io, LIBBASE);
    }
    else
    {
        block >>= unit->au_SectorShift;
        count >>= unit->au_SectorShift;
        ULONG cnt = 0;

        /*
            If the sum of sector offset and the sector count doesn't overflow
            the 28-bit LBA address, use 32-bit access for speed and simplicity.
            Otherwise do the 48-bit LBA addressing.
        */
        if ((block + count) < 0x0fffffff)
        {
            if ((0 == (unit->au_XferModes & AF_XFER_PACKET))
                && ((block + count) > unit->au_Capacity))
            {
                bug("[ATA%02ld] cmd_Write64: Requested block (%lx;%ld) outside disk range "
                    "(%lx)\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum, block, count, unit->au_Capacity);
                io->io_Error = IOERR_BADADDRESS;
                return;
            }
            io->io_Error = unit->au_Write32(unit, (ULONG)(block & 0x0fffffff),
                count, IOStdReq(io)->io_Data, &cnt);
        }
        else
        {
            if ((0 == (unit->au_XferModes & AF_XFER_PACKET))
                && ((block + count) > unit->au_Capacity48))
            {
                bug("[ATA%02ld] cmd_Write64: Requested block (%lx:%08lx;%ld) outside disk "
                    "range (%lx:%08lx)\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum,
                     block>>32, block&0xfffffffful,
                     count, unit->au_Capacity48>>32,
                     unit->au_Capacity48 & 0xfffffffful);
                io->io_Error = IOERR_BADADDRESS;
                return;
            }

            io->io_Error = unit->au_Write64(unit, block, count,
                IOStdReq(io)->io_Data, &cnt);
        }
        IOStdReq(io)->io_Actual = cnt;
   }
}


/* use CMD_FLUSH to force all IO waiting commands to abort */
static void cmd_Flush(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct IORequest *msg;
    struct ata_Bus *bus = ((struct ata_Unit *)io->io_Unit)->au_Bus;

    D(bug("[ATA%02ld] cmd_Flush()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));

    Forbid();

    while((msg = (struct IORequest *)GetMsg((struct MsgPort *)bus->ab_MsgPort)))
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
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;
    struct IORequest *msg;

    D(bug("[ATA%02ld] cmd_TestChanged()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));

    if ((unit->au_XferModes & AF_XFER_PACKET) && (unit->au_Flags & AF_Removable))
    {
        atapi_TestUnitOK(unit);
        if (unit->au_Flags & AF_DiscChanged)
        {
            unit->au_ChangeNum++;

            Forbid();

            /* old-fashioned RemoveInt call first */
            if (unit->au_RemoveInt)
                Cause(unit->au_RemoveInt);

            /* And now the whole list of possible calls */
            ForeachNode(&unit->au_SoftList, msg)
            {
                Cause((struct Interrupt *)IOStdReq(msg)->io_Data);
            }

            unit->au_Flags &= ~AF_DiscChanged;

            Permit();
        }
    }
}

static void cmd_Update(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    /* Do nothing now. In near future there should be drive cache flush though */
    D(bug("[ATA%02ld] cmd_Update()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));
}

static void cmd_Remove(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    D(bug("[ATA%02ld] cmd_Remove()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));

    if (unit->au_RemoveInt)
        io->io_Error = TDERR_DriveInUse;
    else
        unit->au_RemoveInt = IOStdReq(io)->io_Data;
}

static void cmd_ChangeNum(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[ATA%02ld] cmd_ChangeNum()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));

    IOStdReq(io)->io_Actual = ((struct ata_Unit *)io->io_Unit)->au_ChangeNum;
}

static void cmd_ChangeState(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    D(bug("[ATA%02ld] cmd_ChangeState()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));

    if (unit->au_Flags & AF_DiscPresent)
        IOStdReq(io)->io_Actual = 0;
    else
        IOStdReq(io)->io_Actual = 1;

    D(bug("[ATA%02ld] cmd_ChangeState: Media %s\n", unit->au_UnitNum, IOStdReq(io)->io_Actual ? "ABSENT" : "PRESENT"));
}

static void cmd_ProtStatus(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    D(bug("[ATA%02ld] cmd_ProtStatus()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));

    if (unit->au_DevType)
        IOStdReq(io)->io_Actual = -1;
    else
        IOStdReq(io)->io_Actual = 0;

}

static void cmd_GetNumTracks(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[ATA%02ld] cmd_GetNumTracks()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));

    IOStdReq(io)->io_Actual = ((struct ata_Unit *)io->io_Unit)->au_Cylinders;
}

static void cmd_AddChangeInt(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    D(bug("[ATA%02ld] cmd_AddChangeInt()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));

    Forbid();
    AddHead(&unit->au_SoftList, (struct Node *)io);
    Permit();

    io->io_Flags &= ~IOF_QUICK;
    unit->au_Unit.unit_flags &= ~UNITF_ACTIVE;
}

static void cmd_RemChangeInt(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[ATA%02ld] cmd_RemChangeInt()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));

    Forbid();
    Remove((struct Node *)io);
    Permit();
}

static void cmd_Eject(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    D(bug("[ATA%02ld] cmd_Eject()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));

    IOStdReq(io)->io_Error = unit->au_Eject(unit);
    cmd_TestChanged(io, LIBBASE);
}

static void cmd_GetGeometry(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    D(bug("[ATA%02ld] cmd_GetGeometry()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));

    if (IOStdReq(io)->io_Length == sizeof(struct DriveGeometry))
    {
        struct DriveGeometry *dg = (struct DriveGeometry *)IOStdReq(io)->io_Data;

        dg->dg_SectorSize       = 1 << unit->au_SectorShift;

        if (unit->au_Capacity48 != 0)
        {
            if ((unit->au_Capacity48 >> 32) != 0)
                dg->dg_TotalSectors     = 0xffffffff;
            else
                dg->dg_TotalSectors     = unit->au_Capacity48;
        }
        else
            dg->dg_TotalSectors         = unit->au_Capacity;

        dg->dg_Cylinders                = unit->au_Cylinders;
        dg->dg_CylSectors               = unit->au_Sectors * unit->au_Heads;
        dg->dg_Heads                    = unit->au_Heads;
        dg->dg_TrackSectors             = unit->au_Sectors;
        dg->dg_BufMemType               = MEMF_PUBLIC;
        dg->dg_DeviceType               = unit->au_DevType;
        if (dg->dg_DeviceType != DG_DIRECT_ACCESS)
            dg->dg_Flags                    = (unit->au_Flags & AF_Removable) ? DGF_REMOVABLE : 0;
        else
            dg->dg_Flags                = 0;
        dg->dg_Reserved                 = 0;

        IOStdReq(io)->io_Actual = sizeof(struct DriveGeometry);
    }
    else if (IOStdReq(io)->io_Length == 514)
    {
        CopyMemQuick(unit->au_Drive, IOStdReq(io)->io_Data, 512);
    }
    else io->io_Error = TDERR_NotSpecified;
}

static void cmd_DirectScsi(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    D(bug("[ATA%02ld] cmd_DirectScsi()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));

    IOStdReq(io)->io_Actual = sizeof(struct SCSICmd);
    if (unit->au_XferModes & AF_XFER_PACKET)
    {
        io->io_Error = unit->au_DirectSCSI(unit, (struct SCSICmd *)IOStdReq(io)->io_Data);
    }
    else if (unit->au_DevType == DG_DIRECT_ACCESS)
    {
    	io->io_Error = SCSIEmu(unit, (struct SCSICmd *)IOStdReq(io)->io_Data);
    }
    else io->io_Error = IOERR_BADADDRESS;
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
    Try to do IO commands. All commands which require talking with ata devices
    will be handled slow, that is they will be passed to bus task which will
    execute them as soon as hardware will be free.
*/
AROS_LH1(void, BeginIO,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 5, ata)
{
    AROS_LIBFUNC_INIT

    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    io->io_Message.mn_Node.ln_Type = NT_MESSAGE;

    /* Disable interrupts for a while to modify message flags */
    Disable();

    D(bug("[ATA%02ld] BeginIO: Executing IO Command %lx\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum, io->io_Command));

    /*
        If the command is not-immediate, or presence of disc is still unknown,
        let the bus task do the job.
    */
    if (isSlow(io->io_Command))
    {
        unit->au_Unit.unit_flags |= UNITF_ACTIVE | UNITF_INTASK;
        io->io_Flags &= ~IOF_QUICK;
        Enable();

        /* Put the message to the bus */
        PutMsg(unit->au_Bus->ab_MsgPort, (struct Message *)io);
    }
    else
    {
    	D(bug("[ATA%02ld] Fast command\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));
    
        /* Immediate command. Mark unit as active and do the command directly */
        unit->au_Unit.unit_flags |= UNITF_ACTIVE;
        Enable();
        HandleIO(io, LIBBASE);

        unit->au_Unit.unit_flags &= ~UNITF_ACTIVE;

        /*
            If the command was not intended to be immediate and it was not the
            TD_ADDCHANGEINT, reply to confirm command execution now.
        */
        if (!(io->io_Flags & IOF_QUICK) && (io->io_Command != TD_ADDCHANGEINT))
        {
            ReplyMsg((struct Message *)io);
        }
    }

    D(bug("[ATA%02ld] BeginIO: Done\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum));
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 6, ata)
{
    AROS_LIBFUNC_INIT

    /* Cannot Abort IO */
    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, GetRdskLba,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 7, ata)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, GetBlkSize,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 8, ata)
{
    AROS_LIBFUNC_INIT

    return Unit(io)->au_SectorShift;

    AROS_LIBFUNC_EXIT
}

/*
 * The daemon of ata.device first opens all ATAPI devices and then enters
 * endless loop. Every 2 seconds it tells ATAPI units to check the media
 * presence. In case of any state change they will rise user-specified
 * functions.
 * The check is done by sending HD_SCSICMD+1 command (internal testchanged
 * command). ATAPI units should already handle the command further.
 */
void DaemonCode(LIBBASETYPEPTR LIBBASE)
{
    struct MsgPort *myport;     // Message port used with ata.device
    struct IORequest *timer;	// timer
    struct IOStdReq *ios[64];   // Placeholder for unit messages
    int count = 0,b,d;
    struct ata_Bus *bus;

    D(bug("[ATA**] You woke up DAEMON\n"));

    /*
     * Prepare message ports and timer.device's request
     */
    myport = CreateMsgPort();
    timer  = ata_OpenTimer(LIBBASE);
    bus         = (struct ata_Bus*)LIBBASE->ata_Buses.mlh_Head;

    /*
     * grab all buses, see if there is an atapi cdrom connected or anything alike
     * TODO: good thing to consider putting extra code here for future hotplug support *if any*
     */
    while (bus->ab_Node.mln_Succ != NULL)
    {
       b = bus->ab_BusNum;
       D(bug("[ATA++] Checking bus %d\n", b));

       for (d=0; d < MAX_BUSUNITS; d++)
       {
          /*
           * Is a device ATAPI?
           */
          D(bug("[ATA++] Unit %d is of type %x\n", (b<<1)+d, bus->ab_Dev[d]));

          if (bus->ab_Dev[d] >= DEV_ATAPI)
          {
             /*
              * Atapi device found. Create IOStdReq for it
              */
             ios[count] = (struct IOStdReq *) CreateIORequest(myport, sizeof(struct IOStdReq));

             ios[count]->io_Command = HD_SCSICMD + 1;

             /*
              * And force OpenDevice call. Don't use direct call as it's unsafe
              * and not nice at all.
              *
              * so THIS is an OpenDevice().....
              */
             D(bug("[ATA++] Opening ATAPI device, unit %d\n", (b<<1)|d));
             AROS_LVO_CALL3NR(void,
                   AROS_LCA(struct IORequest *, (struct IORequest *)(ios[count]), A1),
                   AROS_LCA(ULONG, (b << 1) | d, D0),
                   AROS_LCA(ULONG, 0, D1),
                   LIBBASETYPEPTR, LIBBASE, 1, ata);

             /*
              * increase amount of ATAPI devices in system
              */
             count++;
          }

          /*
           * INFO: we are only handling up to 64 atapi devices here
           */
          if (count == sizeof(ios) / sizeof(*ios))
             break;
       }
       bus = (struct ata_Bus*)bus->ab_Node.mln_Succ;
    }

    D(bug("[ATA++] Starting sweep medium presence detection\n"));

    /*
     * Endless loop
     */
    for (b=0;;++b)
    {
        /*
         * call separate IORequest for every ATAPI device
         * we're calling HD_SCSICMD+1 command here -- anything like test unit ready?
         */
        if (0 == (b & 1))
        {
            D(bug("[ATA++] Detecting media presence\n"));
            for (d=0; d < count; d++)
                DoIO((struct IORequest *)ios[d]);
        }

        /*
         * check / trigger all buses waiting for an irq
         */
        DB2(bug("[ATA++] Checking timeouts...\n"));
        Forbid();
        ForeachNode(&LIBBASE->ata_Buses, bus)
        {
            if (bus->ab_Timeout >= 0)
            {
                if (0 > (--bus->ab_Timeout))
                {
                    Signal(bus->ab_Task, SIGBREAKF_CTRL_C);
                }
            }
        }
        Permit();

        /*
         * And then hide and wait for 1 second
         */
        DB2(bug("[ATA++] 1 second delay, timer 0x%p...\n", timer));
        ata_WaitTO(timer, 1, 0, 0);

        DB2(bug("[ATA++] Delay completed\n"));
    }
}

/*
    Bus task body. It doesn't really do much. It receives simply all IORequests
    in endless loop and calls proper handling function. The IO is Semaphore-
    protected within a bus.
*/
void BusTaskCode(struct ata_Bus *bus, struct Task* parent, struct SignalSemaphore *ssem)
{
    ULONG sig;
    int iter;
    struct IORequest *msg;
    struct ata_Unit *unit;

    D(bug("[ATA**] Task started (IO: 0x%x)\n", bus->ab_Port));

    /*
     * don't hold parent while we analyze devices.
     * keep things as parallel as they can be
     */
    ObtainSemaphoreShared(ssem);
    Signal(parent, SIGBREAKF_CTRL_C);

    bus->ab_Timer = ata_OpenTimer(bus->ab_Base);

    /* Get the signal used for sleeping */
    bus->ab_Task = FindTask(0);
    bus->ab_SleepySignal = AllocSignal(-1);
    /* Failed to get it? Use SIGBREAKB_CTRL_E instead */
    if (bus->ab_SleepySignal < 0)
        bus->ab_SleepySignal = SIGBREAKB_CTRL_E;

    /*
     * set up irq handler now. all irqs are disabled, so prepare them one by one
     */
    bus->ab_IntHandler = bus->ab_Driver->CreateInterrupt(bus);
    if (!bus->ab_IntHandler)
    {
        D(bug("[ATA  ] Something wrong with creating interrupt?\n"));
    }

    sig = 1L << bus->ab_MsgPort->mp_SigBit;

    for (iter=0; iter<MAX_BUSUNITS; ++iter)
    {
       unit = bus->ab_Units[iter];
       if (ata_setup_unit(bus, iter))
       {
          if (unit->au_XferModes & AF_XFER_PACKET)
             ata_RegisterVolume(0, 0, unit);
          else
             ata_RegisterVolume(0, unit->au_Cylinders - 1, unit);
       }
       else
       {
           /* Destroy unit that couldn't be initialised */
           FreeVecPooled(bus->ab_Base->ata_MemPool, unit);
           bus->ab_Units[iter] = NULL;
           bus->ab_Dev[iter] = DEV_NONE;
       }
    }

    /*
     * ok, we're done with init here.
     * tell the parent task we're ready
     */
    ReleaseSemaphore(ssem);

    /* Wait forever and process messages */
    for (;;)
    {
        Wait(sig);

        /* Even if you get new signal, do not process it until Unit is not active */
        if (!(bus->ab_Flags & UNITF_ACTIVE))
        {
            bus->ab_Flags |= UNITF_ACTIVE;

            /* Empty the request queue */
            while ((msg = (struct IORequest *)GetMsg(bus->ab_MsgPort)))
            {
                /* And do IO's */
                HandleIO(msg, bus->ab_Base);
                /* TD_ADDCHANGEINT doesn't require reply */
                if (msg->io_Command != TD_ADDCHANGEINT)
                {
                    ReplyMsg((struct Message *)msg);
                }
            }

            bus->ab_Flags &= ~(UNITF_INTASK | UNITF_ACTIVE);
        }
    }
}

/* vim: set ts=4 sw=4 :*/
