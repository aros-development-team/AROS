/*
    Copyright (C) 2020-2025, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <asm/io.h>
#include <hidd/pci.h>
#include <exec/errors.h>
#include <exec/memory.h>

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
    if (nvmeq->cehandlers[cqe->command_id]) {
        struct completionevent_handler *slot = nvmeq->cehandlers[cqe->command_id];

        D(bug ("[NVME:IOQ] %s: completing queue entry #%u\n", __func__, cqe->command_id);)

        slot->ceh_Reply = TRUE;
        slot->ceh_Result = AROS_LE2LONG(cqe->result);
        slot->ceh_Status = (AROS_LE2WORD(cqe->status) >> 1) & ~(3 << 12); //Cache the status flag masking out the reserved bits

        {
            struct IOExtTD *iotd = (struct IOExtTD *)slot->ceh_Msg;

            nvme_dma_release(slot, TRUE);

            if (!((iotd->iotd_Req.io_Command == CMD_WRITE) ||
                    (iotd->iotd_Req.io_Command == TD_WRITE64) ||
                    (iotd->iotd_Req.io_Command == NSCMD_TD_WRITE64) ||
                    (iotd->iotd_Req.io_Command == TD_FORMAT) ||
                    (iotd->iotd_Req.io_Command == TD_FORMAT64))) {
                UBYTE *tmpdata = iotd->iotd_Req.io_Data;
                ULONG x;

#if defined(NVME_DUMP_READS)
                bug("[NVME:IOQ] %s: Read Data-:", __func__);
                for (x = 0; x < iotd->iotd_Req.io_Length; x++) {
                    if ((x % 10) == 0) {
                        bug("\n                    ");
                    }
                    bug("%02x ", (UBYTE)tmpdata[x]);
                }
                if ((x % 10) != 0)
                    bug("\n");
#endif
            }
            /* Free up allocations used for the transfer */
            if (slot->ceh_IOMem.me_Un.meu_Addr) {
#if (0)
                ULONG iolen = slot->ceh_IOMem.me_Length;
                CachePostDMA(slot->ceh_IOMem.me_Un.meu_Addr, &iolen, DMAFLAGS_POSTREAD);
#endif
                D(bug ("[NVME:IOQ] %s: Releasing IO Allocation @ %p (%ubytes)\n", __func__, slot->ceh_IOMem.me_Un.meu_Addr, slot->ceh_IOMem.me_Length);)
                FreeMem(slot->ceh_IOMem.me_Un.meu_Addr, slot->ceh_IOMem.me_Length);

                slot->ceh_IOMem.me_Un.meu_Addr = NULL;
            }

            if (slot->ceh_Status) {
                UBYTE sct = (slot->ceh_Status >> 7) & 0x7, sc = (slot->ceh_Status) & 0x7F;
                iotd->iotd_Req.io_Error = IOERR_ABORTED;
                D(bug("[NVME:IOQ] %s: NVME IO Error %u:%u\n", __func__, sct, sc);)
            } else {
                iotd->iotd_Req.io_Error = 0;
                iotd->iotd_Req.io_Actual = iotd->iotd_Req.io_Length;
            }
        }

        D(bug ("[NVME:IOQ] %s: Signaling 0x%p (%08x)\n", __func__, slot->ceh_Task, slot->ceh_SigSet);)
        Signal(slot->ceh_Task, slot->ceh_SigSet);
    }
}

int nvme_submit_iocmd(struct nvme_queue *nvmeq,
                      struct nvme_command *cmd,
                      struct completionevent_handler *handler)
{
    int retval;
    int cmdid;
    struct completionevent_handler *slot;

    D(bug ("[NVME:IOQ] %s(0x%p, 0x%p)\n", __func__, nvmeq, cmd);)

    cmdid = nvme_alloc_cmdid(nvmeq);
    if (cmdid < 0) {
        return -1;
    }

    slot = &nvmeq->ce_entries[cmdid];
    CopyMem(handler, slot, sizeof(struct completionevent_handler));
    slot->ceh_Reply = FALSE;

    if (handler->ceh_DMAMapCount > NVME_INLINE_DMA_SEGMENTS) {
        slot->ceh_DMAMap = AllocMem(handler->ceh_DMAMapCount * sizeof(struct nvme_dma_segment), MEMF_ANY | MEMF_CLEAR);
        if (!slot->ceh_DMAMap) {
            nvme_release_cmdid(nvmeq, cmdid);
            nvme_dma_release(handler, TRUE);
            return -1;
        }
        CopyMem(handler->ceh_DMAMap, slot->ceh_DMAMap,
                handler->ceh_DMAMapCount * sizeof(struct nvme_dma_segment));
        slot->ceh_DMAMapCapacity = handler->ceh_DMAMapCount;
    } else {
        if (handler->ceh_DMAMapCount) {
            CopyMem(handler->ceh_DMAMap, slot->ceh_DMAInline,
                    handler->ceh_DMAMapCount * sizeof(struct nvme_dma_segment));
        }
        slot->ceh_DMAMap = slot->ceh_DMAInline;
        slot->ceh_DMAMapCapacity = NVME_INLINE_DMA_SEGMENTS;
    }
    slot->ceh_DMAMapCount = handler->ceh_DMAMapCount;

    nvme_dma_reset(handler);

    cmd->common.op.command_id = cmdid;
    nvmeq->cehooks[cmdid] = nvme_complete_ioevent;
    nvmeq->cehandlers[cmdid] = slot;

    retval = nvme_submit_cmd(nvmeq, cmd);
    if (retval != 0) {
        nvmeq->cehooks[cmdid] = NULL;
        nvmeq->cehandlers[cmdid] = NULL;
        nvme_dma_release(slot, TRUE);
        nvme_release_cmdid(nvmeq, cmdid);
    }

    return retval;
}
