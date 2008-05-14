/*
    Copyright © 2004-2008, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/
/*
 * CHANGELOG:
 * DATE        NAME                ENTRY
 * ----------  ------------------  -------------------------------------------------------------------
 * 2008-01-25  T. Wiszkowski       Rebuilt, rearranged and partially fixed 60% of the code here
 *                                 Enabled implementation to scan for other PCI IDE controllers
 *                                 Implemented ATAPI Packet Support for both read and write
 *                                 Corrected ATAPI DMA handling                            
 *                                 Fixed major IDE enumeration bugs severely handicapping transfers with more than one controller
 *                                 Compacted source and implemented major ATA support procedure
 *                                 Improved DMA and Interrupt management
 *                                 Removed obsolete code
 * 2008-04-03  M. Schulz           inb, outb and outl are not used directly anymore. Instead, the ata_* macros are taken.
 *                                 PRD should be set in little endian mode up (at least I guess so...)
 * 2008-04-07  M. Schulz           Once PRD is ready one has to clear data caches. PRD might still be in cache only on
 *                                 writeback systems otherwise 
 */

#define DEBUG 0
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
LONG dma_Setup(APTR adr, ULONG len, BOOL read, struct PRDEntry* array)
{
    ULONG tmp = 0, rem = 0;
    ULONG flg = read ? DMA_ReadFromRAM : 0;
    IPTR phy_mem;
    LONG items = 0;

    /*
     * in future you may have to put this in prd construction procedure
     */
    while (0 < len)
    {
        tmp = len;
        phy_mem = (IPTR)CachePreDMA(adr, &tmp, flg);

        D(bug("[ATA  ] DMA: Translating V:%08lx > P:%08lx (%ld bytes)\n", adr, phy_mem, tmp));
        /*
         * update all addresses for the next call
         */
        adr = &((UBYTE*)adr)[tmp];
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
                bug("[ATA  ] DMA ERROR: ATA DMA POINTERS BEYOND MAXIMUM ALLOWED ADDRESS!\n");
                return 0;
            }
            if (items > PRD_MAX)
            {
                bug("[ATA  ] DMA ERROR: ATA DMA PRD TABLE SIZE TOO LARGE\n");
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
            D(bug("[ATA  ] DMA: Inserting into PRD Table: %08lx / %ld @ %08lx\n", phy_mem, rem, array));
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
    D(bug("[ATA  ] DMA: PRD Table set - %ld items in total.\n", items));
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
    D(bug("[DMA] Setup PRD for %08lx/%ld bytes at %x for %s\n", buffer, size, unit->au_Bus->ab_PRD, read ? "read" : "write"));
    items = dma_Setup(buffer, size, read, unit->au_Bus->ab_PRD);

    if (0 == items)
        return FALSE;

    CacheClearE(unit->au_Bus->ab_PRD, items * sizeof(struct PRDEntry), CACRF_ClearD);
    
    ata_outl((ULONG)unit->au_Bus->ab_PRD, dma_PRD, unit->au_DMAPort);
    ata_out(ata_in(dma_Status, unit->au_DMAPort) | DMAF_Error | DMAF_Interrupt, dma_Status, unit->au_DMAPort);
    
    if (read)
        ata_out(DMA_WRITE, dma_Command, unit->au_DMAPort); /* inverse logic */
    else
        ata_out(DMA_READ, dma_Command, unit->au_DMAPort);
    return TRUE;
}

VOID dma_Cleanup(APTR adr, ULONG len, BOOL read)
{
    ULONG tmp = 0;
    ULONG flg = read ? DMA_ReadFromRAM : 0;

    while (len > 0)
    {
        tmp = len;
        CachePostDMA(adr, &tmp, flg);
        adr = &((UBYTE*)adr)[tmp];
        len -= tmp;
        flg |= DMA_Continue;
    }
}

VOID dma_StartDMA(struct ata_Unit *unit)
{
    ata_in(dma_Command, unit->au_DMAPort);
    ata_in(dma_Status, unit->au_DMAPort);
    ata_out(ata_in(dma_Command, unit->au_DMAPort) | DMA_START, dma_Command, unit->au_DMAPort);
    ata_in(dma_Command, unit->au_DMAPort);
    ata_in(dma_Status, unit->au_DMAPort);
}

VOID dma_StopDMA(struct ata_Unit *unit)
{
    ata_in(dma_Command, unit->au_DMAPort);
    ata_in(dma_Status, unit->au_DMAPort);
    ata_out(ata_in(dma_Command, unit->au_DMAPort) & ~DMA_START, dma_Command, unit->au_DMAPort);
    ata_in(dma_Command, unit->au_DMAPort);
    ata_in(dma_Status, unit->au_DMAPort);
    ata_out(ata_in(dma_Status, unit->au_DMAPort) | DMAF_Error | DMAF_Interrupt, dma_Status, unit->au_DMAPort);
}

