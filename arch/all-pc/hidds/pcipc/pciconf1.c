/*
    Copyright (C) 2004-2026, The AROS Development Team. All rights reserved.

    Desc: PCI configuration mechanism 1 access functions
*/

#include <asm/io.h>
#include <proto/exec.h>

#include "pcipc.h"

#define CFGADD(bus,dev,func,reg)    \
    ( 0x80000000 | ((bus)<<16) |    \
    ((dev)<<11) | ((func)<<8) | ((reg)&~3))

ULONG ReadConfig1Long(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg)
{
    ULONG temp;
    
    Disable();
    outl(CFGADD(bus, dev, sub, reg),PCI_AddressPort);
    temp=inl(PCI_DataPort);
    Enable();

    return temp;
}

/*
 * The data port is four bytes wide and the low two bits of the register
 * number pick the lanes inside it, so a narrow access has to be issued at
 * the matching offset. Writing the whole long instead would put the
 * neighbouring register back, and for PCICS_COMMAND that neighbour is
 * PCICS_STATUS, whose error bits are write-1-to-clear.
 */
void WriteConfig1Word(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, UWORD val)
{
    Disable();

    outl(CFGADD(bus, dev, sub, reg),PCI_AddressPort);
    outw(val,PCI_DataPort + (reg & 2));

    Enable();
}

void WriteConfig1Long(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val)
{
    Disable();

    outl(CFGADD(bus, dev, sub, reg),PCI_AddressPort);
    outl(val,PCI_DataPort);

    Enable();
}
