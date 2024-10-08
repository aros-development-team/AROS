/*
    Copyright (C) 2004-2023, The AROS Development Team. All rights reserved

    Desc:
*/

/* Maintainer: Jason S. McMullan <jason.mcmullan@gmail.com>
 */
 
 #include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>

#include <aros/atomic.h>
#include <aros/symbolsets.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <utility/utility.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <hidd/pci.h>

#include <string.h>

#include "ahci.h"
#include "ahci_scsi.h"
#include "timer.h"

#include LC_LIBDEFS_FILE

//#define DEBUG_AHCI_UNIT 1
//#define DEBUG_AHCI_IOR

/* */

#if defined(DEBUG_AHCI_UNIT)
#define AHCI_UNIT_DBG if (unit->sim_Unit == DEBUG_AHCI_UNIT)
#else
#define AHCI_UNIT_DBG
#endif

#if defined(DEBUG_AHCI_IOR)
#define AHCI_IO_FMT     "0x%p"
#define AHCI_IO_STR     " IO " AHCI_IO_FMT
#define AHCI_IO_DBG     ,io
#else
#define AHCI_IO_FMT
#define AHCI_IO_STR
#define AHCI_IO_DBG
#endif

/* Translate to a SCSI command, since the
 * backend will make that work on both ATAPI
 * and ATA devices.
 *
 * Return whether or not we are completed.
 */
