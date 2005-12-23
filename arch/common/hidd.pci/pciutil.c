/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI utility functions
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <hidd/pci.h>

#include "pci.h"
#include "pciutil.h"

#undef	SDEBUG
#undef	DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*
  Fetch text descriptions of the different PCI device classes
*/
void getPCIClassDesc( UBYTE class, UBYTE sub, UBYTE prgif, STRPTR *cdesc, STRPTR *sdesc, STRPTR *pdesc )
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
ULONG sizePCIBaseReg(OOP_Object *driver, struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE func, UBYTE basenum )
{
    ULONG bak,sz;
    struct pHidd_PCIDriver_ReadConfigLong msgr;
    struct pHidd_PCIDriver_WriteConfigLong msgw;
    
    if (!psd->mid_RL) psd->mid_RL 
	    = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigLong);
    if (!psd->mid_WL) psd->mid_WL 
	    = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_WriteConfigLong);

    msgr.mID = psd->mid_RL;
    msgr.bus = bus;
    msgr.dev = dev;
    msgr.sub = func;
    msgr.reg = PCICS_BAR0 + (basenum << 2);

    msgw.mID = psd->mid_WL;
    msgw.bus = bus;
    msgw.dev = dev;
    msgw.sub = func;
    msgw.reg = PCICS_BAR0 + (basenum << 2);

    bak = OOP_DoMethod(driver, (OOP_Msg)&msgr);
    msgw.val = ~0;
    OOP_DoMethod(driver, (OOP_Msg)&msgw);
    sz = OOP_DoMethod(driver, (OOP_Msg)&msgr);
    msgw.val = bak;
    OOP_DoMethod(driver, (OOP_Msg)&msgw);

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
