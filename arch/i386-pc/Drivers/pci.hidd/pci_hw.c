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
#include <hidd/pci.h>

#define DEBUG 1
#include <aros/debug.h>

#include "pci.h"
#include "pci_hw.h"

#ifdef SysBase
#	undef SysBase
#endif /* SysBase */

char stab[400];

ULONG readPCIConfigLong(UBYTE bus, UBYTE dev, UBYTE sub, UBYTE word)
{
	ULONG addr = 0x80000000;
	addr |= (bus & 0xff) << 16;
	addr |= (dev & 0x1f) << 11;
	addr |= (sub & 0x07) << 8;
	addr |= (word & 0x3f) << 2;
	
	outl(PCI_AddressPort, addr);
	addr=inl(PCI_DataPort);

	return addr;
}

void writePCIConfigLong(UBYTE bus, UBYTE dev, UBYTE sub, UBYTE word, ULONG value)
{
	ULONG addr = 0x80000000;
	addr |= (bus & 0xff) << 16;
	addr |= (dev & 0x1f) << 11;
	addr |= (sub & 0x07) << 8;
	addr |= (word & 0x3f) << 2;
	
	outl(PCI_AddressPort, addr);
	outl(PCI_DataPort, value);
}

int isPCIDeviceAvailable(UBYTE bus, UBYTE dev, UBYTE sub)
{
	int ret=0;
	ULONG val = readPCIConfigLong(bus, dev, sub, 0);

	if ((val & 0xffff) != 0xffff) ret=1;

	return ret;
}

void scanPCIBuses(struct pci_staticdata *psd, struct ExecBase *SysBase)
{
	int bus;
	int dev;
	int sub;

	for (bus=0; bus < 256; bus++)
	{
		for (dev=0; dev < 32; dev++)
		{
			for (sub=0; sub < 8; sub++)
			{
				if (isPCIDeviceAvailable(bus, dev, sub))
				{
					Noded_PCI_Device *ndev;
					ndev = AllocMem(sizeof (Noded_PCI_Device), MEMF_PUBLIC | MEMF_CLEAR);
					ndev->node.ln_Pri = bus - 128;
					if (ndev)
					{
						ULONG val;
						HIDDT_PCI_Device *d = &ndev->dev;
						d->Address = (bus << 8) | (dev << 3) | (sub);
						val = readPCIConfigLong(bus, dev, sub, 0);
						d->VendorID = val & 0xffff;
						d->DeviceID = val >> 16;
						val = readPCIConfigLong(bus, dev, sub, 2);
						d->RevisionID	= val & 0xff;
						d->Interface	= (val >> 8) & 0xff;
						d->SubClass	= (val >> 16) & 0xff;
						d->Class		= (val >> 24) & 0xff;
						val = readPCIConfigLong(bus, dev, sub, 11);
						d->SubsystemVendorID	= val && 16;
						d->SubsystemID			= val >> 16;
						val = readPCIConfigLong(bus, dev, sub, 15);
						d->INTLine	= (val >> 8) & 0xff;
						d->IRQLine = val & 0xff;

						d->BaseAddress[0] = (APTR)readPCIConfigLong(bus, dev, sub, 4);
						d->BaseAddress[1] = (APTR)readPCIConfigLong(bus, dev, sub, 5);
						d->BaseAddress[2] = (APTR)readPCIConfigLong(bus, dev, sub, 6);
						d->BaseAddress[3] = (APTR)readPCIConfigLong(bus, dev, sub, 7);
						d->BaseAddress[4] = (APTR)readPCIConfigLong(bus, dev, sub, 8);
						d->BaseAddress[5] = (APTR)readPCIConfigLong(bus, dev, sub, 9);

						D(bug("PCI: %d:%d:%d = %04.4lx:%04.4lx (%d/%d/%d), 0x%08.8lx 0x%08.8lx 0x%08.8lx\n",
							bus,dev,sub,
							d->VendorID,d->DeviceID,
							d->Class,d->SubClass,d->Interface,
							d->BaseAddress[0],d->BaseAddress[1],d->BaseAddress[2]
							));

						Enqueue(&psd->devices, (struct Node*)ndev);
					}
				}
			}
		}
	}
}