static BOOL ahci_sector_rw(struct IORequest *io, UQUAD off64, BOOL is_write)
{
    struct IOExtTD *iotd = (struct IOExtTD *)io;
    struct cam_sim *unit = (struct cam_sim *)io->io_Unit;
    struct ahci_port *ap = unit->sim_Port;
    struct ata_port  *at = ap->ap_ata[0];
    struct AHCIBase *AHCIBase = ap->ap_sc->sc_dev->dev_Base;
    APTR data = iotd->iotd_Req.io_Data;
    ULONG len = iotd->iotd_Req.io_Length;
    ULONG sector_size, bmask;
    BOOL done = TRUE;
    /* It's safe to allocate these on the stack, since they
     * will never be referenced once the ahci_scsi_*_io()
     * routine returns.
     */
    struct SCSICmd scsi = {};
    union scsi_cdb cdb = {};

    ahciDebug("[AHCI%02ld] %s()", unit->sim_Unit, __func__);

    if (ap->ap_type == ATA_PORT_T_DISK) {
        sector_size = at->at_identify.sector_size;
    } else if (ap->ap_type == ATA_PORT_T_ATAPI) {
        // FIXME: Where should this come from?
        sector_size = 2048;
    } else {
        io->io_Error = TDERR_DiskChanged;
        return TRUE;
    }
    bmask = sector_size - 1;

    if ((off64 & bmask) || bmask == 0 || data == NULL) {
        io->io_Error = IOERR_BADADDRESS;
        return TRUE;
    }
    if ((len & bmask) || len == 0) {
        ahciDebug("[AHCI%02ld]" AHCI_IO_STR " Fault, io_Flags = %d, io_Command = %d, IOERR_BADLENGTH (len=0x%x, bmask=0x%x)", unit->sim_Unit AHCI_IO_DBG, io->io_Flags, io->io_Command, len, bmask);
        io->io_Error = IOERR_BADLENGTH;
        return TRUE;
    }

    if (len == 0) {
        IOStdReq(io)->io_Actual = 0;
        return TRUE;
    }

    scsi.scsi_Data = data;
    scsi.scsi_Length = len;
    scsi.scsi_Flags  = is_write ? SCSIF_WRITE : SCSIF_READ;

    /* Make in units of sector size */
    len /= sector_size;
    off64 /= sector_size;

    /* Set up the CDB, based on what the transfer size is */
    scsi.scsi_Command = (APTR)&cdb;

    if (off64 < (1 << 21) && len < (1 << 8)) {
        cdb.rw_6.opcode = is_write ? SCSI_DA_WRITE_6 : SCSI_DA_READ_6;
        cdb.rw_6.addr[0] = (off64 >> 16) & 0x1f;
        cdb.rw_6.addr[1] = (off64 >>  8) & 0xff;
        cdb.rw_6.addr[2] = (off64 >>  0) & 0xff;
        cdb.rw_6.length  = len;
        scsi.scsi_CmdLength = sizeof(cdb.rw_6);
    } else if (off64 < (1ULL << 32) && len < (1 << 16)) {
        cdb.rw_10.opcode = is_write ? SCSI_DA_WRITE_10 : SCSI_DA_READ_10;
        cdb.rw_10.addr[0] = (off64 >> 24) & 0xff;
        cdb.rw_10.addr[1] = (off64 >> 16) & 0xff;
        cdb.rw_10.addr[2] = (off64 >>  8) & 0xff;
        cdb.rw_10.addr[3] = (off64 >>  0) & 0xff;
        cdb.rw_10.length[0] = (len >> 8) & 0xff;
        cdb.rw_10.length[1] = (len >> 0) & 0xff;
        scsi.scsi_CmdLength = sizeof(cdb.rw_10);
    } else if (off64 < (1ULL << 32)) {
        cdb.rw_12.opcode = is_write ? SCSI_DA_WRITE_12 : SCSI_DA_READ_12;
        cdb.rw_12.addr[0] = (off64 >> 24) & 0xff;
        cdb.rw_12.addr[1] = (off64 >> 16) & 0xff;
        cdb.rw_12.addr[2] = (off64 >>  8) & 0xff;
        cdb.rw_12.addr[3] = (off64 >>  0) & 0xff;
        cdb.rw_12.length[0] = (len >> 24) & 0xff;
        cdb.rw_12.length[1] = (len >> 16) & 0xff;
        cdb.rw_12.length[2] = (len >>  8) & 0xff;
        cdb.rw_12.length[3] = (len >>  0) & 0xff;
        scsi.scsi_CmdLength = sizeof(cdb.rw_12);
    } else {
        cdb.rw_16.opcode = is_write ? SCSI_DA_WRITE_16 : SCSI_DA_READ_16;
        cdb.rw_16.addr[0] = (off64 >> 56) & 0xff;
        cdb.rw_16.addr[1] = (off64 >> 48) & 0xff;
        cdb.rw_16.addr[2] = (off64 >> 40) & 0xff;
        cdb.rw_16.addr[3] = (off64 >> 32) & 0xff;
        cdb.rw_16.addr[4] = (off64 >> 24) & 0xff;
        cdb.rw_16.addr[5] = (off64 >> 16) & 0xff;
        cdb.rw_16.addr[6] = (off64 >>  8) & 0xff;
        cdb.rw_16.addr[7] = (off64 >>  0) & 0xff;
        cdb.rw_16.length[0] = (len >> 24) & 0xff;
        cdb.rw_16.length[1] = (len >> 16) & 0xff;
        cdb.rw_16.length[2] = (len >>  8) & 0xff;
        cdb.rw_16.length[3] = (len >>  0) & 0xff;
        scsi.scsi_CmdLength = sizeof(cdb.rw_16);
    }

    if (ap->ap_type == ATA_PORT_T_DISK)
        done = ahci_scsi_disk_io(io, &scsi);
    else if (ap->ap_type == ATA_PORT_T_ATAPI)
        done = ahci_scsi_atapi_io(io, &scsi);
    else
        io->io_Error = IOERR_NOCMD;

    return done;
}

/*
    Try to do IO commands. All commands which require talking with ahci devices
    will be handled slow, that is they will be passed to bus task which will
    execute them as soon as hardware will be free.
*/
AROS_LH1(void, BeginIO,
    AROS_LHA(struct IORequest *, io, A1),
    struct AHCIBase *, AHCIBase, 5, ahci)
{
    AROS_LIBFUNC_INIT

