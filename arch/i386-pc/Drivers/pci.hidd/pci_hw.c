/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI class
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <oop/oop.h>
#include <hidd/pcibus.h>
#include <asm/io.h>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#include "pci.h"
#include "pci_hw.h"

#ifdef SysBase
#	undef SysBase
#endif /* SysBase */

char stab[400];

/* Macro for generating values for PCI_AddressPort */
#define CFGADD(bus,dev,func,reg) \
  ( 0x80000000 | ((bus)<<16) | \
  ((dev)<<11) | ((func)<<8) | ((reg)&~3))

/* Used for CFGspace accesses */
typedef union _pcicfg
{
    ULONG	ul;
    UWORD	uw[2];
    UBYTE	ub[4];
} pcicfg;
    
ULONG readPCIConfigLong(UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    ULONG orig,temp;

    /*
     * We preserve the original value of the address register just in case.
     * Should not be needed, but might save our a** some day.
     */
    orig=inl(PCI_AddressPort);
    outl(CFGADD(bus,dev,sub,reg),PCI_AddressPort);
    temp=inl(PCI_DataPort);
    outl(orig,PCI_AddressPort);

    return temp;
}

void writePCIConfigLong(UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg, ULONG value)
{
    ULONG orig;

    /* As in reading, we preserve original value of address register */
    orig=inl(PCI_AddressPort);
    outl(CFGADD(bus,dev,sub,reg),PCI_AddressPort);
    outl(value,PCI_DataPort);
    outl(orig,PCI_AddressPort);
}

UWORD readPCIConfigWord(UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    pcicfg temp;

    temp.ul = readPCIConfigLong(bus,dev,sub,reg);
    return temp.uw[((reg&2)>>1)];
}

UBYTE readPCIConfigByte(UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    pcicfg temp;

    temp.ul = readPCIConfigLong(bus,dev,sub,reg);
    return temp.ub[(reg&3)];
}

/* Returns 0 for no device, 1 for non-multi device and 2 for
 * a multifunction device */
int isPCIDeviceAvailable(UBYTE bus, UBYTE dev, UBYTE sub)
{
    UWORD Vend;
    UBYTE Type;

    Vend = readPCIConfigWord(bus,dev,sub,PCICS_VENDOR);
    if ((Vend == 0xffff) || (Vend == 0x0000))
    {
	/* 0xffff is an invalid vendor ID, and so is 0x0000
	 * (Well, actually 0x0000 belongs to Gammagraphx, but this really
	 * clashes with multifunc device scanning, so lets just hope nobody
	 * has a card from them :) )
	 */
	return 0;
    }
    Type = readPCIConfigByte(bus,dev,sub,PCICS_HEADERTYPE);
    if ((Type & PCIHT_MULTIFUNC) == PCIHT_MULTIFUNC)
	return 2;

    return 1;
}

