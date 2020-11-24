/*
    Copyright © 2020, The AROS Development Team. All rights reserved
    $Id$
*/
 
#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>
#include <proto/utility.h>

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

#include "nvme_intern.h"
//#include "timer.h"

#include LC_LIBDEFS_FILE

/*
    Try to do IO commands. All commands which require talking with nvme devices
    will be handled slow, that is they will be passed to bus task which will
    execute them as soon as hardware will be free.
*/
AROS_LH1(void, BeginIO,
    AROS_LHA(struct IORequest *, io, A1),
    struct NVMEBase *, NVMEBase, 5, nvme)
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
        TD_GETGEOMETRY,
        TD_MOTOR,
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
    struct nvme_Unit *unit = (struct nvme_Unit *)io->io_Unit;
#if (0)
    struct nvme_port *ap = unit->au_Port;
    struct ata_port  *at = ap->ap_ata[0];
#endif
    APTR data = iotd->iotd_Req.io_Data;
    ULONG len = iotd->iotd_Req.io_Length;
    UQUAD off64;
    struct DriveGeometry *geom;
    struct NSDeviceQueryResult *nsqr;
    BOOL done = (io->io_Flags & IOF_QUICK) ? TRUE : FALSE;

    io->io_Message.mn_Node.ln_Type = NT_MESSAGE;
    io->io_Error = 0;

    D(bug("[NVME%02ld] IO %p Start, io_Flags = %d, io_Command = %d\n", unit->au_UnitNum, io, io->io_Flags, io->io_Command));
#if (0)

    /* Unit going offline? Don't permit new commands. */
    if (unit->au_Flags & SIMF_OffLine) {
        io->io_Error = IOERR_OPENFAIL;
        if (!(io->io_Flags & IOF_QUICK))
            ReplyMsg(&io->io_Message);
        return;
    }
#endif

    ObtainSemaphore(&unit->au_Lock);
    AddHead(&unit->au_IOs, &io->io_Message.mn_Node);
    ReleaseSemaphore(&unit->au_Lock);

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
        done = TRUE;
        break;

    case TD_PROTSTATUS:
    case TD_CHANGENUM:
    case TD_CHANGESTATE:
        IOStdReq(io)->io_Actual = 0;
        done = TRUE;
        break;

    case TD_EJECT:
        done = FALSE;
        break;

    case TD_GETDRIVETYPE:
        IOStdReq(io)->io_Actual = DRIVE_NEWSTYLE;
        done = TRUE;
        break;

    case TD_GETGEOMETRY:
        if (len < sizeof(*geom))
            goto bad_length;

        geom = data;
        SetMem(geom, 0, len);

        geom->dg_Heads        = 1;
        geom->dg_SectorSize   = 1 << unit->au_SecShift;
        if (unit->au_SecCnt >> 32 != 0)
            geom->dg_TotalSectors = 0xffffffff;
        else
            geom->dg_TotalSectors = unit->au_SecCnt;
        geom->dg_Cylinders    = 1;
        geom->dg_CylSectors   = 1;
        geom->dg_TrackSectors = geom->dg_TotalSectors;
        geom->dg_BufMemType   = MEMF_PUBLIC;
        geom->dg_DeviceType   = DG_DIRECT_ACCESS;
        geom->dg_Flags        = 0;
        IOStdReq(io)->io_Actual = sizeof(*geom);
        done = TRUE;
        break;

    case TD_FORMAT:
#if (0)
        if (len & (at->at_identify.nsectors * at->at_identify.sector_size - 1))
            goto bad_length;
        off64  = iotd->iotd_Req.io_Offset;
        if (off64 & (at->at_identify.nsectors * at->at_identify.sector_size - 1))
            goto bad_address;
        done = nvme_sector_rw(io, off64, TRUE);
#else
        done = TRUE;
#endif
        break;

    case TD_MOTOR:
        // FIXME: Tie in with power management
        IOStdReq(io)->io_Actual = 1;
        done = TRUE;
        break;

    case CMD_WRITE:
        off64  = iotd->iotd_Req.io_Offset;
#if (0)
        done = nvme_sector_rw(io, off64, TRUE);
#else
        done = TRUE;
#endif
        break;

    case TD_WRITE64:
    case NSCMD_TD_WRITE64:
        off64  = iotd->iotd_Req.io_Offset;
        off64 |= ((UQUAD)iotd->iotd_Req.io_Actual)<<32;
#if (0)
        done = nvme_sector_rw(io, off64, TRUE);
#else
        done = TRUE;
#endif
        break;

    case CMD_READ:
        off64  = iotd->iotd_Req.io_Offset;
#if (0)
        done = nvme_sector_rw(io, off64, FALSE);
#else
        done = TRUE;
#endif
        break;

    case TD_READ64:
    case NSCMD_TD_READ64:
        off64  = iotd->iotd_Req.io_Offset;
        off64 |= ((UQUAD)iotd->iotd_Req.io_Actual)<<32;
#if (0)
        done = nvme_sector_rw(io, off64, FALSE);
#else
        done = TRUE;
#endif
        break;

    case HD_SCSICMD:
        if (sizeof(struct SCSICmd) != len)
            goto bad_length;
#if (0)
        if (ap->ap_type == ATA_PORT_T_DISK)
            done = nvme_scsi_disk_io(io, data);
        else if (ap->ap_type == ATA_PORT_T_ATAPI)
            done = nvme_scsi_atapi_io(io, data);
        else
            goto bad_cmd;
#else
        done = TRUE;
#endif
        break;

    case TD_ADDCHANGEINT:
        if (io->io_Flags & IOF_QUICK)
            goto bad_cmd;
        break;

    case TD_REMCHANGEINT:
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
        bug("nvme.device %d: Unknown IO command %d\n", unit->au_UnitNum, io->io_Command);
bad_cmd:
        io->io_Error = IOERR_NOCMD;
        done = TRUE;
        break;
bad_length:
        D(bug("[NVME%02ld] IO %p Fault, io_Flags = %d, io_Command = %d, IOERR_BADLENGTH (len = %d)\n", unit->au_UnitNum, io, io->io_Flags, io->io_Command, len));
        io->io_Error = IOERR_BADLENGTH;
        done = TRUE;
        break;
bad_address:
        io->io_Error = IOERR_BADADDRESS;
        done = TRUE;
        break;
    }

    /* The IO is finished, so no need to keep it around anymore */
    if (done) {
        ObtainSemaphore(&unit->au_Lock);
        Remove(&io->io_Message.mn_Node);
        ReleaseSemaphore(&unit->au_Lock);
    } else
        io->io_Flags &= ~IOF_QUICK;

    /* Need a reply now? */
    if (done && !(io->io_Flags & IOF_QUICK))
        ReplyMsg(&io->io_Message);

    if (done)
        D(bug("[NVME%02ld] IO %p Quick, io_Flags = %d, io_Comand = %d, io_Error = %d\n", unit->au_UnitNum, io, io->io_Flags, io->io_Command, io->io_Error));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IORequest *, io, A1),
    struct NVMEBase *, NVMEBase, 6, nvme)
{
    AROS_LIBFUNC_INIT

    /* Aborting IOs is not (yet) supported */
    return 0;

    AROS_LIBFUNC_EXIT
}


/* vim: set ts=8 sts=4 et : */
