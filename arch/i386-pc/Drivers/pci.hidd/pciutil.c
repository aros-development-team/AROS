/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI utility functions
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <asm/io.h>

#include "pci.h"
#include "pciutil.h"

#undef	SDEBUG
#undef	DEBUG
#define DEBUG 1
#include <aros/debug.h>

/*
  Fetch text descriptions of the different PCI device classes
*/
void PCIGetClassDesc( UBYTE class, UBYTE sub, UBYTE prgif, STRPTR *cdesc, STRPTR *sdesc, STRPTR *pdesc )
{
    ULONG i;

    *cdesc = *sdesc = *pdesc = "";

    for (i=0;i < PCI_CLASSTABLE_LEN; i++)
    {
        if (PCI_ClassTable[i].Baseclass == class)
        {
            if ( !(**cdesc))
            {
                *cdesc = PCI_ClassTable[i].Basedesc;
            }
            if (PCI_ClassTable[i].Subclass == sub)
            {
                if ( !(**sdesc))
                {
                    *sdesc = PCI_ClassTable[i].Subdesc;
                }
                if (PCI_ClassTable[i].Prgif == prgif)
                {
                    if ( !(**pdesc))
                    {
                        *pdesc = PCI_ClassTable[i].Prgifdesc;
                    }
                }
            }
        }
    }
}

/*
 * Size a base register
 * Return size of the base register area
 */
ULONG PCISizeBase( UBYTE bus, UBYTE dev, UBYTE func, UBYTE basenum )
{
    ULONG bak,sz;

    bak = PCIReadConfigLong(bus,dev,func,(PCICS_BAR + (basenum<<2)));
    PCIWriteConfigLong(bus,dev,func,(PCICS_BAR + (basenum<<2)),~0);
    sz = PCIReadConfigLong(bus,dev,func,(PCICS_BAR + (basenum<<2)));
    PCIWriteConfigLong(bus,dev,func,(PCICS_BAR + (basenum<<2)),bak);

    if ((sz & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_IO)
    {
	/* This is an IO range */
	sz &= PCIBAR_MASK_IO;
	sz = ~sz;
	sz++;
    }
    else
    {
	/* This is memory mapped */
	sz &= PCIBAR_MASK_MEM;
	sz = ~sz;
	sz++;
    }
    return (sz);
}

/*
 * Dump a PCI device to debug console
 * Useful for diagnostics
 */
void PCIDumpDev( struct PCIDevInfo *di)
{
    STRPTR cDesc, pDesc, sDesc;
    UWORD tmp;

    
    PCIGetClassDesc(di->Class,di->SubClass,di->ProgIF,&cDesc,&pDesc,&sDesc);
    D(bug("PCI: Dumping device %02x:%02x.%02x\n",di->PCIBus,di->PCIDev,di->PCIFunc));
    D(bug("     PCI Class: %s %s %s\n",cDesc,pDesc,sDesc));
    D(bug("     Vendor : %04x",di->Vendor));
    D(bug("     Product: %04x\n",di->Product));
    D(bug("     Status register: "));
    tmp = PCIReadConfigWord(di->PCIBus,di->PCIDev,di->PCIFunc,PCICS_STATUS);
    if (tmp & PCISTF_CAPABILITES) D(bug("Capabilites, "));
    if (tmp & PCISTF_66MHZ) D(bug("66MHz, "));
    if (tmp & PCISTF_FASTB2B) D(bug("Fast back-to-back, "));
    D(bug("\n"));
};