void addPCIDevice(UBYTE bus, UBYTE dev, UBYTE sub, struct pci_staticdata *psd, struct ExecBase *SysBase)
{
    Noded_PCI_Device *ndev;
    UBYTE ht,subbus;
    STRPTR cDesc,sDesc,pDesc;
    
    /* First off, we check if this is a bridge
     * If it is, we update the psd->highBus value
     */
    ht = (readPCIConfigByte(bus,dev,sub,PCICS_HEADERTYPE) & PCIHT_MASK);
    if (ht == PCIHT_BRIDGE)
    {
	/* This device complies to the standard PCI-to-PCI bridge interface */
	subbus = readPCIConfigByte(bus,dev,sub,PCIBR_SUBBUS);
	if (psd->highBus < subbus)
	{
	    psd->highBus = subbus;
	}
    }
    
    ndev = AllocMem(sizeof (Noded_PCI_Device), MEMF_PUBLIC | MEMF_CLEAR);
    ndev->node.ln_Pri = bus - 128;
    if (ndev)
    {
	HIDDT_PCI_Device *d = &ndev->dev;

	/* Store basic values in the Noded_PCI_Device structure */
	d->Address = (bus << 8) | (dev << 3) | (sub);
	d->VendorID    = readPCIConfigWord(bus,dev,sub,PCICS_VENDOR);
	d->DeviceID    = readPCIConfigWord(bus,dev,sub,PCICS_PRODUCT);
	d->RevisionID  = readPCIConfigByte(bus,dev,sub,PCICS_REVISION);
	d->Interface   = readPCIConfigByte(bus,dev,sub,PCICS_PROGIF);
	d->SubClass    = readPCIConfigByte(bus,dev,sub,PCICS_SUBCLASS);
	d->Class       = readPCIConfigByte(bus,dev,sub,PCICS_CLASS);
	d->SubsysVID   = readPCIConfigWord(bus,dev,sub,PCICS_SUBVENDOR);
	d->SubsystemID = readPCIConfigWord(bus,dev,sub,PCICS_SUBSYSTEM);
	d->INTLine     = readPCIConfigByte(bus,dev,sub,PCICS_INT_LINE);
	d->IRQLine     = readPCIConfigByte(bus,dev,sub,PCICS_INT_PIN);
	d->HeaderType  = (readPCIConfigByte(bus,dev,sub,PCICS_HEADERTYPE) & PCIHT_MASK);

	/* This part will not work as expected on anything but headertype 0 */
	d->BaseAddress[0] = (APTR)readPCIConfigLong(bus, dev, sub, PCICS_BAR0);
	d->BaseAddress[1] = (APTR)readPCIConfigLong(bus, dev, sub, PCICS_BAR1);
	d->BaseAddress[2] = (APTR)readPCIConfigLong(bus, dev, sub, PCICS_BAR2);
	d->BaseAddress[3] = (APTR)readPCIConfigLong(bus, dev, sub, PCICS_BAR3);
	d->BaseAddress[4] = (APTR)readPCIConfigLong(bus, dev, sub, PCICS_BAR4);
	d->BaseAddress[5] = (APTR)readPCIConfigLong(bus, dev, sub, PCICS_BAR5);

	getPCIClassDesc(d->Class,d->SubClass,d->Interface,&cDesc,&sDesc,&pDesc);

	bug("[PCI] %02x:%02x:%1x = %04.4lx:%04.4lx (%s %s %s)\n",
		    bus,dev,sub,
		    d->VendorID,d->DeviceID,
		    cDesc,sDesc,pDesc
	     );
	if ((d->HeaderType & PCIHT_MASK) == PCIHT_BRIDGE)
	{
	    bug("[PCI] Bridge: Pri bus=%02x, Sec bus=%02x, Sub bus=%02x\n",
			readPCIConfigByte(bus,dev,sub,PCIBR_PRIBUS),
			readPCIConfigByte(bus,dev,sub,PCIBR_SECBUS),
			readPCIConfigByte(bus,dev,sub,PCIBR_SUBBUS));
	}

	Enqueue(&psd->devices, (struct Node*)ndev);
    }
}	

void scanPCIBuses(struct pci_staticdata *psd, struct ExecBase *SysBase)
{
    int bus;
    int dev;
    int sub;
    int type;

    bus = 0;
    do 
    {
	D(bug("PCI: Scanning bus %d\n",bus));
	for (dev=0; dev < 32; dev++)
	{
	    type = isPCIDeviceAvailable(bus,dev,0);
	    switch(type)
	    {
		case 1:
		    addPCIDevice(bus,dev,0,psd,SysBase);
		    break;
		case 2:
		    addPCIDevice(bus,dev,0,psd,SysBase);
		    for (sub=1; sub < 8; sub++)
		    {
			if (isPCIDeviceAvailable(bus, dev, sub))
			{
			    addPCIDevice(bus,dev,sub,psd,SysBase);
			}
		    }
		    break;
		default:
		    break;
	    }
	}
	bus++;
    } while (bus <= psd->highBus);
}
