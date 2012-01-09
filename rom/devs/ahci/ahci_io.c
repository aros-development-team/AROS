/*
    Copyright © 2004-2011, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

/* Maintainer: Jason S. McMullan <jason.mcmullan@gmail.com>
 */

#define DEBUG 0

#include <aros/debug.h>
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

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>
#include <proto/oop.h>

#include <hidd/pci.h>

#include <string.h>

#include "ahci.h"
#include "ahci_intern.h"
#include "ahci_scsi.h"
#include "timer.h"

#include LC_LIBDEFS_FILE

/* Translate to a SCSI command, since the
 * backend will make that work on both ATAPI
 * and ATA devices.
 */
static BOOL ahci_sector_rw(struct IORequest *io, UQUAD off64, BOOL is_write)
{
    struct IOExtTD *iotd = (struct IOExtTD *)io;
    struct cam_sim *unit = (struct cam_sim *)io->io_Unit;
    struct ahci_port *ap = unit->sim_Port;
    struct ata_port  *at = ap->ap_ata[0];
    APTR data = iotd->iotd_Req.io_Data;
    ULONG len = iotd->iotd_Req.io_Length;
    const ULONG bmask = at->at_identify.sector_size - 1;
    BOOL done = TRUE;
    /* It's safe to allocate these on the stack, since they
     * will never be referenced once the ahci_scsi_*_io() 
     * routine returns.
     */
    struct SCSICmd scsi = {};
    union scsi_cdb cdb = {};

    if (ap->ap_type != ATA_PORT_T_DISK &&
        ap->ap_type != ATA_PORT_T_ATAPI) {
        io->io_Error = TDERR_DiskChanged;
        return TRUE;
    }

    if ((off64 & bmask) || bmask == 0 || data == NULL) {
        io->io_Error = IOERR_BADADDRESS;
        return TRUE;
    }
    if ((len & bmask) || len == 0) {
        io->io_Error = IOERR_BADLENGTH;
        return TRUE;
    }

    if (len == 0) {
        io->io_Error = 0;
        IOStdReq(io)->io_Actual = 0;
        return TRUE;
    }

    scsi.scsi_Data = data;
    scsi.scsi_Length = len;
    scsi.scsi_Flags  = is_write ? SCSIF_WRITE : SCSIF_READ;

    /* Make in units of sector size */
    len /= at->at_identify.sector_size;
    off64 /= at->at_identify.sector_size;

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
    LIBBASETYPEPTR, LIBBASE, 5, ahci)
{
    AROS_LIBFUNC_INIT

    const UWORD NSDSupported[] = {
        CMD_READ,
        CMD_WRITE,
        CMD_UPDATE,
        TD_ADDCHANGEINT,
        TD_CHANGESTATE,
        TD_FORMAT,
        TD_GETGEOMETRY,
        TD_PROTSTATUS,
        TD_READ64,
        TD_REMCHANGEINT,
        TD_WRITE64,
        NSCMD_DEVICEQUERY,
        NSCMD_TD_READ64,
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
    BOOL done = FALSE;

    io->io_Message.mn_Node.ln_Type = NT_MESSAGE;

    D(bug("[AHCI%02ld] BeginIO: Start, io_Flags = %d, io_Command = %d\n", 0, io->io_Flags, io->io_Command));

    ObtainSemaphore(&unit->sim_Lock);

    switch (io->io_Command) {
    case NSCMD_DEVICEQUERY:
        if (len < sizeof(*nsqr))
            goto bad_length;

        nsqr = data;
        nsqr->DevQueryFormat = 0;
        nsqr->SizeAvailable  = sizeof(struct NSDeviceQueryResult);
        nsqr->DeviceType     = NSDEVTYPE_TRACKDISK;
        nsqr->DeviceSubType  = 0;
        nsqr->SupportedCommands = (UWORD *)NSDSupported;
        IOStdReq(io)->io_Actual = sizeof(*nsqr);
        io->io_Error  = 0;
        done = TRUE;
        break;
    case TD_PROTSTATUS:
        io->io_Error = 0;
        IOStdReq(io)->io_Actual = (ap->ap_type == ATA_PORT_T_DISK) ? 0 : -1;
        done = TRUE;
        break;
    case TD_CHANGESTATE:
        io->io_Error = 0;
        IOStdReq(io)->io_Actual = (ap->ap_type == ATA_PORT_T_DISK || ap->ap_type == ATA_PORT_T_ATAPI) ? 0 : 1;
        done = TRUE;
        break;
    case TD_GETDRIVETYPE:
        IOStdReq(io)->io_Actual = DRIVE_NEWSTYLE;
        done = TRUE;
        break;
    case TD_GETGEOMETRY:
        if (len < sizeof(*geom))
            goto bad_length;

        if (ap->ap_type != ATA_PORT_T_DISK && ap->ap_type != ATA_PORT_T_ATAPI)
            goto bad_cmd;

        geom = data;
        memset(geom, 0, len);
        if (ap->ap_type == ATA_PORT_T_DISK) {
            geom->dg_SectorSize   = at->at_identify.sector_size;
            geom->dg_TotalSectors = 0;
            geom->dg_Cylinders    = at->at_identify.ncyls;
            geom->dg_CylSectors   = at->at_identify.nsectors * at->at_identify.nheads;
            geom->dg_Heads        = at->at_identify.nheads;
            geom->dg_TrackSectors = at->at_identify.nsectors;
        }
        geom->dg_BufMemType   = MEMF_PUBLIC;
        geom->dg_DeviceType   = (ap->ap_type == ATA_PORT_T_ATAPI) ? DG_CDROM : DG_DIRECT_ACCESS;
        geom->dg_Flags        = (at->at_identify.config & (1 << 7)) ? DGF_REMOVABLE : 0;
        IOStdReq(io)->io_Actual = sizeof(*geom);
        io->io_Error = 0;
        done = TRUE;
        break;
    case TD_FORMAT:
        if (len & (at->at_identify.nsectors * at->at_identify.sector_size - 1))
            goto bad_length;
        off64  = iotd->iotd_Req.io_Offset;
        if (off64 & (at->at_identify.nsectors * at->at_identify.sector_size - 1))
            goto bad_address;
        done = ahci_sector_rw(io, off64, TRUE);
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
        if (io->io_Flags & IOF_QUICK)
            goto bad_cmd;
        AddHead((struct List *)&unit->sim_ChangeInts, (struct Node *)io);
        io->io_Error = 0;
        break;
    case TD_REMCHANGEINT:
        if (io->io_Flags & IOF_QUICK)
            goto bad_cmd;
        ahci_os_lock_port(ap);
        Remove((struct Node *)io);
        ahci_os_unlock_port(ap);
        io->io_Error = 0;
        done = TRUE;
        break;
    case CMD_UPDATE:
        // FIXME: Implement these
        io->io_Error = 0;
        done = TRUE;
        break;
    default:
        bug("ahci.device %d: Unknown IO command %d\n", unit->sim_Unit, io->io_Command);
bad_cmd:
        io->io_Error = IOERR_NOCMD;
        done = TRUE;
        break;
bad_length:
        io->io_Error = IOERR_BADLENGTH;
        done = TRUE;
        break;
bad_address:
        io->io_Error = IOERR_BADADDRESS;
        done = TRUE;
        break;
    }

    ReleaseSemaphore(&unit->sim_Lock);

    if (done || (io->io_Flags & IOF_QUICK))
        ReplyMsg(&io->io_Message);

    D(bug("[AHCI%02ld] BeginIO: Done, io_Flags = %d, io_Error = %d\n", unit->sim_Unit, io->io_Flags, io->io_Error));
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 6, ahci)
{
    AROS_LIBFUNC_INIT

    /* Cannot Abort IO */
    return 0;

    AROS_LIBFUNC_EXIT
}


/* vim: set ts=8 sts=4 et : */