    const UWORD NSDSupported[] = {
        CMD_READ,
        CMD_WRITE,
        CMD_UPDATE,
        CMD_CLEAR,
        TD_ADDCHANGEINT,
        TD_CHANGENUM,
        TD_CHANGESTATE,
        TD_EJECT,
        TD_FORMAT,
        TD_FORMAT64,
        TD_GETGEOMETRY,
        TD_MOTOR,
        TD_PROTSTATUS,
        TD_READ64,
        TD_REMCHANGEINT,
        TD_SEEK,
        TD_SEEK64,
        TD_WRITE64,
        NSCMD_DEVICEQUERY,
        NSCMD_TD_FORMAT64,
        NSCMD_TD_READ64,
        NSCMD_TD_SEEK64,
        NSCMD_TD_WRITE64,
        0
    };
    struct IOExtTD *iotd = (struct IOExtTD *)io;
    struct cam_sim *unit = (struct cam_sim *)io->io_Unit;
    struct ahci_port *ap = unit->sim_Port;
    struct ata_port  *at = ap->ap_ata[0];
    APTR data = iotd->iotd_Req.io_Data;
    ULONG len = iotd->iotd_Req.io_Length;
    UQUAD off64;
    struct DriveGeometry *geom;
    struct NSDeviceQueryResult *nsqr;
    BOOL done = (io->io_Flags & IOF_QUICK) ? TRUE : FALSE;

    ahciDebug("[AHCI%02ld] %s(" AHCI_IO_FMT ")\n", unit->sim_Unit, __func__ AHCI_IO_DBG);

    io->io_Message.mn_Node.ln_Type = NT_MESSAGE;
    io->io_Error = 0;

    ahciDebug("[AHCI%02ld]" AHCI_IO_STR " Start, io_Flags = %d, io_Command = %d\n", unit->sim_Unit AHCI_IO_DBG, io->io_Flags, io->io_Command);

    /* Unit going offline? Don't permit new commands. */
    if (unit->sim_Flags & SIMF_OffLine) {
        io->io_Error = IOERR_OPENFAIL;
        if (!(io->io_Flags & IOF_QUICK))
            ReplyMsg(&io->io_Message);
        ahciDebug("[AHCI%02ld] %s: WARN - unit is offline\n", unit->sim_Unit, __func__);
        return;
    }

    ObtainSemaphore(&unit->sim_Lock);
    AddHead(&unit->sim_IOs, &io->io_Message.mn_Node);
    ReleaseSemaphore(&unit->sim_Lock);

