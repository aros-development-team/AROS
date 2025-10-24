/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved
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

#ifndef DMA_Continue
#define DMA_Continue    (1L << 1)
#endif

#define NVME_CMD_PSDT_MASK      (3 << 6)
#define NVME_CMD_PSDT_SGL       (2 << 6)
#define NVME_SGL_DESC_TYPE_DATA_BLOCK   0x00

struct nvme_sgl_descriptor {
    UQUAD address;
    ULONG length;
    UBYTE rsvd[3];
    UBYTE type;
};

BOOL nvme_initsgl(struct nvme_command *cmdio, struct completionevent_handler *ioehandle, struct nvme_Unit *unit, ULONG len, APTR *data, BOOL is_write)
{
    device_t dev = unit->au_Bus->ab_Dev;
    ULONG page_size;
    ULONG max_segments;
    ULONG capacity;
    ULONG alloc_len;
    struct nvme_sgl_descriptor *sgl;
    ULONG remaining = len;
    UBYTE *cpu_ptr = (UBYTE *)*data;
    ULONG flags = is_write ? DMAFLAGS_PREWRITE : DMAFLAGS_PREREAD;
    ULONG descriptor_count = 0;
    struct NVMEBase *NVMEBase = dev ? dev->dev_NVMEBase : NULL;

    if (!dev || !dev->dev_PCIDriverObject || !NVMEBase) {
        return FALSE;
    }

    (void)NVMEBase;

    page_size = dev->pagesize;

    if (len == 0) {
        return FALSE;
    }

    max_segments = (len + page_size - 1) / page_size;
    if (max_segments == 0) {
        max_segments = 1;
    }

    capacity = max_segments;
    alloc_len = (capacity * sizeof(struct nvme_sgl_descriptor)) + 16;
    ioehandle->ceh_IOMem.me_Length = alloc_len;
    ioehandle->ceh_IOMem.me_Un.meu_Addr = AllocMem(alloc_len, MEMF_ANY | MEMF_CLEAR);
    if (!ioehandle->ceh_IOMem.me_Un.meu_Addr) {
        return FALSE;
    }

    sgl = (struct nvme_sgl_descriptor *)(((IPTR)ioehandle->ceh_IOMem.me_Un.meu_Addr + 15) & ~15);

    while (remaining > 0) {
        ULONG chunk = remaining;
        APTR phys = CachePreDMA(cpu_ptr, &chunk, flags);

        if (!phys || chunk == 0) {
            FreeMem(ioehandle->ceh_IOMem.me_Un.meu_Addr, ioehandle->ceh_IOMem.me_Length);
            ioehandle->ceh_IOMem.me_Un.meu_Addr = NULL;
            return FALSE;
        }

        if (!nvme_dma_append(ioehandle, cpu_ptr, chunk, flags)) {
            FreeMem(ioehandle->ceh_IOMem.me_Un.meu_Addr, ioehandle->ceh_IOMem.me_Length);
            ioehandle->ceh_IOMem.me_Un.meu_Addr = NULL;
            return FALSE;
        }

        if (descriptor_count >= capacity) {
            ULONG new_capacity = capacity << 1;
            if (new_capacity <= capacity) {
                FreeMem(ioehandle->ceh_IOMem.me_Un.meu_Addr, ioehandle->ceh_IOMem.me_Length);
                ioehandle->ceh_IOMem.me_Un.meu_Addr = NULL;
                return FALSE;
            }
            ULONG new_alloc = (new_capacity * sizeof(struct nvme_sgl_descriptor)) + 16;
            APTR new_storage = AllocMem(new_alloc, MEMF_ANY | MEMF_CLEAR);

            if (!new_storage) {
                FreeMem(ioehandle->ceh_IOMem.me_Un.meu_Addr, ioehandle->ceh_IOMem.me_Length);
                ioehandle->ceh_IOMem.me_Un.meu_Addr = NULL;
                return FALSE;
            }

            struct nvme_sgl_descriptor *new_sgl = (struct nvme_sgl_descriptor *)(((IPTR)new_storage + 15) & ~15);

            CopyMem(sgl, new_sgl, descriptor_count * sizeof(struct nvme_sgl_descriptor));

            FreeMem(ioehandle->ceh_IOMem.me_Un.meu_Addr, ioehandle->ceh_IOMem.me_Length);

            ioehandle->ceh_IOMem.me_Un.meu_Addr = new_storage;
            ioehandle->ceh_IOMem.me_Length = new_alloc;

            sgl = new_sgl;
            capacity = new_capacity;
        }

        sgl[descriptor_count].address = AROS_QUAD2LE((UQUAD)(IPTR)HIDD_PCIDriver_CPUtoPCI(dev->dev_PCIDriverObject, phys));
        sgl[descriptor_count].length = AROS_LONG2LE(chunk);
        sgl[descriptor_count].rsvd[0] = sgl[descriptor_count].rsvd[1] = sgl[descriptor_count].rsvd[2] = 0;
        sgl[descriptor_count].type = NVME_SGL_DESC_TYPE_DATA_BLOCK;
        descriptor_count++;

        remaining -= chunk;
        cpu_ptr += chunk;
        flags = (is_write ? DMAFLAGS_PREWRITE : DMAFLAGS_PREREAD) | DMA_Continue;
    }

    {
        ULONG sgl_bytes = descriptor_count * sizeof(struct nvme_sgl_descriptor);
        APTR sgl_phys = CachePreDMA(sgl, &sgl_bytes, DMAFLAGS_PREWRITE);

        if (!sgl_phys || sgl_bytes < descriptor_count * sizeof(struct nvme_sgl_descriptor)) {
            FreeMem(ioehandle->ceh_IOMem.me_Un.meu_Addr, ioehandle->ceh_IOMem.me_Length);
            ioehandle->ceh_IOMem.me_Un.meu_Addr = NULL;
            return FALSE;
        }

        if (!nvme_dma_append(ioehandle, sgl, descriptor_count * sizeof(struct nvme_sgl_descriptor), DMAFLAGS_PREWRITE)) {
            FreeMem(ioehandle->ceh_IOMem.me_Un.meu_Addr, ioehandle->ceh_IOMem.me_Length);
            ioehandle->ceh_IOMem.me_Un.meu_Addr = NULL;
            return FALSE;
        }

        cmdio->rw.op.flags = (cmdio->rw.op.flags & ~NVME_CMD_PSDT_MASK) | NVME_CMD_PSDT_SGL;
        cmdio->rw.prp1 = AROS_QUAD2LE((UQUAD)(IPTR)HIDD_PCIDriver_CPUtoPCI(dev->dev_PCIDriverObject, sgl_phys));
        cmdio->rw.prp2 = 0;
    }

    return TRUE;
}
