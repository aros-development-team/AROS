/*
    Copyright © 2004-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI configuration mechanism 1 access functions
    Lang: English
*/

#include <asm/io.h>
#include <proto/exec.h>

#include "pci.h"

#define CFGADD(bus,dev,func,reg)    \
    ( 0x80000000 | ((bus)<<16) |    \
    ((dev)<<11) | ((func)<<8) | ((reg&0xff)&~3))

ULONG ReadConfig1Long(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg)
{
    ULONG temp;
    
    Disable();
    outl(CFGADD(bus, dev, sub, reg),PCI_AddressPort);
    temp=inl(PCI_DataPort);
    Enable();

    return temp;
}

void WriteConfig1Long(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val)
{
    Disable();

    outl(CFGADD(bus, dev, sub, reg),PCI_AddressPort);
    outl(val,PCI_DataPort);

    Enable();
}
