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
    prd[0].prde_Address = ptr;

    /*
	All other PRD addresses are the next 64K pages, until the page address
	is bigger as the highest address used
    */
    for (i=1; i < PRD_MAX; i++)
    {
	prd[i].prde_Address = (prd[i-1].prde_Address & 0xffff0000) + 0x00010000;
	prd[i].prde_Length = 0;
	if (prd[i].prde_Address > ptr + size)
	    break;
    }

    if (size <= prd[1].prde_Address - prd[0].prde_Address)
    {
	prd[0].prde_Length = size;
	size = 0;
    }
    else
    {
	prd[0].prde_Length = prd[1].prde_Address - prd[0].prde_Address;
	size -= prd[0].prde_Length;
    }
    
    prd[0].prde_Length &= 0x0000ffff;

    i = 1;

    while(size >= 65536)
    {
	prd[i].prde_Length = 0;	    /* 64KB in one PRD */
	size -= 65536;
	i++;
    }

    if (size > 0)
    {
	prd[i].prde_Length = size;
	i++;
    }

    prd[i-1].prde_Length |= 0x80000000;

    outl((ULONG)prd, unit->au_DMAPort + dma_PRD);
    outb(inb(unit->au_DMAPort + dma_Status) | DMAF_Error | DMAF_Interrupt, unit->au_DMAPort + dma_Status);
    
    /*
	If io set to TRUE, then sectors are read, when set to FALSE, they are written
    */
    if (io)
	outb(DMA_WRITE, unit->au_DMAPort + dma_Command);
    else
	outb(DMA_READ, unit->au_DMAPort + dma_Command);
}

VOID dma_StartDMA(struct ata_Unit *unit)
{
    inb(unit->au_DMAPort + dma_Command);
    inb(unit->au_DMAPort + dma_Status);
    outb(inb(unit->au_DMAPort + dma_Command) | DMA_START, unit->au_DMAPort + dma_Command);
    inb(unit->au_DMAPort + dma_Command);
    inb(unit->au_DMAPort + dma_Status);
}

VOID dma_StopDMA(struct ata_Unit *unit)
{
    inb(unit->au_DMAPort + dma_Command);
    inb(unit->au_DMAPort + dma_Status);
    outb(inb(unit->au_DMAPort) & ~DMA_START, unit->au_DMAPort + dma_Command);
    inb(unit->au_DMAPort + dma_Command);
    inb(unit->au_DMAPort + dma_Status);
    outb(inb(unit->au_DMAPort + dma_Status) | DMAF_Error | DMAF_Interrupt, unit->au_DMAPort + dma_Status);
}

