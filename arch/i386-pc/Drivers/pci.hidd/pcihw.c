/*
    Copyright (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: PCI BIOS stuff for standalone i386 AROS
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/types.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <asm/io.h>

#include "pci.h"
#include "pcihw.h"

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
  Probe a specific device on a specific bus.
  Also has the argument func, which will be used
  for multifunction devices later on.
*/
void PCIProbeDev( UBYTE bus, UBYTE dev, UBYTE func )
{
    ULONG temp,i;
    UWORD vid,did;
    UBYTE cid,sid,pid;
    
    STRPTR class, sub, prgif;
    
    vid = PCIReadConfigWord(bus,dev,func,0);
    did = PCIReadConfigWord(bus,dev,func,2);

    cid = PCIReadConfigByte(bus,dev,func,11);
    sid = PCIReadConfigByte(bus,dev,func,10);
    pid = PCIReadConfigByte(bus,dev,func,9);

    PCIGetClassDesc(cid,sid,pid,&class,&sub,&prgif);
    
    if ( (vid != 0xffff && did != 0xffff) )
    {    
        D(bug("PCI: Found [%04x:%04x]@%02x.%02x %s %s %s\n",vid,did,bus,dev,class,sub,prgif));
    }    
}

/*
  Scans a complete PCI bus.
*/
void PCIScanBus( bus )
{
    UBYTE dev;

    for (dev=0;dev < 32;dev++)
    {
        PCIProbeDev(bus,dev,0);
    }
}

/*
  Will for now only scan bus 0 and 1, which
  should cover any normal PC
*/
int PCIHWProbe( void )
{
    PCIScanBus(0);
    PCIScanBus(1);
    return (0);
}










