/*
    Copyright (C) 2020-2026, The AROS Development Team. All rights reserved.
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
#include "nvme_timer.h"

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
            __atomic_store_n(&nvmeq->cmdid_busy[slot], 1, __ATOMIC_RELEASE);
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

    __atomic_store_n(&nvmeq->cmdid_busy[cmdid], 0, __ATOMIC_RELEASE);
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
#if defined(__aarch64__)
    __asm__ __volatile__("dmb oshst" ::: "memory");
#elif defined(__arm__)
    __asm__ __volatile__("dmb sy" ::: "memory");
#else
    __atomic_thread_fence(__ATOMIC_RELEASE);
#endif
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

    UWORD cmdid = AROS_LE2WORD(cqe->command_id);
    if (cmdid < nvmeq->q_depth && nvmeq->cehooks[cmdid]) {
        D(bug ("[NVME:HW] %s: calling completion hook\n", __func__);)
        nvmeq->cehooks[cmdid](nvmeq, cqe);
    }
}

int nvme_process_cq(struct nvme_queue *nvmeq)
{
    UWORD head, phase;
    int processed = 0;

    D(bug ("[NVME:HW] %s(0x%p)\n", __func__, nvmeq);)

    /*
     * The queue is drained both by the interrupt and by the tasks that
     * poll while they wait, so the walk has to be atomic against them
     * - the same protection the submission side already takes. Losing
     * it lets two walkers consume the same entry and leaves our idea
     * of the head disagreeing with the controller's, which on a shared
     * pin means it keeps the line asserted for a completion nobody
     * will ever collect.
     */
    Disable();
#if defined(__AROSEXEC_SMP__)
    {
        struct NVMEBase *NVMEBase = nvmeq->dev->dev_NVMEBase;
        (void)NVMEBase;
        KrnSpinLock(&nvmeq->q_lock, NULL, SPINLOCK_MODE_WRITE);
#endif

    if (nvmeq->cq_processing || nvmeq->cq_stopped) {
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&nvmeq->q_lock);
#endif
        Enable();
        return 0;
    }
    nvmeq->cq_processing = TRUE;

    head = nvmeq->cq_head;
    phase = nvmeq->cq_phase;

    D(bug ("[NVME:HW] %s: head=%u, phase=%u\n", __func__, head, phase);)
    for (;;) {
        struct nvme_completion *cqe = (struct nvme_completion *)&nvmeq->cqba[head];

        if ((AROS_LE2WORD(cqe->status) & 1) != phase) {
            break;
        }
#if defined(__aarch64__)
        __asm__ __volatile__("dmb oshld" ::: "memory");
#elif defined(__arm__)
        __asm__ __volatile__("dmb sy" ::: "memory");
#else
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
#endif
        nvmeq->sq_head = AROS_LE2WORD(cqe->sq_head);
        if (++head == nvmeq->q_depth) {
            head = 0;
            phase = !phase;
        }

        /* The callback can take q_lock itself. Keep ownership of the whole
           walk while it runs: another walker must not ring the doorbell and
           allow the controller to overwrite the entry passed to the hook. */
        nvmeq->cq_head = head;
        nvmeq->cq_phase = phase;
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&nvmeq->q_lock);
#endif
        nvme_complete_event(nvmeq, cqe);
#if defined(__AROSEXEC_SMP__)
        KrnSpinLock(&nvmeq->q_lock, NULL, SPINLOCK_MODE_WRITE);
#endif
        processed++;
    }

    /*
     * Only ring the doorbell when we actually consumed something. Telling
     * the controller a head it already has is not just wasted MMIO: it makes
     * a controller that tracks outstanding completions decide the interrupt
     * is still owed, re-assert its (level) INTx, and so drive an interrupt
     * storm that starves the machine - this is what QEMU's controller does,
     * and it wedges the boot as soon as the first admin command completes.
     */
    if (processed) {
        D(bug ("[NVME:HW] %s: updating head=%u, phase=%u\n", __func__, head, phase);)
        nvmeq->q_db[1 << nvmeq->dev->db_stride] = head;
        nvmeq->cq_head = head;
        nvmeq->cq_phase = phase;
    }
    nvmeq->cq_processing = FALSE;
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&nvmeq->q_lock);
    }
#endif
    Enable();

    D(bug ("[NVME:HW] %s: finished\n", __func__);)

    return processed;
}

/* Stop new walkers, then wait for any callback already outside q_lock. */
void nvme_stop_completions(struct nvme_queue *nvmeq, struct IORequest *tmr)
{
#if defined(__AROSEXEC_SMP__)
    struct NVMEBase *NVMEBase = nvmeq->dev->dev_NVMEBase;
#endif
    BOOL processing;
    do {
        Disable();
#if defined(__AROSEXEC_SMP__)
        KrnSpinLock(&nvmeq->q_lock, NULL, SPINLOCK_MODE_WRITE);
#endif
        nvmeq->cq_stopped = TRUE;
        processing = nvmeq->cq_processing;
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&nvmeq->q_lock);
#endif
        Enable();
        if (processing)
            nvme_WaitTO(tmr, 0, 1000, 0);
    } while (processing);
}
