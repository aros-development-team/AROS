/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI BIOS stuff for standalone i386 AROS
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

#undef	SDEBUG
#undef	DEBUG
#define DEBUG 1
#include <aros/debug.h>

union pcicfg
{
        ULONG ulong;
        UWORD uword[2];
        UBYTE ubyte[4];
};

/* Generate a value for port 0xcf8 */
#define CFGADD(bus,dev,func,reg) \
  ( 0x80000000 | ((bus)<<16) | \
  ((dev)<<11) | ((func)<<8) | ((reg)&~3))

/*
  Read a full ULONG from PCI configuration space
  Accesses should really be locked with a semaphore
  or something.
*/
ULONG PCIReadConfigLong( UBYTE bus, UBYTE dev, UBYTE func, UBYTE reg)
{
    ULONG orig,temp;
    
    orig = inl(0xcf8);
    outl(CFGADD(bus,dev,func,reg),0xcf8);
    temp = inl(0xcfc);
    outl(orig,0xcf8);

    return temp;
}

/*
  Reading UWORD and UBYTE from the PCI configuration space.
  The reason for being so convoluted is that according to the
  PCI 2.2 standard, a host bridge should only respond to DWORD
  accesses on the 0xcf8 and 0xcfc ports. It seems to work with
  byte and word accesses as well, as linux does, but this should
  be more compatible.
*/
UWORD PCIReadConfigWord( UBYTE bus, UBYTE dev, UBYTE func, UBYTE reg)
{
    union pcicfg temp;

    temp.ulong = PCIReadConfigLong(bus,dev,func,reg);
    return temp.uword[((reg&2)>>1)];
}

UBYTE PCIReadConfigByte( UBYTE bus, UBYTE dev, UBYTE func, UBYTE reg)
{
    union pcicfg temp;

    temp.ulong = PCIReadConfigLong(bus,dev,func,reg);
    return temp.ubyte[(reg&3)];
}

/*
 * Write a full ULONG to PCI config space
 */
void PCIWriteConfigLong( UBYTE bus, UBYTE dev, UBYTE func, UBYTE reg, ULONG val)
{
    ULONG orig;

    orig = inl(0xcf8);
    outl(CFGADD(bus,dev,func,reg),0xcf8);
    outl(val,0xcfc);
    outl(orig,0xcf8);
};

void PCIWriteConfigWord( UBYTE bus, UBYTE dev, UBYTE func, UBYTE reg, UWORD val)
{
    union pcicfg temp;

    temp.ulong = PCIReadConfigLong(bus,dev,func,reg);
    temp.uword[((reg&2)>>1)] = val;
    PCIWriteConfigLong(bus,dev,func,reg,temp.ulong);
}

void PCIWriteConfigByte( UBYTE bus, UBYTE dev, UBYTE func, UBYTE reg, UBYTE val)
{
    union pcicfg temp;

    temp.ulong = PCIReadConfigLong(bus,dev,func,reg);
    temp.ubyte[(reg&3)] = val;
    PCIWriteConfigLong(bus,dev,func,reg,temp.ulong);
}
   
/*
  Probe a specific device on a specific bus.
  Also has the argument func, which will be used
  for multifunction devices later on.
  Returns the headertype to the caller
*/
UBYTE PCIProbeDev( UBYTE bus, UBYTE dev, UBYTE func, struct pci_staticdata *xsd )
{
    struct PCIDevInfo *di;
    int i;

    di = AllocMem(sizeof(struct PCIDevInfo),MEMF_PUBLIC|MEMF_CLEAR);
    if (di)
    {
	di->Vendor = PCIReadConfigWord(bus,dev,func,PCICS_VENDOR);
	di->Product = PCIReadConfigWord(bus,dev,func,PCICS_PRODUCT);
	di->Class = PCIReadConfigByte(bus,dev,func,PCICS_CLASS);
	di->SubClass = PCIReadConfigByte(bus,dev,func,PCICS_SUBCLASS);
	di->ProgIF = PCIReadConfigByte(bus,dev,func,PCICS_PROGIF);
	di->HeaderType = PCIReadConfigByte(bus,dev,func,PCICS_HEADERTYPE);
	di->IRQLine = PCIReadConfigByte(bus,dev,func,PCICS_INT_LINE);
	di->IRQPin = PCIReadConfigByte(bus,dev,func,PCICS_INT_PIN);
	di->PCIBus = bus;
	di->PCIDev = dev;
	di->PCIFunc = func;

	if ( (di->Vendor != 0xffff && di->Product != 0xffff) && (di->Vendor != 0x0000 && di->Product != 0x0000) )
	{   
	    /* Yes, we found a valid card here */
	    /* Only scan bases on non-bridges */
	    if ((di->HeaderType & PCIHT_MASK) == 0)
	    {
		/* Now we scan the base addresses */
		for (i=0;i<6;i++)
		{
		    di->Bases[i] = PCIReadConfigLong(bus,dev,func,PCICS_BAR+(i<<2));
		    di->BaseSize[i] = PCISizeBase(bus,dev,func,i);
		    if(di->Bases[i] & 1)
		    {
			/* IO memory */
			di->Bases[i] &= 0xfffe;
		    }
		    else
		    {
			/* memory mapped */
			di->Bases[i] &= 0xfffffff0;
		    }
		}
	    }
	    /* Dump device info */
	    PCIDumpDev(di);
	    
	    /* Add it to the pci.hidd devicelist */
	    AddTail(&xsd->devs,(struct Node *)di);
	}    
	else
	    FreeMem(di,sizeof(struct PCIDevInfo));

	return (di->HeaderType);
    }
    return (0);
}

/*
  Scans a complete PCI bus.
*/
void PCIScanBus( UBYTE bus, struct pci_staticdata *xsd )
{
    UBYTE dev,type,func;


    for (dev=0;dev < 32;dev++)
    {
        type = PCIProbeDev(bus,dev,0,xsd);
	if (type & PCIHT_MULTIFUNC)
	{
	    /* We found a multi function device */
	    for (func=1;func < 8;func++)
		PCIProbeDev(bus,dev,func,xsd);
	}
	
    }
}

/*
  Will for now only scan bus 0 and 1, which
  should cover any normal PC
*/
int PCIHWProbe( struct pci_staticdata *xsd )
{
    PCIScanBus(0,xsd);
    PCIScanBus(1,xsd);
    return (0);
}










