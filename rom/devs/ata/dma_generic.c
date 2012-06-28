/*
    Copyright © 2004-2011, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/
/*
 * PARTIAL CHANGELOG:
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
 * 2011-05-19  P. Fedin		   The Big rework. Separated bus-specific code. Made 64-bit-friendly.
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