    switch (io->io_Command) {
    case NSCMD_DEVICEQUERY:
        ahciDebug("[AHCI%02ld]" AHCI_IO_STR " NSCMD_DEVICEQUERY\n", unit->sim_Unit AHCI_IO_DBG);
        if (len < sizeof(*nsqr))
            goto bad_length;

        nsqr = data;
        nsqr->DevQueryFormat = 0;
        nsqr->SizeAvailable  = sizeof(struct NSDeviceQueryResult);
        nsqr->DeviceType     = NSDEVTYPE_TRACKDISK;
        nsqr->DeviceSubType  = 0;
        nsqr->SupportedCommands = (UWORD *)NSDSupported;
        IOStdReq(io)->io_Actual = sizeof(*nsqr);
        done = TRUE;
        break;
    case TD_PROTSTATUS:
        ahciDebug("[AHCI%02ld]" AHCI_IO_STR " TD_PROTSTATUS\n", unit->sim_Unit AHCI_IO_DBG);
        IOStdReq(io)->io_Actual = (ap->ap_type == ATA_PORT_T_DISK) ? 0 : -1;
        done = TRUE;
        break;
    case TD_CHANGENUM:
        ahciDebug("[AHCI%02ld]" AHCI_IO_STR " TD_CHANGENUM\n", unit->sim_Unit AHCI_IO_DBG);
        IOStdReq(io)->io_Actual = unit->sim_ChangeNum;
        done = TRUE;
        break;
    case TD_CHANGESTATE:
        ahciDebug("[AHCI%02ld]" AHCI_IO_STR " TD_CHANGESTATE\n", unit->sim_Unit AHCI_IO_DBG);
        IOStdReq(io)->io_Actual = (unit->sim_Flags & SIMF_MediaPresent) ? 0 : -1;
        done = TRUE;
        break;
    case TD_EJECT:
    {
        ahciDebug("[AHCI%02ld]" AHCI_IO_STR " TD_EJECT\n", unit->sim_Unit AHCI_IO_DBG);
        if (at->at_identify.config & (1 << 7))
        {
            // FIXME: Eject removable media
        }
        done = TRUE;
        break;
    }
    case TD_GETDRIVETYPE:
        ahciDebug("[AHCI%02ld]" AHCI_IO_STR " TD_GETDRIVETYPE\n", unit->sim_Unit AHCI_IO_DBG);
        IOStdReq(io)->io_Actual = DRIVE_NEWSTYLE;
        done = TRUE;
        break;
    case TD_GETGEOMETRY:
        ahciDebug("[AHCI%02ld]" AHCI_IO_STR " TD_GETGEOMETRY\n", unit->sim_Unit AHCI_IO_DBG);
        if (len < sizeof(*geom))
            goto bad_length;

        if (ap->ap_type != ATA_PORT_T_DISK && ap->ap_type != ATA_PORT_T_ATAPI)
            goto bad_cmd;

        geom = data;
        SetMem(geom, 0, len);
        if (ap->ap_type == ATA_PORT_T_DISK) {
            geom->dg_SectorSize   = at->at_identify.sector_size;
            geom->dg_Cylinders    = at->at_ncyls;
            geom->dg_CylSectors   = at->at_identify.nsectors * at->at_identify.nheads;
            if (((UQUAD)geom->dg_Cylinders * geom->dg_CylSectors) >> 32 != 0)
                geom->dg_TotalSectors = 0xffffffff;
            else
                geom->dg_TotalSectors = geom->dg_Cylinders * geom->dg_CylSectors;
            geom->dg_Heads        = at->at_identify.nheads;
            geom->dg_TrackSectors = at->at_identify.nsectors;
        }
        geom->dg_BufMemType   = MEMF_PUBLIC;
        geom->dg_DeviceType   = (ap->ap_type == ATA_PORT_T_ATAPI) ? DG_CDROM : DG_DIRECT_ACCESS;
        geom->dg_Flags        = (at->at_identify.config & (1 << 7)) ? DGF_REMOVABLE : 0;
        IOStdReq(io)->io_Actual = sizeof(*geom);
        done = TRUE;
        break;

    case TD_FORMAT:
        if (len > (at->at_identify.nsectors * at->at_identify.sector_size - 1))
            goto bad_length;
        off64  = iotd->iotd_Req.io_Offset;
        if (off64 > (at->at_identify.nsectors * at->at_identify.sector_size - 1))
            goto bad_address;
        done = ahci_sector_rw(io, off64, TRUE);
        break;

    case TD_FORMAT64:
    case NSCMD_TD_FORMAT64:
        if (len > (at->at_identify.nsectors * at->at_identify.sector_size - 1))
            goto bad_length;
        off64  = iotd->iotd_Req.io_Offset;
        off64 |= ((UQUAD)iotd->iotd_Req.io_Actual)<<32;
        if (off64 > (at->at_identify.nsectors * at->at_identify.sector_size - 1))
            goto bad_address;
        done = ahci_sector_rw(io, off64, TRUE);
        break;

    case TD_SEEK:
        IOStdReq(io)->io_Actual = 0;
        done = TRUE;
        break;

    case TD_SEEK64:
    case NSCMD_TD_SEEK64:
        IOStdReq(io)->io_Actual = 0;
        done = TRUE;
        break;

    case TD_MOTOR:
        ahciDebug("[AHCI%02ld]" AHCI_IO_STR " TD_MOTOR\n", unit->sim_Unit AHCI_IO_DBG);
        // FIXME: Tie in with power management
        IOStdReq(io)->io_Actual = 1;
        done = TRUE;
        break;

    case CMD_WRITE:
        off64  = iotd->iotd_Req.io_Offset;
        done = ahci_sector_rw(io, off64, TRUE);
        break;

    case TD_WRITE64:
    case NSCMD_TD_WRITE64:
        off64  = iotd->iotd_Req.io_Offset;
        off64 |= ((UQUAD)iotd->iotd_Req.io_Actual)<<32;
        done = ahci_sector_rw(io, off64, TRUE);
        break;

    case CMD_READ:
        off64  = iotd->iotd_Req.io_Offset;
        done = ahci_sector_rw(io, off64, FALSE);
        break;

    case TD_READ64:
    case NSCMD_TD_READ64:
        off64  = iotd->iotd_Req.io_Offset;
        off64 |= ((UQUAD)iotd->iotd_Req.io_Actual)<<32;
        done = ahci_sector_rw(io, off64, FALSE);
        break;

    case HD_SCSICMD:
        if (sizeof(struct SCSICmd) != len)
            goto bad_length;
        if (ap->ap_type == ATA_PORT_T_DISK)
            done = ahci_scsi_disk_io(io, data);
        else if (ap->ap_type == ATA_PORT_T_ATAPI)
            done = ahci_scsi_atapi_io(io, data);
        else
            goto bad_cmd;
        break;

    case TD_ADDCHANGEINT:
        ahciDebug("[AHCI%02ld]" AHCI_IO_STR " TD_ADDCHANGEINT\n", unit->sim_Unit AHCI_IO_DBG);
        if (io->io_Flags & IOF_QUICK)
            goto bad_cmd;
        break;

    case TD_REMCHANGEINT:
        ahciDebug("[AHCI%02ld]" AHCI_IO_STR " TD_REMCHANGEINT\n", unit->sim_Unit AHCI_IO_DBG);
        if (io->io_Flags & IOF_QUICK)
            goto bad_cmd;
        done = TRUE;
        break;

    case CMD_CLEAR:
        // FIXME: Implemennt cache invalidate
        done = TRUE;
        break;

    case CMD_UPDATE:
        // FIXME: Implement cache flush
        done = TRUE;
        break;

    default:
        ahciDebug("ahci.device %d: Unknown IO command %d\n", unit->sim_Unit, io->io_Command);
bad_cmd:
        ahciDebug("[AHCI%02ld] WARN -" AHCI_IO_STR " Fault, io_Flags = %d, IOERR_NOCMD (io_Command = %d), len = %d\n", unit->sim_Unit AHCI_IO_DBG, io->io_Flags, io->io_Command, len);
        io->io_Error = IOERR_NOCMD;
        done = TRUE;
        break;
bad_length:
        ahciDebug("[AHCI%02ld]" AHCI_IO_STR " Fault, io_Flags = %d, io_Command = %d, IOERR_BADLENGTH (len = %d)\n", unit->sim_Unit AHCI_IO_DBG, io->io_Flags, io->io_Command, len);
        io->io_Error = IOERR_BADLENGTH;
        done = TRUE;
        break;
bad_address:
        ahciDebug("[AHCI%02ld] WARN -" AHCI_IO_STR " IOERR_BADADDRESS Fault, io_Flags = %d, io_Command = %d, len = %d\n", unit->sim_Unit AHCI_IO_DBG, io->io_Flags, io->io_Command, len);
        io->io_Error = IOERR_BADADDRESS;
        done = TRUE;
        break;
    }

