/*
    Copyright (C) 2020-2025, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include <asm/io.h>
#include <hidd/pci.h>

#include <string.h>

#if defined(__AROSEXEC_SMP__)
#include <proto/kernel.h>
#endif

#include "nvme_debug.h"
#include "nvme_intern.h"
#include "nvme_hw.h"

/*
    Admin Queue Support Functions
*/

void nvme_complete_adminevent(struct nvme_queue *nvmeq, struct nvme_completion *cqe)
{
    struct completionevent_handler *handler;

    D(bug("[NVME:ADMINQ] %s(0x%p)\n", __func__, cqe);)

    if ((handler = nvmeq->cehandlers[cqe->command_id]) != NULL) {
        D(bug ("[NVME:ADMINQ] %s: cehandler @ 0x%p\n", __func__, handler);)
        handler->ceh_Result = AROS_LE2LONG(cqe->result);
        handler->ceh_Status = AROS_LE2WORD(cqe->status) >> 1;
        nvmeq->cehandlers[cqe->command_id] = NULL;
        nvme_dma_release(handler, TRUE);
        D(bug ("[NVME:ADMINQ] %s: Signaling 0x%p (%08x)\n", __func__, handler->ceh_Task, handler->ceh_SigSet);)
        Signal(handler->ceh_Task, handler->ceh_SigSet);
        nvmeq->cehooks[cqe->command_id] = NULL;
        nvme_release_cmdid(nvmeq, cqe->command_id);
    }
}

int nvme_submit_admincmd(device_t dev, struct nvme_command *cmd, struct completionevent_handler *handler)
{
    int retval;
    int cmdid;

    D(bug("[NVME:ADMINQ] %s(0x%p, 0x%p)\n", __func__, dev, cmd);)

    cmdid = nvme_alloc_cmdid(dev->dev_Queues[0]);
    if (cmdid < 0) {
        return -1;
    }

    cmd->common.op.command_id = cmdid;

    dev->dev_Queues[0]->cehooks[cmdid] = nvme_complete_adminevent;
    dev->dev_Queues[0]->cehandlers[cmdid] = handler;

    if (handler) {
        /* clear the signal first */
        SetSignal(0, handler->ceh_SigSet);
    }

    retval = nvme_submit_cmd(dev->dev_Queues[0], cmd);
    if (retval != 0) {
        dev->dev_Queues[0]->cehooks[cmdid] = NULL;
        dev->dev_Queues[0]->cehandlers[cmdid] = NULL;
        if (handler) {
            nvme_dma_release(handler, TRUE);
        }
        nvme_release_cmdid(dev->dev_Queues[0], cmdid);
    }

    return retval;
}
