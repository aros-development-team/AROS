/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved
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

#if (AROS_BIG_ENDIAN != 0)
#define SWAP_LE_QUAD(x) (x) = AROS_QUAD2LE(x)
#else
#define SWAP_LE_QUAD(x)
#endif

typedef struct nvme_prp_entry {
    union {
        struct {
            UQUAD offset : 12;
            UQUAD pagestart : 52;
        };
        UQUAD raw;
    };
} nvme_prp_entry_t;

BOOL nvme_initprp(struct nvme_command *cmdio, struct completionevent_handler *ioehandle, struct nvme_Unit *unit, ULONG len, APTR *data, BOOL is_write)
{
    nvme_prp_entry_t *prp1 = (APTR)&cmdio->rw.prp1;
    UQUAD prp1_page = (IPTR)*data & ~(unit->au_Bus->ab_Dev->pagesize - 1);
    UWORD prp1_offset = (IPTR)*data & (unit->au_Bus->ab_Dev->pagesize - 1);
    ULONG prp1_len;
    
    DPRP(bug("[NVME%02ld] %s(%p, %u)\n", unit->au_UnitNum, __func__, *data, len);)

    // Set up PRP1
    prp1->pagestart = prp1_page >> unit->au_Bus->ab_Dev->pageshift;
    prp1->offset = prp1_offset;
    DPRP(bug("[NVME%02ld] %s: prp1 %p = %015x:%02x\n", unit->au_UnitNum, __func__, *data, prp1->pagestart, prp1->offset);)
    SWAP_LE_QUAD(prp1->raw);
    
    prp1_len = unit->au_Bus->ab_Dev->pagesize - prp1_offset;

    DPRP(bug("[NVME%02ld] %s: prp1 data len %u\n", unit->au_UnitNum, __func__, prp1_len);)

    // Check if we need to use PRP2
    if (len > prp1_len) {
        UQUAD next_addr = (IPTR)*data + prp1_len;

        DPRP(bug("[NVME%02ld] %s: using prp2 for %p\n", unit->au_UnitNum, __func__, next_addr);)

        UQUAD prp2_page = next_addr & ~(unit->au_Bus->ab_Dev->pagesize - 1);
        UWORD prp2_offset = next_addr & (unit->au_Bus->ab_Dev->pagesize - 1);
        ULONG prp2_len;

        nvme_prp_entry_t *prp2 = (APTR)&cmdio->rw.prp2;
        prp2->pagestart = prp2_page >> unit->au_Bus->ab_Dev->pageshift;
        prp2->offset = prp2_offset;

        prp2_len = unit->au_Bus->ab_Dev->pagesize - prp2_offset;

        DPRP(bug("[NVME%02ld] %s: prp2 data len %u\n", unit->au_UnitNum, __func__, prp2_len);)

        // Check if a PRP list is needed
        if (len > (prp1_len + prp2_len))
        {
            ULONG num_prps = ((len - (prp1_len + prp2_len) + unit->au_Bus->ab_Dev->pagesize - 1) / unit->au_Bus->ab_Dev->pagesize) + 1;
            int prpblocks, prpentry, prp = 0, prpperpage = (unit->au_Bus->ab_Dev->pagesize / sizeof(nvme_prp_entry_t));

            prpblocks = ((num_prps - 1) / prpperpage) + 1;
            DPRP(bug("[NVME%02ld] %s: prp list needed for %u entries(s) in %u prp page(s)\n", unit->au_UnitNum, __func__, num_prps, prpblocks);)

            ioehandle->ceh_IOMem.me_Length = unit->au_Bus->ab_Dev->pagesize + (num_prps + prpblocks) * sizeof(nvme_prp_entry_t);
            if ((ioehandle->ceh_IOMem.me_Un.meu_Addr = AllocMem(ioehandle->ceh_IOMem.me_Length, MEMF_ANY)) != NULL)
            {
                nvme_prp_entry_t *prplist = (APTR)(((IPTR)ioehandle->ceh_IOMem.me_Un.meu_Addr + unit->au_Bus->ab_Dev->pagesize) & ~(unit->au_Bus->ab_Dev->pagesize - 1));
                UQUAD curr_addr, curr_pagestart;

                DPRP(
                    bug("[NVME%02ld] %s: allocated prplist storage @ 0x%p (%u bytes)\n", unit->au_UnitNum, __func__, ioehandle->ceh_IOMem.me_Un.meu_Addr, ioehandle->ceh_IOMem.me_Length);
                    bug("[NVME%02ld] %s: prplist @ 0x%p\n", unit->au_UnitNum, __func__, prplist);
                )

                // Populate PRP list
                for (prpentry = 0; prpentry < (num_prps + prpblocks - 1); prpentry++)
                {
                    if ((prpblocks > 1) && (prpentry < (num_prps + prpblocks - 2)) &&
                        (prpentry > 0) && (((prpentry + 1) % prpperpage) == 0))
                    {
                        curr_addr = (UQUAD)(IPTR)&prplist[prpentry + 1].raw;
                        curr_pagestart = curr_addr & ~(unit->au_Bus->ab_Dev->pagesize - 1);
                        prplist[prpentry].pagestart =
                            curr_pagestart >> unit->au_Bus->ab_Dev->pageshift;
                        prplist[prpentry].offset = 0;
                        DPRP(bug("[NVME%02ld] %s: # prplist[%u] = %015x:%02x\n", unit->au_UnitNum, __func__, prpentry, prplist[prpentry].pagestart, prplist[prpentry].offset);)
                    }
                    else
                    {
                        curr_addr = next_addr + prp * unit->au_Bus->ab_Dev->pagesize;
                        curr_pagestart = curr_addr & ~(unit->au_Bus->ab_Dev->pagesize - 1);
                        prplist[prpentry].pagestart =
                            curr_pagestart >> unit->au_Bus->ab_Dev->pageshift;
                        prplist[prpentry].offset = 0;
                        prp++;
                        DPRP(bug("[NVME%02ld] %s:   prplist[%u] = %015x:%02x\n", unit->au_UnitNum, __func__, prpentry, prplist[prpentry].pagestart, prplist[prpentry].offset);)
                    }
                    SWAP_LE_QUAD(prplist[prpentry].raw);
                }

                // Point PRP2 to the PRP list
                prp2->pagestart = (IPTR)prplist >> unit->au_Bus->ab_Dev->pageshift;
                prp2->offset = 0;
#if (0)
                ULONG dmalen = (num_prps + prpblocks - 1) << 3;
                CachePreDMA(prplist, &dmalen, DMAFLAGS_PREREAD);
#endif
            }
            else
            {
                bug("[NVME%02ld] %s: failed to allloc storage for prplist!\n", unit->au_UnitNum, __func__);
                return FALSE;
            }
        }
        DPRP(bug("[NVME%02ld] %s: prp2 %015x:%02x\n", unit->au_UnitNum, __func__, prp2->pagestart, prp2->offset);)
        SWAP_LE_QUAD(prp2->raw);
    }
    return TRUE;
}
