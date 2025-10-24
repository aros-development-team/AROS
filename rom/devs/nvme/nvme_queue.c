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
    Generic Queue Support Functions
*/

struct nvme_queue *nvme_alloc_queue(device_t dev, int qid, int depth, int vector)
{
    struct nvme_queue *nvmeq;
    size_t cq_bytes = depth * sizeof(struct nvme_completion);
    size_t sq_bytes = depth * sizeof(struct nvme_command);
    struct NVMEBase *NVMEBase = dev ? dev->dev_NVMEBase : NULL;

    D(bug ("[NVME:QUEUE] %s(0x%p, %u, %u, %d)\n", __func__, dev, qid, depth, vector);)

    if (!dev || !dev->dev_PCIDriverObject || !NVMEBase) {
        return NULL;
    }

    (void)NVMEBase;

    nvmeq = AllocMem(sizeof(*nvmeq), MEMF_CLEAR);
    if (!nvmeq) {
        return NULL;
    }

    nvmeq->dev = dev;
    nvmeq->q_depth = depth;
    nvmeq->cq_vector = vector;
    nvmeq->q_db = &dev->dbs[qid << (dev->db_stride + 1)];

#if defined(__AROSEXEC_SMP__)
    KrnSpinInit(&nvmeq->q_lock);
#endif

    nvmeq->cmdid_busy = AllocMem(depth, MEMF_CLEAR);
    if (!nvmeq->cmdid_busy) {
        nvme_free_queue(nvmeq);
        return NULL;
    }

    nvmeq->cehooks = AllocMem(sizeof(_NVMEQUEUE_CE_HOOK) * depth, MEMF_CLEAR);
    if (!nvmeq->cehooks) {
        nvme_free_queue(nvmeq);
        return NULL;
    }

    nvmeq->cehandlers = AllocMem(sizeof(struct completionevent_handler *) * depth, MEMF_CLEAR);
    if (!nvmeq->cehandlers) {
        nvme_free_queue(nvmeq);
        return NULL;
    }

    nvmeq->ce_entries = AllocMem(sizeof(struct completionevent_handler) * depth, MEMF_CLEAR);
    if (!nvmeq->ce_entries) {
        nvme_free_queue(nvmeq);
        return NULL;
    }

    nvmeq->cqba = HIDD_PCIDriver_AllocPCIMem(dev->dev_PCIDriverObject, cq_bytes);
    if (!nvmeq->cqba) {
        nvme_free_queue(nvmeq);
        return NULL;
    }
    memset((void *)nvmeq->cqba, 0, cq_bytes);
    nvmeq->cq_dma = (UQUAD)(IPTR)HIDD_PCIDriver_CPUtoPCI(dev->dev_PCIDriverObject, (APTR)nvmeq->cqba);

    nvmeq->sqba = HIDD_PCIDriver_AllocPCIMem(dev->dev_PCIDriverObject, sq_bytes);
    if (!nvmeq->sqba) {
        nvme_free_queue(nvmeq);
        return NULL;
    }
    memset(nvmeq->sqba, 0, sq_bytes);
    nvmeq->sq_dma = (UQUAD)(IPTR)HIDD_PCIDriver_CPUtoPCI(dev->dev_PCIDriverObject, (APTR)nvmeq->sqba);

    nvmeq->cq_head = 0;
    nvmeq->cq_phase = 1;
    nvmeq->cmdid_hint = 0;

    D(bug ("[NVME:QUEUE] %s: queue allocated @ 0x%p (depth %u)\n", __func__, nvmeq, depth);)
    D(bug ("[NVME:QUEUE] %s:       doorbells @ 0x%p\n", __func__, nvmeq->q_db);)
    D(bug ("[NVME:QUEUE] %s:       completion @ 0x%p (dma %p)\n", __func__, nvmeq->cqba, (APTR)nvmeq->cq_dma);)
    D(bug ("[NVME:QUEUE] %s:       submission @ 0x%p (dma %p)\n", __func__, nvmeq->sqba, (APTR)nvmeq->sq_dma);)

    return nvmeq;
}

void nvme_free_queue(struct nvme_queue *nvmeq)
{
    struct NVMEBase *NVMEBase = (nvmeq && nvmeq->dev) ? nvmeq->dev->dev_NVMEBase : NULL;

    if (!nvmeq) {
        return;
    }

    (void)NVMEBase;

    if (nvmeq->sqba) {
        if (nvmeq->dev && nvmeq->dev->dev_PCIDriverObject && NVMEBase) {
            HIDD_PCIDriver_FreePCIMem(nvmeq->dev->dev_PCIDriverObject, nvmeq->sqba);
        }
        nvmeq->sqba = NULL;
    }

    if (nvmeq->cqba) {
        if (nvmeq->dev && nvmeq->dev->dev_PCIDriverObject && NVMEBase) {
            HIDD_PCIDriver_FreePCIMem(nvmeq->dev->dev_PCIDriverObject, (APTR)nvmeq->cqba);
        }
        nvmeq->cqba = NULL;
    }

    if (nvmeq->ce_entries) {
        UWORD idx;

        for (idx = 0; idx < nvmeq->q_depth; idx++) {
            nvme_dma_release(&nvmeq->ce_entries[idx], FALSE);
        }

        FreeMem(nvmeq->ce_entries, sizeof(struct completionevent_handler) * nvmeq->q_depth);
        nvmeq->ce_entries = NULL;
    }

    if (nvmeq->cehandlers) {
        FreeMem(nvmeq->cehandlers, sizeof(struct completionevent_handler *) * nvmeq->q_depth);
        nvmeq->cehandlers = NULL;
    }

    if (nvmeq->cehooks) {
        FreeMem(nvmeq->cehooks, sizeof(_NVMEQUEUE_CE_HOOK) * nvmeq->q_depth);
        nvmeq->cehooks = NULL;
    }

    if (nvmeq->cmdid_busy) {
        FreeMem(nvmeq->cmdid_busy, nvmeq->q_depth);
        nvmeq->cmdid_busy = NULL;
    }

    FreeMem(nvmeq, sizeof(*nvmeq));
}