    /* The IO is finished, so no need to keep it around anymore */
    if (done) {
        ObtainSemaphore(&unit->sim_Lock);
        Remove(&io->io_Message.mn_Node);
        ReleaseSemaphore(&unit->sim_Lock);
    } else
        io->io_Flags &= ~IOF_QUICK;
        

    /* Need a reply now? */
    if (done && !(io->io_Flags & IOF_QUICK))
        ReplyMsg(&io->io_Message);

    if (done)
        ahciDebug("[AHCI%02ld]" AHCI_IO_STR " Quick, io_Flags = %d, io_Comand = %d, io_Error = %d\n", unit->sim_Unit AHCI_IO_DBG, io->io_Flags, io->io_Command, io->io_Error);
    ahciDebug("[AHCI%02ld] %s:" AHCI_IO_STR " finished\n", unit->sim_Unit, __func__ AHCI_IO_DBG);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IORequest *, io, A1),
    struct AHCIBase *, AHCIBase, 6, ahci)
{
    AROS_LIBFUNC_INIT

    struct cam_sim *unit = (struct cam_sim *)io->io_Unit;
    ahciDebug("[AHCI%02ld] %s(0x%p)\n", unit->sim_Unit, __func__, io);

    /* Aborting IOs is not (yet) supported */
    return 0;

    AROS_LIBFUNC_EXIT
}


/* vim: set ts=8 sts=4 et : */
