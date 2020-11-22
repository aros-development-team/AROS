/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include <asm/io.h>
#include <hidd/pci.h>
#include <interface/Hidd_PCIDriver.h>

#include <string.h>

#include "nvme_intern.h"

int nvme_alloc_cmdid(struct nvme_queue *nvmeq)
{
    int cmdid;
    cmdid = nvmeq->cmdid_data + 1;
    nvmeq->cmdid_data = cmdid;
    return cmdid;
}

int nvme_submit_cmd(struct nvme_queue *nvmeq, struct nvme_command *cmd)
{
    unsigned long flags;
    UWORD tail;

    D(bug ("[NVME:HW] %s(0x%p, 0x%p)\n", __func__, nvmeq, cmd);)
    D(bug ("[NVME:HW] %s: sending command id #%u\n", __func__, cmd->common.op.command_id);)
//	spin_lock_irqsave(&nvmeq->q_lock, flags);

    tail = nvmeq->sq_tail;
    CopyMem(cmd, &nvmeq->sqba[tail], sizeof(struct nvme_command));
    if (++tail == nvmeq->q_depth)
        tail = 0;
    *nvmeq->q_db = tail;
    nvmeq->sq_tail = tail;

//	spin_unlock_irqrestore(&nvmeq->q_lock, flags);

    return 0;
}

struct nvme_queue *nvme_alloc_queue(device_t dev, int qid, int depth, int vector)
{
    struct NVMEBase *NVMEBase = dev->dev_NVMEBase;;
    unsigned extra = 0; //DIV_ROUND_UP(depth, 8) + (depth *
					//	sizeof(struct nvme_cmd_info));
    struct nvme_queue *nvmeq;
    
    D(bug ("[NVME:HW] %s(0x%p, %u, %u, %d)\n", __func__, dev, qid, depth, vector);)

    nvmeq = AllocMem(sizeof(struct nvme_queue) + extra, MEMF_CLEAR);
    D(bug ("[NVME:HW] %s: queue allocated @ 0x%p (depth %u)\n", __func__, nvmeq, depth);)
    if (nvmeq)
    {
        /* completion queue ... */
        nvmeq->cqba = HIDD_PCIDriver_AllocPCIMem(dev->dev_PCIDriverObject, depth * sizeof(struct nvme_completion));
        D(bug ("[NVME:HW] %s:       completion @ 0x%p\n", __func__, nvmeq->cqba);)
        if (nvmeq->cqba)
        {
            memset((void *)nvmeq->cqba, 0, depth * sizeof(struct nvme_completion));

            /* submission queue ... */
            nvmeq->sqba = HIDD_PCIDriver_AllocPCIMem(dev->dev_PCIDriverObject, depth * sizeof(struct nvme_command));
            D(bug ("[NVME:HW] %s:       cmd submission @ 0x%p\n", __func__, nvmeq->sqba);)
            if (nvmeq->sqba)
            {
                nvmeq->dev = dev;

                //spin_lock_init(&nvmeq->q_lock);

                nvmeq->cq_head = 0;
                nvmeq->cq_phase = 1;

                nvmeq->q_db = &dev->dbs[qid << (dev->db_stride + 1)];
                nvmeq->q_depth = depth;
                D(bug ("[NVME:HW] %s:       doorbells @ 0x%p\n", __func__, nvmeq->q_db);)
                nvmeq->cq_vector = vector;
            }
        }
    }
    return nvmeq;
}

void nvme_complete_event(struct nvme_completion *cqe)
{
//        void *ctx;
//            nvme_completion_fn fn;
    D(bug ("[NVME:HW] %s(0x%p)\n", __func__, cqe);)

    D(bug("[NVME:HW] %s: cmd id = %u\n", __func__, cqe->command_id);)
    D(bug("[NVME:HW] %s:     status = %04x\n", __func__, AROS_LE2WORD(cqe->status) >> 1);)
    D(bug("[NVME:HW] %s:     result = %08x\n", __func__, AROS_LE2LONG(cqe->result));)

//		ctx = free_cmdid(nvmeq, cqe.command_id, &fn);
//		fn(nvmeq->dev, ctx, &cqe);
}

void nvme_process_cq(struct nvme_queue *nvmeq)
{
    D(bug ("[NVME:HW] %s(0x%p)\n", __func__, nvmeq);)

    UWORD head, phase;

    head = nvmeq->cq_head;
    phase = nvmeq->cq_phase;

    for (;;) {
        struct nvme_completion *cqe = (struct nvme_completion *)&nvmeq->cqba[head];

        if ((AROS_LE2WORD(cqe->status) & 1) != phase)
                break;
        nvmeq->sq_head = AROS_LE2WORD(cqe->sq_head);
        if (++head == nvmeq->q_depth) {
                head = 0;
                phase = !phase;
        }
        nvme_complete_event(cqe);
    }

    /* If the controller ignores the cq head doorbell and continuously
     * writes to the queue, it is theoretically possible to wrap around
     * the queue twice and mistakenly return IRQ_NONE.  Linux only
     * requires that 0.1% of your interrupts are handled, so this isn't
     * a big problem.
     */
    if ((head != nvmeq->cq_head) || (phase == nvmeq->cq_phase))
    {
        nvmeq->q_db[1 << nvmeq->dev->db_stride] = head;
        nvmeq->cq_head = head;
        nvmeq->cq_phase = phase;
    }
    D(bug ("[NVME:HW] %s: finished\n", __func__);)
}
