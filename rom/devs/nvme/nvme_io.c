/*
    Copyright (C) 2020-2025, The AROS Development Team. All rights reserved
*/

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/kernel.h>
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

#include "nvme_debug.h"
#include "nvme_intern.h"
#include "nvme_queue_io.h"

#include LC_LIBDEFS_FILE

extern BOOL nvme_initprp(struct nvme_command *cmdio, struct completionevent_handler *ioehandle, struct nvme_Unit *unit, ULONG len, APTR *data, BOOL is_write);
extern BOOL nvme_initsgl(struct nvme_command *cmdio, struct completionevent_handler *ioehandle, struct nvme_Unit *unit, ULONG len, APTR *data, BOOL is_write);

static BOOL nvme_sector_rw(struct IORequest *io, UQUAD off64, BOOL is_write)
{
    struct IOExtTD *iotd = (struct IOExtTD *)io;
    struct nvme_Unit *unit = (struct nvme_Unit *)io->io_Unit;
    struct NVMEBase *NVMEBase = unit->au_Bus->ab_Base;
    device_t dev = unit->au_Bus->ab_Dev;
    APTR data = iotd->iotd_Req.io_Data;
    ULONG len = iotd->iotd_Req.io_Length;
    struct nvme_queue *nvmeq;
    struct completionevent_handler ioehandle;
    struct nvme_command cmdio;
    int queueno;
    BOOL setup_ok = FALSE;

    D(
        bug("[NVME%02ld] %s(%08x%08x, 0x%p, %u) %s\n", unit->au_UnitNum, __func__, (off64 >> 32), off64 & 0xFFFFFFFF, data, len, is_write ? "WRITE" : "READ");

        bug("[NVME%02ld] %s: bus @ 0x%p\n", unit->au_UnitNum, __func__, unit->au_Bus);
        bug("[NVME%02ld] %s: bus dev @ 0x%p\n", unit->au_UnitNum, __func__, unit->au_Bus->ab_Dev);

        bug("[NVME%02ld] %s: %u queues available\n", unit->au_UnitNum, __func__, unit->au_Bus->ab_Dev->queuecnt);
    )

    if ((off64 >> unit->au_SecShift) > unit->au_High) {
        bug("[NVME%02ld] %s: BADADDRESS %x > %x\n", unit->au_UnitNum, __func__, (off64 >> unit->au_SecShift), unit->au_High);
        io->io_Error = IOERR_BADADDRESS;
        return TRUE;
    } else if ((len == 0) || (len > (1 << unit->au_Bus->ab_Dev->dev_mdts) * unit->au_Bus->ab_Dev->pagesize)) {
        bug("[NVME%02ld] %s: BADLENGTH (writing %u bytes to %x)\n", unit->au_UnitNum, __func__, len, (off64 >> unit->au_SecShift));
        io->io_Error = IOERR_BADLENGTH;
        return TRUE;
    }

    memset(&cmdio, 0, sizeof(cmdio));
    memset(&ioehandle, 0, sizeof(ioehandle));
    nvme_dma_init(&ioehandle);

    if (dev && (dev->dev_Features & NVME_DEVF_SGL_SUPPORTED)) {
        if (nvme_initsgl(&cmdio, &ioehandle, unit, len, &data, is_write)) {
            setup_ok = TRUE;
        } else {
            nvme_dma_release(&ioehandle, TRUE);
            nvme_dma_init(&ioehandle);
            ioehandle.ceh_IOMem.me_Un.meu_Addr = NULL;
            ioehandle.ceh_IOMem.me_Length = 0;
        }
    }

    if (!setup_ok) {
        if (!nvme_initprp(&cmdio, &ioehandle, unit, len, &data, is_write)) {
            nvme_dma_release(&ioehandle, TRUE);
            io->io_Error = IOERR_BADADDRESS;
            return TRUE;
        }
    }

    ULONG nsid = (unit->au_UnitNum & ((1 << 12) - 1)) + 1;
    queueno = 1 + (KrnGetCPUNumber() % unit->au_Bus->ab_Dev->queuecnt);
    nvmeq = unit->au_Bus->ab_Dev->dev_Queues[queueno];

    DIO(bug("[NVME%02ld(%02u)] %s: queue @ 0x%p\n", unit->au_UnitNum, queueno, __func__, nvmeq);)

    ioehandle.ceh_Task = nvmeq->q_IOTask;
    ioehandle.ceh_SigSet = SIGF_SINGLE;
    ioehandle.ceh_Msg = io;

    ObtainSemaphore(&unit->au_Lock);
    Remove(&io->io_Message.mn_Node);
    ReleaseSemaphore(&unit->au_Lock);

    if (is_write) {
        cmdio.rw.op.opcode = nvme_cmd_write;
    } else {
        cmdio.rw.op.opcode = nvme_cmd_read;
    }

    cmdio.rw.nsid = AROS_LONG2LE(nsid);
    cmdio.rw.length = AROS_WORD2LE((len >> unit->au_SecShift) - 1);
    cmdio.rw.slba = AROS_QUAD2LE(off64 >> unit->au_SecShift);
    cmdio.rw.control = 0;
    cmdio.rw.dsmgmt = 0;

    DIO(
        bug("[NVME%02ld(%02u)] %s: %08x%08x (%u)\n", unit->au_UnitNum, queueno, __func__, (cmdio.rw.slba >> 32), (cmdio.rw.slba & 0xFFFFFFFF), AROS_LE2WORD(cmdio.rw.length));
    )

    if (nvme_submit_iocmd(nvmeq, &cmdio, &ioehandle) != 0) {
        if (ioehandle.ceh_IOMem.me_Un.meu_Addr) {
            FreeMem(ioehandle.ceh_IOMem.me_Un.meu_Addr, ioehandle.ceh_IOMem.me_Length);
            ioehandle.ceh_IOMem.me_Un.meu_Addr = NULL;
        }
        nvme_dma_release(&ioehandle, TRUE);
        io->io_Error = IOERR_ABORTED;
        return TRUE;
    }

    return FALSE;
}


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
        TD_CHANGESTATE,
        TD_SEEK,
        TD_FORMAT,
        TD_GETGEOMETRY,
        TD_MOTOR,
        TD_PROTSTATUS,
        TD_READ64,
        TD_WRITE64,
        TD_SEEK64,
        TD_FORMAT64,
        NSCMD_DEVICEQUERY,
        NSCMD_TD_READ64,
        NSCMD_TD_WRITE64,
        NSCMD_TD_SEEK64,
        NSCMD_TD_FORMAT64,
        0
    };
    struct IOExtTD *iotd = (struct IOExtTD *)io;
    struct nvme_Unit *unit = (struct nvme_Unit *)io->io_Unit;
    APTR data = iotd->iotd_Req.io_Data;
    ULONG len = iotd->iotd_Req.io_Length;
    UQUAD off64;
    struct DriveGeometry *geom;
    struct NSDeviceQueryResult *nsqr;
    BOOL done = (io->io_Flags & IOF_QUICK) ? TRUE : FALSE;

    io->io_Message.mn_Node.ln_Type = NT_MESSAGE;
    io->io_Error = 0;

    D(bug("[NVME%02ld] IO %p Start, io_Flags = %d, io_Command = %d\n", unit->au_UnitNum, io, io->io_Flags, io->io_Command));
    ObtainSemaphore(&unit->au_Lock);
    AddHead(&unit->au_IOs, &io->io_Message.mn_Node);
    ReleaseSemaphore(&unit->au_Lock);

    switch (io->io_Command) {
    case CMD_READ:
        D(bug("[NVME%02ld] CMD_READ ", unit->au_UnitNum);)
        off64  = iotd->iotd_Req.io_Offset;
        D(bug("%08x (%u)\n", off64 & 0xFFFFFFFF, iotd->iotd_Req.io_Length);)
        done = nvme_sector_rw(io, off64, FALSE);
        break;

    case TD_READ64:
    case NSCMD_TD_READ64:
        D(bug("[NVME%02ld] TD_READ64 ", unit->au_UnitNum);)
        off64  = iotd->iotd_Req.io_Offset;
        off64 |= ((UQUAD)iotd->iotd_Req.io_Actual)<<32;
        D(bug("%08x%08x (%u)\n", off64 >> 32, off64 & 0xFFFFFFFF, iotd->iotd_Req.io_Length);)
        done = nvme_sector_rw(io, off64, FALSE);
        break;

    case CMD_WRITE:
        D(bug("[NVME%02ld] CMD_WRITE ", unit->au_UnitNum);)
        off64  = iotd->iotd_Req.io_Offset;
        D(bug("%08x (%u)\n", off64 & 0xFFFFFFFF, iotd->iotd_Req.io_Length);)
        done = nvme_sector_rw(io, off64, TRUE);
        break;

    case TD_WRITE64:
    case NSCMD_TD_WRITE64:
        D(bug("[NVME%02ld] TD_WRITE64 ", unit->au_UnitNum);)
        off64  = iotd->iotd_Req.io_Offset;
        off64 |= ((UQUAD)iotd->iotd_Req.io_Actual)<<32;
        D(bug("%08x%08x (%u)\n", off64 >> 32, off64 & 0xFFFFFFFF, iotd->iotd_Req.io_Length);)
        done = nvme_sector_rw(io, off64, TRUE);
        break;

    case TD_FORMAT:
        D(bug("[NVME%02ld] TD_FORMAT\n", unit->au_UnitNum);)
        off64  = iotd->iotd_Req.io_Offset;
        done = nvme_sector_rw(io, off64, TRUE);
        break;

    case TD_FORMAT64:
    case NSCMD_TD_FORMAT64:
        D(bug("[NVME%02ld] TD_FORMAT64\n", unit->au_UnitNum);)
        off64  = iotd->iotd_Req.io_Offset;
        off64 |= ((UQUAD)iotd->iotd_Req.io_Actual)<<32;
        done = nvme_sector_rw(io, off64, TRUE);
        break;

    case TD_SEEK:
        D(bug("[NVME%02ld] TD_SEEK\n", unit->au_UnitNum);)
        IOStdReq(io)->io_Actual = 0;
        done = TRUE;
        break;

    case TD_SEEK64:
    case NSCMD_TD_SEEK64:
        D(bug("[NVME%02ld] TD_SEEK64\n", unit->au_UnitNum);)
        IOStdReq(io)->io_Actual = 0;
        done = TRUE;
        break;

    case TD_CHANGESTATE:
        D(bug("[NVME%02ld] TD_CHANGESTATE\n", unit->au_UnitNum);)
        IOStdReq(io)->io_Actual = 0;
        done = TRUE;
        break;

    case NSCMD_DEVICEQUERY:
        D(bug("[NVME%02ld] TD_DEVICEQUERY\n", unit->au_UnitNum);)
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
        D(bug("[NVME%02ld] TD_PROTSTATUS\n", unit->au_UnitNum);)
        IOStdReq(io)->io_Actual = 0;
        done = TRUE;
        break;

    case TD_GETDRIVETYPE:
        D(bug("[NVME%02ld] TD_GETDRIVETYPE\n", unit->au_UnitNum);)
        IOStdReq(io)->io_Actual = DRIVE_NEWSTYLE;
        done = TRUE;
        break;

    case TD_GETGEOMETRY:
        D(bug("[NVME%02ld] TD_GETGEOMETRY\n", unit->au_UnitNum);)
        if (len < sizeof(*geom))
            goto bad_length;

        geom = data;
        SetMem(geom, 0, len);

        geom->dg_Heads        = unit->nu_Heads;
        geom->dg_SectorSize   = 1 << unit->au_SecShift;
        if (unit->au_SecCnt >> 32 != 0)
            geom->dg_TotalSectors = 0xFFFFFFFF;
        else
            geom->dg_TotalSectors = unit->au_SecCnt;
        geom->dg_Cylinders    = unit->nu_Cyl;
        geom->dg_CylSectors   = 63 * unit->nu_Heads;
        geom->dg_TrackSectors = 63;
        geom->dg_BufMemType   = MEMF_PUBLIC;
        geom->dg_DeviceType   = DG_DIRECT_ACCESS;
        geom->dg_Flags        = 0;
        IOStdReq(io)->io_Actual = sizeof(*geom);
        done = TRUE;
        break;

    case TD_ADDCHANGEINT:
        D(bug("[NVME%02ld] TD_ADDCHANGEINT\n", unit->au_UnitNum);)
        if (io->io_Flags & IOF_QUICK)
            goto bad_cmd;
        break;

    case TD_REMCHANGEINT:
        D(bug("[NVME%02ld] TD_REMCHANGEINT\n", unit->au_UnitNum);)
        if (io->io_Flags & IOF_QUICK)
            goto bad_cmd;
        done = TRUE;
        break;

    case TD_MOTOR:
        D(bug("[NVME%02ld] TD_MOTOR\n", unit->au_UnitNum);)
        // FIXME: Tie in with power management
        IOStdReq(io)->io_Actual = 1;
        done = TRUE;
        break;

    case CMD_CLEAR:
        D(bug("[NVME%02ld] TD_CLEAR\n", unit->au_UnitNum);)
        // FIXME: Implemennt cache invalidate
        done = TRUE;
        break;

    case CMD_UPDATE:
        D(bug("[NVME%02ld] TD_UPDATE\n", unit->au_UnitNum);)
        // FIXME: Implement cache flush
        done = TRUE;
        break;

    default:
        bug("nvme.device %u: Unknown IO command %d\n", unit->au_UnitNum, io->io_Command);
bad_cmd:
        io->io_Error = IOERR_NOCMD;
        done = TRUE;
        break;
bad_length:
        bug("nvme.device %u: IO %p Fault, io_Flags = %d, io_Command = %d, IOERR_BADLENGTH (len = %d)\n", unit->au_UnitNum, io, io->io_Flags, io->io_Command, len);
        io->io_Error = IOERR_BADLENGTH;
        done = TRUE;
        break;
bad_address:
        bug("nvme.device %u: IO %p Fault, io_Flags = %d, io_Command = %d, len = %d, IOERR_BADADDRESS\n", unit->au_UnitNum, io, io->io_Flags, io->io_Command, len);
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
