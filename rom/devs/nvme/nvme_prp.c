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

BOOL nvme_initprp(struct nvme_command *cmdio, struct completionevent_handler *ioehandle, struct nvme_Unit *unit, ULONG len, APTR *data, BOOL is_write)
{
    device_t dev = unit->au_Bus->ab_Dev;
    ULONG page_size;
    ULONG page_mask;
    struct NVMEBase *NVMEBase = dev ? dev->dev_NVMEBase : NULL;
    ULONG first_seg_len = len;
    ULONG dma_flags = is_write ? DMAFLAGS_PREWRITE : DMAFLAGS_PREREAD;
    APTR phys1;
    UQUAD dma_addr1;
    ULONG first_chunk;

    ioehandle->ceh_IOMem.me_Un.meu_Addr = NULL;

    if (!dev || !dev->dev_PCIDriverObject || !NVMEBase) {
        goto fail;
    }

    (void)NVMEBase;

    page_size = dev->pagesize;
    page_mask = page_size - 1;

    phys1 = CachePreDMA(*data, &first_seg_len, dma_flags);
    if (!phys1 || first_seg_len == 0) {
        goto fail;
    }

    if (!nvme_dma_append(ioehandle, *data, first_seg_len, dma_flags)) {
        goto fail;
    }

    dma_addr1 = (UQUAD)(IPTR)HIDD_PCIDriver_CPUtoPCI(dev->dev_PCIDriverObject, phys1);
    first_chunk = MIN(len, page_size - (dma_addr1 & page_mask));
    first_chunk = MIN(first_chunk, first_seg_len);

    cmdio->rw.op.flags &= ~NVME_CMD_PSDT_MASK;
    cmdio->rw.prp1 = AROS_QUAD2LE(dma_addr1);

    if (len <= first_chunk) {
        cmdio->rw.prp2 = 0;
        return TRUE;
    }

    {
        ULONG remaining = len - first_chunk;
        ULONG second_seg_len = remaining;
        APTR next_cpu = (APTR)((UBYTE *)(*data) + first_chunk);
        APTR phys2 = CachePreDMA(next_cpu, &second_seg_len, dma_flags | DMA_Continue);

        if (!phys2 || second_seg_len < remaining) {
            goto fail;
        }

        if (!nvme_dma_append(ioehandle, next_cpu, second_seg_len, dma_flags | DMA_Continue)) {
            goto fail;
        }

        UQUAD dma_addr2 = (UQUAD)(IPTR)HIDD_PCIDriver_CPUtoPCI(dev->dev_PCIDriverObject, phys2);

        if ((dma_addr2 & page_mask) != 0) {
            goto fail;
        }

        if (remaining <= page_size) {
            cmdio->rw.prp2 = AROS_QUAD2LE(dma_addr2);
            return TRUE;
        }

        {
            ULONG entries_needed = (remaining + page_size - 1) / page_size;
            ULONG entries_per_page = page_size / sizeof(UQUAD);
            ULONG entries_left;
            ULONG lists_needed = 0;
            ULONG alloc_bytes;
            ULONG alloc_total;
            UBYTE *prp_storage;
            ULONG list_index;
            ULONG page_index = 0;
            ULONG flush_flags;
            UQUAD first_list_pci = 0;

            if (entries_per_page == 0) {
                goto fail;
            }

            if ((entries_per_page == 1) && (entries_needed > 1)) {
                goto fail;
            }

            entries_left = entries_needed;
            while (entries_left > 0) {
                lists_needed++;
                if (entries_left > entries_per_page) {
                    entries_left -= (entries_per_page - 1);
                } else {
                    entries_left = 0;
                }
            }

            alloc_bytes = lists_needed * page_size;
            alloc_total = alloc_bytes + (page_size - 1);

            ioehandle->ceh_IOMem.me_Length = alloc_total;
            ioehandle->ceh_IOMem.me_Un.meu_Addr = AllocMem(alloc_total, MEMF_ANY | MEMF_CLEAR);
            if (!ioehandle->ceh_IOMem.me_Un.meu_Addr) {
                goto fail;
            }

            prp_storage = (UBYTE *)(((IPTR)ioehandle->ceh_IOMem.me_Un.meu_Addr + page_size - 1) & ~page_mask);

            entries_left = entries_needed;
            for (list_index = 0; list_index < lists_needed; list_index++) {
                UQUAD *list_page = (UQUAD *)(prp_storage + (list_index * page_size));
                BOOL more_lists = entries_left > entries_per_page;
                ULONG capacity = more_lists ? (entries_per_page - 1) : entries_left;
                ULONG slot;

                for (slot = 0; slot < capacity; slot++, page_index++) {
                    list_page[slot] = AROS_QUAD2LE(dma_addr2 + ((UQUAD)page_index * page_size));
                }

                entries_left -= capacity;

                if (more_lists) {
                    UQUAD next_addr = (UQUAD)(IPTR)HIDD_PCIDriver_CPUtoPCI(
                        dev->dev_PCIDriverObject,
                        prp_storage + ((list_index + 1) * page_size));

                    if ((next_addr & page_mask) != 0) {
                        goto fail;
                    }

                    list_page[entries_per_page - 1] = AROS_QUAD2LE(next_addr);
                }
            }

            if (page_index != entries_needed) {
                goto fail;
            }

            flush_flags = DMAFLAGS_PREWRITE;
            for (list_index = 0; list_index < lists_needed; list_index++) {
                UBYTE *page_ptr = prp_storage + (list_index * page_size);
                ULONG page_len = page_size;
                APTR phys_page = CachePreDMA(page_ptr, &page_len, flush_flags);
                UQUAD pci_addr;

                if (!phys_page || page_len < page_size) {
                    goto fail;
                }

                page_len = page_size;

                if (!nvme_dma_append(ioehandle, page_ptr, page_len, flush_flags)) {
                    goto fail;
                }

                pci_addr = (UQUAD)(IPTR)HIDD_PCIDriver_CPUtoPCI(dev->dev_PCIDriverObject, phys_page);

                if ((pci_addr & page_mask) != 0) {
                    goto fail;
                }

                if (list_index == 0) {
                    first_list_pci = pci_addr;
                }

                flush_flags |= DMA_Continue;
            }

            cmdio->rw.prp2 = AROS_QUAD2LE(first_list_pci);
        }
    }

    return TRUE;

fail:
    nvme_dma_release(ioehandle, TRUE);

    if (ioehandle->ceh_IOMem.me_Un.meu_Addr) {
        FreeMem(ioehandle->ceh_IOMem.me_Un.meu_Addr, ioehandle->ceh_IOMem.me_Length);
        ioehandle->ceh_IOMem.me_Un.meu_Addr = NULL;
        ioehandle->ceh_IOMem.me_Length = 0;
    }

    return FALSE;
}
