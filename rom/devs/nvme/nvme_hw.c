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

#if defined(__AROSEXEC_SMP__)
#include <proto/kernel.h>
#endif

#include "nvme_intern.h"

/*
    Hardware Access Support Functions
*/

int nvme_alloc_cmdid(struct nvme_queue *nvmeq)
{
    int cmdid;

    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&nvmeq->q_lock, NULL, SPINLOCK_MODE_WRITE);
#endif

    cmdid = nvmeq->cmdid_data + 1;
    nvmeq->cmdid_data = cmdid;

#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&nvmeq->q_lock);
#endif
    Enable();

    return cmdid;
}

int nvme_submit_cmd(struct nvme_queue *nvmeq, struct nvme_command *cmd)
{
    unsigned long flags;
    UWORD tail;

    D(bug ("[NVME:HW] %s(0x%p, 0x%p)\n", __func__, nvmeq, cmd);)
    D(bug ("[NVME:HW] %s: sending command id #%u\n", __func__, cmd->common.op.command_id);)

    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&nvmeq->q_lock, NULL, SPINLOCK_MODE_WRITE);
#endif

    tail = nvmeq->sq_tail;
    CopyMem(cmd, &nvmeq->sqba[tail], sizeof(struct nvme_command));
    if (++tail == nvmeq->q_depth)
        tail = 0;
    *nvmeq->q_db = tail;
    nvmeq->sq_tail = tail;

#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&nvmeq->q_lock);
#endif
    Enable();

    return 0;
}

void nvme_complete_event(struct nvme_queue *nvmeq, struct nvme_completion *cqe)
{
    D(bug ("[NVME:HW] %s(0x%p)\n", __func__, cqe);)

    D(bug("[NVME:HW] %s: cmd id = %u\n", __func__, cqe->command_id);)
    D(bug("[NVME:HW] %s:     status = %04x\n", __func__, AROS_LE2WORD(cqe->status) >> 1);)
    D(bug("[NVME:HW] %s:     result = %08x\n", __func__, AROS_LE2LONG(cqe->result));)

    if (nvmeq->cehooks[cqe->command_id])
    {
        D(bug ("[NVME:HW] %s: calling completion hook\n", __func__);)
        nvmeq->cehooks[cqe->command_id](nvmeq, cqe);
    }
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
        nvme_complete_event(nvmeq, cqe);
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
