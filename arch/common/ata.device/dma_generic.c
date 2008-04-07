/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved
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
    Prepare PRD entries for sectors transfer. This function assumes, that noone
    else will even touch PRD. It should be however truth, as for given bus all
    ATA accesses are protected with a semaphore.
*/
VOID dma_SetupPRD(struct ata_Unit *unit, APTR buffer, ULONG sectors, BOOL io)
{
   dma_SetupPRDSize(unit, buffer, sectors << unit->au_SectorShift, io);
}

VOID dma_SetupPRDSize(struct ata_Unit *unit, APTR buffer, ULONG size, BOOL io)
{
    struct PRDEntry *prd = unit->au_Bus->ab_PRD;
    IPTR ptr = (IPTR)buffer;
    int i;

    D(bug("[DMA] Setup PRD for %d bytes at %x\n",
	size, ptr));

    /* 
	The first PRD address is the buffer pointer self, doesn't have to be 
	aligned to 64K boundary
    */
    prd[0].prde_Address = AROS_LONG2LE(ptr);

    /*
	All other PRD addresses are the next 64K pages, until the page address
	is bigger as the highest address used
    */
    for (i=1; i < PRD_MAX; i++)
    {
	prd[i].prde_Address = AROS_LONG2LE((AROS_LE2LONG(prd[i-1].prde_Address) & 0xffff0000) + 0x00010000);
	prd[i].prde_Length = 0;
	if (AROS_LE2LONG(prd[i].prde_Address) > ptr + size)
	    break;
    }

    if (size <= AROS_LE2LONG(prd[1].prde_Address) - AROS_LE2LONG(prd[0].prde_Address))
    {
	prd[0].prde_Length = AROS_LONG2LE(size);
	size = 0;
    }
    else
    {
	prd[0].prde_Length = AROS_LONG2LE(AROS_LE2LONG(prd[1].prde_Address) - AROS_LE2LONG(prd[0].prde_Address));
	size -= AROS_LE2LONG(prd[0].prde_Length);
    }
    
    prd[0].prde_Length &= AROS_LONG2LE(0x0000ffff);

    i = 1;

    while(size >= 65536)
    {
	prd[i].prde_Length = 0;	    /* 64KB in one PRD */
	size -= 65536;
	i++;
    }

    if (size > 0)
    {
	prd[i].prde_Length = AROS_LONG2LE(size);
	i++;
    }

    prd[i-1].prde_Length |= AROS_LONG2LE(0x80000000);

    CacheClearE(&prd[0], (i) * sizeof(struct PRDEntry), CACRF_ClearD);
    
    ata_outl((ULONG)prd, dma_PRD, unit->au_DMAPort);
    ata_out(ata_in(dma_Status, unit->au_DMAPort) | DMAF_Error | DMAF_Interrupt, dma_Status, unit->au_DMAPort);
    
    /*
	If io set to TRUE, then sectors are read, when set to FALSE, they are written
    */
    if (io)
	ata_out(DMA_WRITE, dma_Command, unit->au_DMAPort);
    else
	ata_out(DMA_READ, dma_Command, unit->au_DMAPort);
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

