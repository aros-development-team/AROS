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
#include "nvme_hw.h"

/*
    Generic Queue Support Functions
*/

struct nvme_queue *nvme_alloc_queue(device_t dev, int qid, int depth, int vector)
{
    struct NVMEBase *NVMEBase = dev->dev_NVMEBase;;
    unsigned extra = 0; //DIV_ROUND_UP(depth, 8) + (depth *
					//	sizeof(struct nvme_cmd_info));
    struct nvme_queue *nvmeq;
    
    D(bug ("[NVME:QUEUE] %s(0x%p, %u, %u, %d)\n", __func__, dev, qid, depth, vector);)

    nvmeq = AllocMem(sizeof(struct nvme_queue) + extra, MEMF_CLEAR);
    D(bug ("[NVME:QUEUE] %s: queue allocated @ 0x%p (depth %u)\n", __func__, nvmeq, depth);)
    if (nvmeq)
    {
        /* completion queue ... */
        nvmeq->cqba = HIDD_PCIDriver_AllocPCIMem(dev->dev_PCIDriverObject, depth * sizeof(struct nvme_completion));
        D(bug ("[NVME:QUEUE] %s:       completion @ 0x%p\n", __func__, nvmeq->cqba);)
        if (nvmeq->cqba)
        {
            memset((void *)nvmeq->cqba, 0, depth * sizeof(struct nvme_completion));

            /* submission queue ... */
            nvmeq->sqba = HIDD_PCIDriver_AllocPCIMem(dev->dev_PCIDriverObject, depth * sizeof(struct nvme_command));
            D(bug ("[NVME:QUEUE] %s:       cmd submission @ 0x%p\n", __func__, nvmeq->sqba);)
            if (nvmeq->sqba)
            {
                nvmeq->dev = dev;

#if defined(__AROSEXEC_SMP__)
                KrnSpinInit(&nvmeq->q_lock);
#endif
                nvmeq->cq_head = 0;
                nvmeq->cq_phase = 1;

                nvmeq->q_db = &dev->dbs[qid << (dev->db_stride + 1)];
                nvmeq->q_depth = depth;
                D(bug ("[NVME:QUEUE] %s:       doorbells @ 0x%p\n", __func__, nvmeq->q_db);)
                nvmeq->cq_vector = vector;
            }
        }
    }
    return nvmeq;
}
