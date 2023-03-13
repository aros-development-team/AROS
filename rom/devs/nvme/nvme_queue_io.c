/*
    Copyright (C) 2020-2023, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <asm/io.h>
#include <hidd/pci.h>
#include <interface/Hidd_PCIDriver.h>
#include <exec/errors.h>

#include <string.h>

#if defined(__AROSEXEC_SMP__)
#include <proto/kernel.h>
#endif

#include "nvme_debug.h"
#include "nvme_intern.h"
#include "nvme_hw.h"
#include "nvme_queue_io.h"

/*
    IO Queue Support Functions
*/

void nvme_complete_ioevent(struct nvme_queue *nvmeq, struct nvme_completion *cqe)
{
    D(bug ("[NVME:IOQ] %s(0x%p)\n", __func__, cqe);)
    if (nvmeq->cehandlers[cqe->command_id])
    {
        D(bug ("[NVME:IOQ] %s: completing queue entry #%u\n", __func__, cqe->command_id);)

        nvmeq->cehandlers[cqe->command_id]->ceh_Reply = TRUE;
        nvmeq->cehandlers[cqe->command_id]->ceh_Result = AROS_LE2LONG(cqe->result);
        nvmeq->cehandlers[cqe->command_id]->ceh_Status = (AROS_LE2WORD(cqe->status) >> 1) & ~(3 << 12); //Cache the status flag masking out the reserved bits

        {
            struct IOExtTD *iotd = (struct IOExtTD *)nvmeq->cehandlers[cqe->command_id]->ceh_Msg;
            APTR dma;
            LONG iolen;

            dma = iotd->iotd_Req.io_Data;
            iolen = (LONG)iotd->iotd_Req.io_Length;

            if ((iotd->iotd_Req.io_Command == CMD_WRITE) ||
                (iotd->iotd_Req.io_Command == TD_WRITE64) ||
                (iotd->iotd_Req.io_Command == NSCMD_TD_WRITE64) ||
                (iotd->iotd_Req.io_Command == TD_FORMAT) ||
                (iotd->iotd_Req.io_Command == TD_FORMAT64))
            {
                CachePostDMA(dma, &iolen, DMAFLAGS_POSTWRITE);
            }
            else
            {
                UBYTE *tmpdata = iotd->iotd_Req.io_Data;
                ULONG x;

                CachePostDMA(dma, &iolen, DMAFLAGS_POSTREAD);
#if defined(NVME_DUMP_READS)
                bug("[NVME:IOQ] %s: Read Data-:", __func__);
                for (x = 0; x < iotd->iotd_Req.io_Length; x++)
                {
                    if ((x % 10) == 0)
                    {
                        bug("\n                    ");
                    }
                    bug("%02x ", (UBYTE)tmpdata[x]);
                }
                if ((x % 10) != 0)
                    bug("\n");
#endif
            }
            /* Free up allocations used for the transfer */
            if (nvmeq->cehandlers[cqe->command_id]->ceh_IOMem.me_Un.meu_Addr)
            {
#if (0)
                ULONG iolen = nvmeq->cehandlers[cqe->command_id]->ceh_IOMem.me_Length;
                CachePostDMA(nvmeq->cehandlers[cqe->command_id]->ceh_IOMem.me_Un.meu_Addr, &iolen, DMAFLAGS_POSTREAD);
#endif
                D(bug ("[NVME:IOQ] %s: Releasing IO Allocation @ %p (%ubytes)\n", __func__, nvmeq->cehandlers[cqe->command_id]->ceh_IOMem.me_Un.meu_Addr, nvmeq->cehandlers[cqe->command_id]->ceh_IOMem.me_Length);)
                FreeMem(nvmeq->cehandlers[cqe->command_id]->ceh_IOMem.me_Un.meu_Addr, nvmeq->cehandlers[cqe->command_id]->ceh_IOMem.me_Length);

                nvmeq->cehandlers[cqe->command_id]->ceh_IOMem.me_Un.meu_Addr = NULL;
            }

            if (nvmeq->cehandlers[cqe->command_id]->ceh_Status)
            {
                UBYTE sct = (nvmeq->cehandlers[cqe->command_id]->ceh_Status >> 7) & 0x7, sc = (nvmeq->cehandlers[cqe->command_id]->ceh_Status) & 0x7F;
                iotd->iotd_Req.io_Error = IOERR_ABORTED;
                D(bug("[NVME:IOQ] %s: NVME IO Error %u:%u\n", __func__, sct, sc);)
            }
            else
            {
                iotd->iotd_Req.io_Error = 0;
                iotd->iotd_Req.io_Actual = iotd->iotd_Req.io_Length;
            }
        }

        D(bug ("[NVME:IOQ] %s: Signaling 0x%p (%08x)\n", __func__, nvmeq->cehandlers[cqe->command_id]->ceh_Task, nvmeq->cehandlers[cqe->command_id]->ceh_SigSet);)
        Signal(nvmeq->cehandlers[cqe->command_id]->ceh_Task, nvmeq->cehandlers[cqe->command_id]->ceh_SigSet);
    }
}

int nvme_submit_iocmd(struct completionevent_handler *ce,
                                    struct nvme_queue *nvmeq,
                                    struct nvme_command *cmd,
                                    struct completionevent_handler *handler)
{
    int retval;

    D(bug ("[NVME:IOQ] %s(0x%p, 0x%p)\n", __func__, cmd);)

    handler->ceh_Reply = FALSE;
    cmd->common.op.command_id = nvme_alloc_cmdid(nvmeq);
    nvmeq->cehooks[cmd->common.op.command_id] = nvme_complete_ioevent;
    nvmeq->cehandlers[cmd->common.op.command_id] = &ce[cmd->common.op.command_id];
    CopyMem(handler, &ce[cmd->common.op.command_id], sizeof(struct completionevent_handler));
    retval = nvme_submit_cmd(nvmeq, cmd);

    return retval;
}
