/*
    Copyright © 2004-2012, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>

#include <exec/types.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <dos/bptr.h>

#include <proto/exec.h>

#include "ata.h"

/*
    Prepare PRD entries for sectors transfer. This function assumes that no one
    else will even touch PRD. It should be however truth, as for given bus all
    ATA accesses are protected with a semaphore.
*/
LONG dma_Setup(APTR addr, ULONG len, BOOL read, struct PRDEntry* array)
{
    ULONG tmp = 0, rem = 0;
    ULONG flg = read ? DMA_ReadFromRAM : 0;
    IPTR phy_mem;
    LONG items = 0;

    D(bug("[ATA  ] dma_Setup(addr %p, len %d, PRDEntry  @ %p for %s)\n", addr, len, array, read ? "READ" : "WRITE"));

    /*
     * in future you may have to put this in prd construction procedure
     */
    while (0 < len)
    {
        tmp = len;
        phy_mem = (IPTR)CachePreDMA(addr, &tmp, flg);

        D(bug("[ATA  ] dma_Setup: Translating V:%p > P:%p (%ld bytes)\n", addr, phy_mem, tmp));
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
                bug("[ATA  ] dma_Setup: ERROR: ATA DMA POINTERS BEYOND MAXIMUM ALLOWED ADDRESS!\n");
                return 0;
            }
            if (items > PRD_MAX)
            {
                bug("[ATA  ] dma_Setup: ERROR: ATA DMA PRD TABLE SIZE TOO LARGE\n");
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
            D(bug("[ATA  ] dma_Setup: Inserting into PRD Table: %p / %ld @ %p\n", phy_mem, rem, array));
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
    D(bug("[ATA  ] dma_Setup: PRD Table set - %ld items in total.\n", items));
    
    /*
     * PRD table all set.
     */
    return items;
}


BOOL dma_SetupPRD(struct ata_Unit *unit, APTR buffer, ULONG sectors, BOOL io)
{
   return dma_SetupPRDSize(unit, buffer, sectors << unit->au_SectorShift, io);
}

BOOL dma_SetupPRDSize(struct ata_Unit *unit, APTR buffer, ULONG size, BOOL read)
{
    LONG items = 0;
    IPTR prd_phys;
    ULONG length;

    D(bug("[ATA%02ld] dma_SetupPRDSize(buffer @ %p, %ld bytes, PRD @ %x for %s)\n", unit->au_UnitNum, buffer, size, unit->au_Bus->ab_PRD, read ? "READ" : "WRITE"));

    items = dma_Setup(buffer, size, read, unit->au_Bus->ab_PRD);

    if (0 == items)
        return FALSE;

    length = items * sizeof(struct PRDEntry);

    prd_phys = (IPTR)CachePreDMA(unit->au_Bus->ab_PRD, &length, DMA_ReadFromRAM);

    ATA_OUTL(prd_phys, dma_PRD, unit->au_DMAPort);

    if (read)
        ATA_OUT(DMA_WRITE, dma_Command, unit->au_DMAPort); /* inverse logic */
    else
        ATA_OUT(DMA_READ, dma_Command, unit->au_DMAPort);
    return TRUE;
}

VOID dma_Cleanup(APTR addr, ULONG len, BOOL read)
{
    ULONG tmp = 0;
    ULONG flg = read ? DMA_ReadFromRAM : 0;

    D(bug("[ATA  ] dma_Cleanup(%p, %d bytes)\n", addr, len));

    while (len > 0)
    {
        tmp = len;
        CachePostDMA(addr, &tmp, flg);
        addr = &((UBYTE*)addr)[tmp];
        len -= tmp;
        flg |= DMA_Continue;
    }
}

VOID dma_StartDMA(struct ata_Unit *unit)
{
    D(bug("[ATA%02ld] dma_StartDMA()\n", unit->au_UnitNum));
    ATA_OUT(ATA_IN(dma_Command, unit->au_DMAPort) | DMA_START, dma_Command, unit->au_DMAPort);
}

VOID dma_StopDMA(struct ata_Unit *unit)
{
    D(bug("[ATA%02ld] dma_StopDMA()\n", unit->au_UnitNum));
    ATA_OUT(ATA_IN(dma_Command, unit->au_DMAPort) & ~DMA_START, dma_Command, unit->au_DMAPort);
}

