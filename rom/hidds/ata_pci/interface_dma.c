/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved
    $Id$

    Desc: Generic PCI-DMA ATA controller driver
    Lang: English
*/

#include <aros/debug.h>
#include <devices/scsidisk.h>
#include <exec/exec.h>
#include <proto/exec.h>

#include "interface_dma.h"

/*
 * Prepare PRD entries for sectors transfer. This function assumes that no one
 * else will even touch PRD. It should be however truth, as for given bus all
 * ATA accesses are protected with a semaphore.
 */
static LONG dma_Setup(APTR addr, ULONG len, BOOL read, struct PRDEntry* array)
{
    ULONG tmp = 0, rem = 0;
    ULONG flg = read ? DMA_ReadFromRAM : 0;
    IPTR phy_mem;
    LONG items = 0;

    D(bug("[PCI-ATA] dma_Setup(addr %p, len %d, PRDEntry  @ %p for %s)\n", addr, len, array, read ? "READ" : "WRITE"));

    /*
     * in future you may have to put this in prd construction procedure
     */
    while (0 < len)
    {
        tmp = len;
        phy_mem = (IPTR)CachePreDMA(addr, &tmp, flg);

        D(bug("[PCI-ATA] dma_Setup: Translating V:%p > P:%p (%ld bytes)\n", addr, phy_mem, tmp));
        /*
         * update all addresses for the next call
         */
        addr = &((UBYTE*)addr)[tmp];
        len -= tmp;
        flg |= DMA_Continue;

        /* 
         * check if we're crossing the magic 64k boundary:
         */
        while (0 < tmp)
        {
            /*
             * politely say what sucks
             */
            if (phy_mem > 0xffffffffull)
            {
                D(bug("[PCI-ATA] dma_Setup: ERROR: ATA DMA POINTERS BEYOND MAXIMUM ALLOWED ADDRESS!\n"));
                return 0;
            }
            if (items > PRD_MAX)
            {
                D(bug("[PCI-ATA] dma_Setup: ERROR: ATA DMA PRD TABLE SIZE TOO LARGE\n"));
                return 0;
            }

            /*
             * calculate remainder and see if it is larger of the current memory block.
             * if smaller, adjust its size.
             */
            rem = 0x10000 - (phy_mem & 0xffff);
            if (rem > tmp)
                rem = tmp;
            /*
             * update PRD with address and remainder
             */
            D(bug("[PCI-ATA] dma_Setup: Inserting into PRD Table: %p / %d @ %p\n", phy_mem, rem, array));
            array->prde_Address = AROS_LONG2LE(phy_mem);
            array->prde_Length  = AROS_LONG2LE((rem & 0xffff));
            ++array;
            ++items;

            /*
             * update locals ;-)
             */
            phy_mem += rem;
            tmp -= rem;
        }
    }

    if (items > 0)
    {
        --array;
        array->prde_Length |= AROS_LONG2LE(PRDE_EOT);
    }
    D(bug("[PCI-ATA] dma_Setup: PRD Table set - %u items in total.\n", items));

    /*
     * PRD table all set.
     */
    return items;
}

BOOL dma_SetupPRDSize(struct dma_data *unit, APTR buffer, IPTR size, BOOL read)
{
    LONG items = 0;
    IPTR prd_phys;
    ULONG length;

    items = dma_Setup(buffer, size, read, unit->ab_PRD);

    if (0 == items)
        return FALSE;

    length = items * sizeof(struct PRDEntry);

    prd_phys = (IPTR)CachePreDMA(unit->ab_PRD, &length, DMA_ReadFromRAM);

    outl(prd_phys, dma_PRD + unit->au_DMAPort);
    outb(read ? DMA_WRITE : DMA_READ, dma_Command + unit->au_DMAPort); /* inverse logic */

    return TRUE;
}

void dma_Cleanup(struct dma_data *unit, APTR addr, IPTR len, BOOL read)
{
    ULONG tmp, flg;
    port_t port = dma_Command + unit->au_DMAPort;

    /* Stop DMA engine */
    outb(inb(port) & ~DMA_START, port);

    flg = read ? DMA_ReadFromRAM : 0;
    while (len > 0)
    {
        tmp = len;
        CachePostDMA(addr, &tmp, flg);
        addr = &((UBYTE*)addr)[tmp];
        len -= tmp;
        flg |= DMA_Continue;
    }
}

VOID dma_StartDMA(struct dma_data *unit)
{
    port_t port = dma_Command + unit->au_DMAPort;

    outb(inb(port) | DMA_START, port);
}

ULONG dma_CheckErr(struct dma_data *unit)
{
    UBYTE stat = inb(dma_Status + unit->au_DMAPort);

    return (stat & DMAF_Error) ? HFERR_DMA : 0;
}

const APTR dma_FuncTable[]=
{
    dma_SetupPRDSize,
    dma_StartDMA,
    dma_Cleanup,
    dma_CheckErr,
    (APTR)-1
};
