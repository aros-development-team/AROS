/*
    Copyright © 2004, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <oop/oop.h>

#include <hidd/pci.h>

#include <dos/bptr.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include "ata.h"

#undef LIBBASE
#define LIBBASE (unit->au_Base)
#define SysBase (LIBBASE->ata_SysBase)

/*
    Prepare PRD entries for sectors transfer. This function assumes, that noone
    else will even touch PRD. It should be however truth, as for given bus all
    ATA accesses are protected with a semaphore.
*/
VOID dma_SetupPRD(struct ata_Unit *unit, APTR buffer, ULONG sectors, BOOL io)
{
    struct PRDEntry *prd = unit->au_Bus->ab_PRD;
    IPTR ptr = (IPTR)buffer;
    ULONG size = sectors << unit->au_SectorShift;
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
	If io set to TRUE, then sectors are readed, when set to FALSE, they are written
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

#undef LIBBASE
#define LIBBASE (bus->ab_Base)
#define OOPBase (LIBBASE->ata_OOPBase)

#define bus ((struct ata_Bus*)hook->h_Data)

static
AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *,	hook,	A0),
    AROS_UFHA(OOP_Object *,	Device,	A2),
    AROS_UFHA(APTR,		message,A1))
{
    AROS_USERFUNC_INIT

    UWORD ProductID, VendorID;
    ULONG IOBase;
    OOP_AttrBase __IHidd_PCIDev = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    struct TagItem attrs[] = {
	{ aHidd_PCIDevice_isMaster, TRUE },
	{ TAG_DONE, 0UL}
    };

    D(bug("[ATA.scanbus] got device\n"));
    
    OOP_GetAttr(Device, aHidd_PCIDevice_ProductID, (APTR)&ProductID);
    OOP_GetAttr(Device, aHidd_PCIDevice_VendorID,  (APTR)&VendorID);
    OOP_GetAttr(Device, aHidd_PCIDevice_Base4,  (APTR)&IOBase);

    D(bug("[ATA.scanbus] IDE device %04x:%04x - DMA registers at %x\n",
	ProductID, VendorID, IOBase));

    if (IOBase)
    {
	if (bus->ab_Port == 0x1f0)
	{
	    if (bus->ab_Units[0])
		bus->ab_Units[0]->au_DMAPort = IOBase;
	    if (bus->ab_Units[1])
		bus->ab_Units[1]->au_DMAPort = IOBase;
	}
	else if (bus->ab_Port == 0x170)
	{
	    if (bus->ab_Units[0])
		bus->ab_Units[0]->au_DMAPort = IOBase + 8;
	    if (bus->ab_Units[1])
		bus->ab_Units[1]->au_DMAPort = IOBase + 8;
	}
    }

    D(bug("[ATA] Bus0 status says %02x, Bus1 status says %02x\n",
	inb(IOBase + 2), inb(IOBase + 10)));

    OOP_SetAttrs(Device, attrs);

    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);

    AROS_USERFUNC_EXIT
}


#undef bus

VOID dma_Init(struct ata_Bus *bus)
{
    OOP_Object *pci;

    D(bug("[ATA.DMA] Generic init\n"));

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

    if (pci)
    {
	struct Hook FindHook = {
	    h_Entry:    (IPTR (*)())Enumerator,
	    h_Data:	    bus,
	};

	struct TagItem Requirements[] = {
	    {tHidd_PCI_Class,	    0x01},
	    {tHidd_PCI_SubClass,    0x01},
	    {TAG_DONE,		    0x00}
	};

	struct pHidd_PCI_EnumDevices enummsg = {
	    mID:	    OOP_GetMethodID(CLID_Hidd_PCI, moHidd_PCI_EnumDevices),
	    callback:	    &FindHook,
	    requirements:   (struct TagItem *)&Requirements,
	}, *msg = &enummsg;
	
	OOP_DoMethod(pci, (OOP_Msg)msg);
	
	OOP_DisposeObject(pci);
    }
    
}

