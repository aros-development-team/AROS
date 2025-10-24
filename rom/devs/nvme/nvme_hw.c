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

/*
    Hardware Access Support Functions
*/

int nvme_alloc_cmdid(struct nvme_queue *nvmeq)
{
#if defined(__AROSEXEC_SMP__)
    struct NVMEBase *NVMEBase = nvmeq->dev->dev_NVMEBase;
#endif
    int cmdid = -1;
    UWORD depth = nvmeq->q_depth;
    UWORD start;
    UWORD idx;

    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&nvmeq->q_lock, NULL, SPINLOCK_MODE_WRITE);
#endif

    start = nvmeq->cmdid_hint;
    for (idx = 0; idx < depth; idx++) {
        UWORD slot = (start + idx) % depth;
        if (!nvmeq->cmdid_busy[slot]) {
            nvmeq->cmdid_busy[slot] = 1;
            nvmeq->cmdid_hint = (slot + 1) % depth;
            cmdid = slot;
            break;
        }
    }

#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&nvmeq->q_lock);
#endif
    Enable();

    return cmdid;
}

void nvme_release_cmdid(struct nvme_queue *nvmeq, UWORD cmdid)
{
#if defined(__AROSEXEC_SMP__)
    struct NVMEBase *NVMEBase = nvmeq->dev->dev_NVMEBase;
#endif

    if (cmdid >= nvmeq->q_depth)
        return;

    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&nvmeq->q_lock, NULL, SPINLOCK_MODE_WRITE);
#endif

    nvmeq->cmdid_busy[cmdid] = 0;
    nvmeq->cmdid_hint = cmdid;

#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&nvmeq->q_lock);
#endif
    Enable();
}

int nvme_submit_cmd(struct nvme_queue *nvmeq, struct nvme_command *cmd)
{
#if defined(__AROSEXEC_SMP__)
    struct NVMEBase *NVMEBase = nvmeq->dev->dev_NVMEBase;
#endif
    UWORD tail;
    UWORD next;

    D(bug ("[NVME:HW] %s(0x%p, 0x%p)\n", __func__, nvmeq, cmd);)
    D(bug ("[NVME:HW] %s: sending command id #%u\n", __func__, cmd->common.op.command_id);)

    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&nvmeq->q_lock, NULL, SPINLOCK_MODE_WRITE);
#endif

    tail = nvmeq->sq_tail;
    next = tail + 1;
    if (next == nvmeq->q_depth)
        next = 0;

    if (next == nvmeq->sq_head) {
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&nvmeq->q_lock);
#endif
        Enable();
        return -1;
    }

    CopyMem(cmd, &nvmeq->sqba[tail], sizeof(struct nvme_command));
    nvmeq->sq_tail = next;
    *nvmeq->q_db = next;

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

    if (nvmeq->cehooks[cqe->command_id]) {
        D(bug ("[NVME:HW] %s: calling completion hook\n", __func__);)
        nvmeq->cehooks[cqe->command_id](nvmeq, cqe);
    }
}

void nvme_process_cq(struct nvme_queue *nvmeq)
{
    UWORD head, phase;

    D(bug ("[NVME:HW] %s(0x%p)\n", __func__, nvmeq);)

    head = nvmeq->cq_head;
    phase = nvmeq->cq_phase;

    D(bug ("[NVME:HW] %s: head=%u, phase=%u\n", __func__, head, phase);)

#if defined(__AROSEXEC_SMP__)
    {
        struct NVMEBase *NVMEBase = nvmeq->dev->dev_NVMEBase;
        (void)NVMEBase;
        KrnSpinLock(&nvmeq->q_lock, NULL, SPINLOCK_MODE_WRITE);
#endif
    for (;;) {
        struct nvme_completion *cqe = (struct nvme_completion *)&nvmeq->cqba[head];

        if ((AROS_LE2WORD(cqe->status) & 1) != phase) {
            break;
        }
        nvmeq->sq_head = AROS_LE2WORD(cqe->sq_head);
        if (++head == nvmeq->q_depth) {
            head = 0;
            phase = !phase;
        }
        nvme_complete_event(nvmeq, cqe);
    }

    if ((head != nvmeq->cq_head) || (phase == nvmeq->cq_phase)) {
        D(bug ("[NVME:HW] %s: updating head=%u, phase=%u\n", __func__, head, phase);)
        nvmeq->q_db[1 << nvmeq->dev->db_stride] = head;
        nvmeq->cq_head = head;
        nvmeq->cq_phase = phase;
    }
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&nvmeq->q_lock);
    }
#endif
    D(bug ("[NVME:HW] %s: finished\n", __func__);)
}
