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
ULONG sizePCIBaseReg( UBYTE bus, UBYTE dev, UBYTE func, UBYTE basenum )
{
    ULONG bak,sz;

    bak = readPCIConfigLong(bus,dev,func,(PCICS_BAR0 + (basenum<<2)));
    writePCIConfigLong(bus,dev,func,(PCICS_BAR0 + (basenum<<2)),~0);
    sz = readPCIConfigLong(bus,dev,func,(PCICS_BAR0 + (basenum<<2)));
    writePCIConfigLong(bus,dev,func,(PCICS_BAR0 + (basenum<<2)),bak);

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
