/*
    Copyright © 2004-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI configuration mechanism 2 access functions
    Lang: English
*/

#include <aros/debug.h>
#include <asm/io.h>
#include <proto/exec.h>

#include "pci.h"

#ifdef LEGACY_SUPPORT

#define CFG2ADD(dev,reg)    \
    (0xc000 | ((dev)<<8) | (reg&0xff))

static ULONG ReadConfig2Long(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg)
{
    ULONG temp;

    if (dev < 16)
    {
	Disable();

	outb(0xf0|(sub<<1),PCI_AddressPort);
	outb(bus,PCI_ForwardPort);
	temp=inl(CFG2ADD(dev, reg));
	outb(0,PCI_AddressPort);

	Enable();
	return temp;
    }
    else
	return 0xffffffff;
}

static void WriteConfig2Long(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val)
{
    if (dev < 16)
    {
	Disable();
	outb(0xf0|(sub<<1),PCI_CSEPort);
	outb(bus,PCI_ForwardPort);
	outl(val,CFG2ADD(dev, reg));
	outb(0,PCI_AddressPort);
	Enable();
    }
}

static inline BOOL SanityCheck(struct pci_staticdata *psd)
{
    UWORD temp;

    /* Check if the bus 0 is not empty */
    temp = ReadConfigWord(psd, 0, 0, 0, PCICS_PRODUCT);
    if ((temp != 0x0000) && (temp != 0xFFFF))
	return TRUE;

    D(bug("[PCI.PC] Sanity check failed\n"));
    return FALSE;
}

static BOOL ProbeMech1(struct pci_staticdata *psd)
{
    ULONG temp = inl(PCI_AddressPort);
    ULONG val;

    outl(0x80000000, PCI_AddressPort);
    val = inl(PCI_AddressPort);
    outl(temp, PCI_AddressPort);

    if (val == 0x80000000)
    {
	D(bug("[PCI.PC] Configuration mechanism 1 detected\n"));

        if (SanityCheck(psd))
        {
            /* Succesfully detected exit */
            return TRUE;
        }
    }

    return FALSE;
}

void ProbePCI(struct pci_staticdata *psd)
{
    /*
     * All new board support only mechanism 1. So we probe for it first.
     * We do it because on some machines (MacMini) PCI_MechSelect is
     * used by chipset as a reset register (and perhaps some other proprietary control).
     * Writing 0x01 to it makes machine cold reboot not working.
     */
    if (ProbeMech1(psd))
    	return;

    /*
     * There's no Mechanism 1.
     * Perhaps it's Intel Neptune or alike board. We can try to switch it to Mech1.
     */
    outb(0x01, PCI_MechSelect);
    if (ProbeMech1(psd))
    	return;

    /* Completely no support. Try mechanism 2. */
    outb(0x00, PCI_MechSelect);
    outb(0x00, PCI_CSEPort);
    outb(0x00, PCI_ForwardPort);

    if ((inb(PCI_CSEPort) == 0x00) && (inb(PCI_ForwardPort) == 0x00))
    {
	D(bug("[PCI.PC] configuration mechanism 2 detected\n"));

    	psd->ReadConfigLong  = ReadConfig2Long;
    	psd->WriteConfigLong = WriteConfig2Long;

	if (SanityCheck(psd))
	{
	    /* Confirmed */
	    return;
	}
    }

    /*
     * Newer systems may have empty bus 0. In this case SanityCheck() will fail. We
     * assume configuration type 1 for such systems.
     * Probably SanityCheck() should be revised or removed at all.
     */
    D(bug("[PCI.PC] Failing back to configuration mechanism 1\n"));

    psd->ReadConfigLong  = ReadConfig1Long;
    psd->WriteConfigLong = WriteConfig1Long;
}

#endif
