/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI direct driver for i386 native.
    Lang: English
*/

#define __OOP_NOATTRBASES__

#include <exec/types.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <aros/symbolsets.h>
#include <asm/io.h>

#include "pci.h"

#define DEBUG 1
#include <aros/debug.h>

#undef HiddPCIDriverAttrBase
#undef HiddAttrBase

#define	HiddPCIDriverAttrBase	(PSD(cl)->hiddPCIDriverAB)
#define HiddAttrBase		(PSD(cl)->hiddAB)

#define CFGADD(bus,dev,func,reg)    \
    ( 0x80000000 | ((bus)<<16) |    \
    ((dev)<<11) | ((func)<<8) | ((reg)&~3))
#define CFG2ADD(dev,reg)    \
    (0xc000 | ((dev)<<8) | (reg))

typedef union _pcicfg
{
    ULONG   ul;
    UWORD   uw[2];
    UBYTE   ub[4];
} pcicfg;

/*
    We overload the New method in order to introduce the Hidd Name and
    HardwareName attributes.
*/
OOP_Object *PCPCI__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New mymsg;
    
    struct TagItem mytags[] = {
	{ aHidd_Name, (IPTR)"PCINative" },
	{ aHidd_HardwareName, (IPTR)"IA32 native direct access PCI driver" },
	{ TAG_DONE, 0 }
    };

    mymsg.mID = msg->mID;
    mymsg.attrList = (struct TagItem *)&mytags;

    if (msg->attrList)
    {
        mytags[2].ti_Tag = TAG_MORE;
        mytags[2].ti_Data = (IPTR)msg->attrList;
    }
 
    msg = &mymsg;
 
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return o;
}

ULONG ReadConfig1Long(UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    ULONG temp;
    
    Disable();
    outl(CFGADD(bus, dev, sub, reg),PCI_AddressPort);
    temp=inl(PCI_DataPort);
    Enable();

    return temp;
}

ULONG ReadConfig2Long(UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    ULONG temp;

    if (dev < 16) {
	Disable();
	outb(0xf0|(sub<<1),PCI_AddressPort);
	outb(bus,PCI_ForwardPort);
	temp=inl(CFG2ADD(dev, reg));
	outb(0,PCI_AddressPort);
	Enable();
	return temp;
    } else
	return 0xffffffff;
}


ULONG ReadConfigLong(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    switch(psd->ConfType) {
    case 1:
	return ReadConfig1Long(bus, dev, sub, reg);
    case 2:
	return ReadConfig2Long(bus, dev, sub, reg);
    }
    return 0xffffffff;
}

ULONG PCPCI__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, 
					    struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    return ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
}

UWORD ReadConfigWord(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    pcicfg temp;

    temp.ul = ReadConfigLong(psd, bus, dev, sub, reg);
    return temp.uw[(reg&2)>>1];
}
    

UWORD PCPCI__Hidd_PCIDriver__ReadConfigWord(OOP_Class *cl, OOP_Object *o, 
					    struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    return ReadConfigWord(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
}

UBYTE PCPCI__Hidd_PCIDriver__ReadConfigByte(OOP_Class *cl, OOP_Object *o, 
					    struct pHidd_PCIDriver_ReadConfigByte *msg)
{
    pcicfg temp;

    temp.ul = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg); 
    return temp.ub[msg->reg & 3];
}

void WriteConfig1Long(UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg, ULONG val)
{
    Disable();
    outl(CFGADD(bus, dev, sub, reg),PCI_AddressPort);
    outl(val,PCI_DataPort);
    Enable();
}    

void WriteConfig2Long(UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg, ULONG val)
{
    if (dev < 16) {
	Disable();
	outb(0xf0|(sub<<1),PCI_AddressPort);
	outb(bus,PCI_ForwardPort);
	outl(val,CFG2ADD(dev, reg));
	outb(0,PCI_AddressPort);
	Enable();
    }
}

void PCPCI__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
					    struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    switch(PSD(cl)->ConfType) {
    case 1:
	WriteConfig1Long(msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
	break;
    case 2:
	WriteConfig2Long(msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
    }
}

void SanityCheck(struct pci_staticdata *psd)
{
    UWORD temp;
    
    temp = ReadConfigWord(psd, 0, 0, 0, PCICS_SUBCLASS);
    if ((temp == PCI_CLASS_BRIDGE_HOST) || (temp == PCI_CLASS_DISPLAY_VGA))
	return;
    temp = ReadConfigWord(psd, 0, 0, 0, PCICS_VENDOR);
    if ((temp == PCI_VENDOR_INTEL) || (temp == PCI_VENDOR_COMPAQ))
	return;
    D(bug("Sanity check failed\n"));
    psd->ConfType = 0;
}
/* Class initialization and destruction */

static int PCPCI_InitClass(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *pci;
    ULONG temp;
    
    D(bug("PCPCI: Driver initialization\n"));

    struct pHidd_PCI_AddHardwareDriver msg,*pmsg=&msg;
    
    LIBBASE->psd.hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    LIBBASE->psd.hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    if (LIBBASE->psd.hiddPCIDriverAB == 0 || LIBBASE->psd.hiddAB == 0)
    {
	D(bug("PCPCI: ObtainAttrBases failed\n"));
	return FALSE;
    }

    LIBBASE->psd.ConfType = 0;
    outb(0x01, PCI_TestPort);
    temp = inl(PCI_AddressPort);
    outl(0x80000000, PCI_AddressPort);
    if (inl(PCI_AddressPort) == 0x80000000)
	LIBBASE->psd.ConfType = 1;
    outl(temp, PCI_AddressPort);
    if (LIBBASE->psd.ConfType == 1) {
	D(bug("PCPCI: Configuration mechanism 1 detected\n"));
        SanityCheck(&LIBBASE->psd);
    }
    if (LIBBASE->psd.ConfType == 0) {
	outb(0x00, PCI_TestPort);
	outb(0x00, PCI_AddressPort);
	outb(0x00, PCI_ForwardPort);
	if ((inb(PCI_AddressPort) == 0x00) && (inb(PCI_ForwardPort) == 0x00)) {
	    LIBBASE->psd.ConfType = 2;
	    D(bug("PCPCI: configuration mechanism 2 detected\n"));
	    SanityCheck(&LIBBASE->psd);
	}
    }	
    
    msg.driverClass = LIBBASE->psd.driverClass;
    msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);
    D(bug("PCPCI: Adding Driver to main the class OK\n"));

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    OOP_DoMethod(pci, (OOP_Msg)pmsg);
    OOP_DisposeObject(pci);

    D(bug("PCPCI: All OK\n"));

    return TRUE;
}

static int PCPCI_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("PCPCI: Class destruction\n"));
    
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);
    
    return TRUE;
}
	
ADD2INITLIB(PCPCI_InitClass, 0)
ADD2EXPUNGELIB(PCPCI_ExpungeClass, 0)
