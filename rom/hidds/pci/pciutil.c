/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc: PCI utility functions
*/

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <hidd/pci.h>
#include <hardware/pci.h>

#include "pci.h"
#include "pciutil.h"

#undef HiddPCIDriverBase
#define HiddPCIDriverBase (psd->hiddPCIDriverMB)

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
IPTR sizePCIBaseReg(OOP_Object *driver, struct pci_staticdata *psd, struct DeviceData *device, UBYTE bus, UBYTE dev, UBYTE func, UBYTE basenum )
{
    ULONG bak,sz;
    ULONG bakhi,szhi = 0;
    BOOL is64;
    UWORD cmd;

    /*
     * Sizing writes all-ones to the BAR, so turn decoding off meanwhile,
     * as Linux does. Host bridges are left alone.
     */
    cmd = HIDD_PCIDriver_ReadConfigWord(driver, (OOP_Object *)device, bus, dev, func, PCICS_COMMAND);
    if (device->Class == 0x06 && device->SubClass == 0x00)
        cmd = 0;
    if (cmd & (PCICMF_IODECODE | PCICMF_MEMDECODE))
        HIDD_PCIDriver_WriteConfigWord(driver, (OOP_Object *)device, bus, dev, func, PCICS_COMMAND,
            cmd & ~(PCICMF_IODECODE | PCICMF_MEMDECODE));

    bak = HIDD_PCIDriver_ReadConfigLong(driver, (OOP_Object *)device, bus, dev, func, PCICS_BAR0 + (basenum << 2));
    HIDD_PCIDriver_WriteConfigLong(driver, (OOP_Object *)device, bus, dev, func, PCICS_BAR0 + (basenum << 2), ~0);
    sz = HIDD_PCIDriver_ReadConfigLong(driver, (OOP_Object *)device, bus, dev, func, PCICS_BAR0 + (basenum << 2));
    HIDD_PCIDriver_WriteConfigLong(driver, (OOP_Object *)device, bus, dev, func, PCICS_BAR0 + (basenum << 2), bak);

    /* A 64-bit memory BAR keeps the upper half of its size mask in the next slot */
    is64 = ((bak & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_MMAP) &&
           ((bak & PCIBAR_MEMTYPE_MASK) == PCIBAR_MEMTYPE_64BIT) &&
           (basenum + 1 < (device->isBridge ? 2 : 6));
    if (is64)
    {
        bakhi = HIDD_PCIDriver_ReadConfigLong(driver, (OOP_Object *)device, bus, dev, func, PCICS_BAR0 + ((basenum + 1) << 2));
        HIDD_PCIDriver_WriteConfigLong(driver, (OOP_Object *)device, bus, dev, func, PCICS_BAR0 + ((basenum + 1) << 2), ~0);
        szhi = HIDD_PCIDriver_ReadConfigLong(driver, (OOP_Object *)device, bus, dev, func, PCICS_BAR0 + ((basenum + 1) << 2));
        HIDD_PCIDriver_WriteConfigLong(driver, (OOP_Object *)device, bus, dev, func, PCICS_BAR0 + ((basenum + 1) << 2), bakhi);
    }

    if (cmd & (PCICMF_IODECODE | PCICMF_MEMDECODE))
        HIDD_PCIDriver_WriteConfigWord(driver, (OOP_Object *)device, bus, dev, func, PCICS_COMMAND, cmd);

    if ((sz & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_IO)
    {
        /* This is an IO range */
        sz &= PCIBAR_MASK_IO;
        sz = ~sz;
        sz++;
        /* Upper 16 bits of result is ignored if BAR is for I/O *AND* bits 16-31 returned zero upon read */
        if(!(bak>>16)) sz &= 0xffff;
    }
    else
    {
        /* This is memory mapped */
        sz &= PCIBAR_MASK_MEM;
        if (is64)
        {
            /*
             * The size is the lowest set bit of the combined mask, which must
             * be a contiguous run of ones from there to the top of the decoded
             * width: bit 31 when the upper dword reads back zero, bit 63
             * otherwise. Any other mask cannot be sized: return 0.
             */
#if (__WORDSIZE == 64)
            IPTR mask = (((IPTR)szhi << 32) | sz);
            IPTR ones = ~0;
            IPTR size;

            if (szhi == 0)
                ones = (IPTR)0xFFFFFFFFu;   /* upper half reads zeroes: decoded width is 32 bits */

            size = mask & (~mask + 1);

            if (mask != (~ (size - 1) & ones))
                return 0;
            return size;
#else
            if (szhi == 0)
            {
                /* 64-bit BAR that answers zeroes above dword 0: 32-bit sized */
                sz = ~sz;
                sz++;

                if (sz == 0 || (sz & (sz - 1)) != 0)
                    return 0;
                return (IPTR)sz;
            }
            if (szhi != ~0)
                return 0;
            /* szhi == ~0: fall through to the low-half sizing below */
#endif
        }
        sz = ~sz;
        sz++;
    }
    return (sz);
}
