/*
    Copyright © 2008-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __OOP_NOATTRBASES__

#include <exec/types.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/openfirmware.h>
#include <proto/kernel.h>

#include <aros/symbolsets.h>
#include <asm/mpc5200b.h>
#include <asm/io.h>

#include "pci.h"

#define DEBUG 1
#include <aros/debug.h>

#undef HiddPCIDriverAttrBase
#undef HiddAttrBase

#define HiddPCIDriverAttrBase   (PSD(cl)->hiddPCIDriverAB)
#define HiddAttrBase            (PSD(cl)->hiddAB)

#define CFGADD(bus,dev,func,reg)    \
    ( 0x80000000 | ((bus)<<16) |    \
    ((dev)<<11) | ((func)<<8) | ((reg)&~3))

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
OOP_Object *PCIEfika__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New mymsg;

    struct TagItem mytags[] = {
        { aHidd_Name, (IPTR)"PCINative" },
        { aHidd_HardwareName, (IPTR)"Efika5200 native direct access PCI driver" },
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

ULONG ReadConfigLong(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    ULONG temp;

    Disable();
    outl(CFGADD(bus, dev, sub, reg),(ULONG*)(psd->mbar + 0xdf8));
    sync();
    temp=inl_le(psd->pciio);
    outl(0, (ULONG *)(psd->mbar + 0xdf8));
    sync();
    Enable();

    return temp;
}

ULONG PCIEfika__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_ReadConfigLong *msg)
{
	ULONG val = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
    return val;
}

UWORD ReadConfigWord(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    pcicfg temp;

    temp.ul = ReadConfigLong(psd, bus, dev, sub, reg);

	return temp.uw[1 - ((reg&2)>>1)];
}


UWORD PCIEfika__Hidd_PCIDriver__ReadConfigWord(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    return ReadConfigWord(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
}

UBYTE PCIEfika__Hidd_PCIDriver__ReadConfigByte(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_ReadConfigByte *msg)
{
    pcicfg temp;

    temp.ul = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
	return temp.ub[3 - (msg->reg & 3)];
}

void WriteConfigLong(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg, ULONG val)
{
    Disable();
    outl(CFGADD(bus, dev, sub, reg),(ULONG*)(psd->mbar + 0xdf8));
    sync();
    outl_le(val, psd->pciio);
    outl(0, (ULONG *)(psd->mbar + 0xdf8));
    sync();
    Enable();
}

void PCIEfika__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
}

void PCIEfika__Hidd_PCIDriver__WriteConfigWord(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_WriteConfigWord *msg)
{
    pcicfg temp;

    temp.ul = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
    temp.uw[1 - ((msg->reg&2)>>1)] = msg->val;
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg, temp.ul);
}

void PCIEfika__Hidd_PCIDriver__WriteConfigByte(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_WriteConfigByte *msg)
{
    pcicfg temp;

    temp.ul = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
    temp.ub[3 - (msg->reg & 3)] = msg->val;
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg, temp.ul);
}

void *PCIEfika__Hidd_PCIDriver__MapPCI(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_MapPCI *msg)
{
	void *KernelBase = OpenResource("kernel.resource");
	KrnMapGlobal(msg->PCIAddress, msg->PCIAddress, msg->Length, MAP_CacheInhibit | MAP_Guarded | MAP_Readable | MAP_Writable);
	return msg->PCIAddress;
}

void *PCIEfika__Hidd_PCIDriver__AllocPCIMem(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_AllocPCIMem *msg)
{
	void *KernelBase = OpenResource("kernel.resource");
    void *memory = OOP_DoSuperMethod(cl, o, msg);

    if (memory)
    {
    	KrnSetProtection(memory, msg->Size, MAP_CacheInhibit | MAP_Readable | MAP_Writable);
    }

    return memory;
}

/*
    PCIDriver::FreePCIMemory(Address) frees previously allocated memory for PCI
    devices
*/
VOID PCIEfika__Hidd_PCIDriver__FreePCIMem(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_FreePCIMem *msg)
{
	void *KernelBase = OpenResource("kernel.resource");
	KrnSetProtection(msg->Address, 4096, MAP_Readable | MAP_Writable);
    OOP_DoSuperMethod(cl, o, msg);
}


/* Class initialization and destruction */

static int PCIEfika_InitClass(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *pci;
    int i;
    ULONG temp;
    void *OpenFirmwareBase = NULL;
    void *KernelBase = NULL;

    D(bug("[PCI_Efika] Driver initialization\n"));

    KernelBase = OpenResource("kernel.resource");
    OpenFirmwareBase = OpenResource("openfirmware.resource");
    D(bug("[PCI_Efika] OpenFirmwareBase = %08x\n", OpenFirmwareBase));

    void *key = OF_OpenKey("/builtin");
	if (key)
	{
		void *prop = OF_FindProperty(key, "reg");
		if (prop)
		{
			intptr_t *mbar = OF_GetPropValue(prop);
			LIBBASE->psd.mbar = (void *)(*mbar);

			D(bug("[PCI_Efika] MBAR located at %08x\n", LIBBASE->psd.mbar));
		}
	}

	D(bug("[PCI_Efika] PCITBATR0 = %08x\n", inl(LIBBASE->psd.mbar + 0x0d64)));
	D(bug("[PCI_Efika] PCITBATR1 = %08x\n", inl(LIBBASE->psd.mbar + 0x0d68)));

	D(bug("[PCI_Efika] PCIIW0BTAR = %08x\n", inl(LIBBASE->psd.mbar + 0x0d70)));
	D(bug("[PCI_Efika] PCIIW1BTAR = %08x\n", inl(LIBBASE->psd.mbar + 0x0d74)));
	D(bug("[PCI_Efika] PCIIW2BTAR = %08x\n", inl(LIBBASE->psd.mbar + 0x0d78)));

	D(bug("[PCI_Efika] PCIIWCR = %08x\n", inl(LIBBASE->psd.mbar + 0x0d80)));
	D(bug("[PCI_Efika] PCIICR = %08x\n", inl(LIBBASE->psd.mbar + 0x0d84)));

	for (i=0; i < 3; i++)
	{
		uint8_t attr = inl(LIBBASE->psd.mbar + 0xd80) >> (24 - i*8);

		/* If bit 0 of attribute is cleared, then the window is not used */
		if ((attr & 1) == 0)
			continue;

		temp = inl(LIBBASE->psd.mbar + 0xd70 + i*4);
		uint32_t size = 0x1000000 + ((temp << 8) & 0xff000000);
		uint32_t base = temp & 0xff000000;

		D(bug("[PCI_Efika] PCI %s: %08x - %08x\n", attr & 8 ? "IO" : "MEM", base, base + size - 1));
		if (attr & 8)
		{
			LIBBASE->psd.pciio = (uint8_t *)base;
			LIBBASE->psd.pciio_size = size;
		}
		else
		{
			LIBBASE->psd.pcimem = (uint8_t *)base;
			LIBBASE->psd.pcimem_size = size;
		}
	}

	/* Map first 64K of PCI IO space. More than that is not used anyway :) */
	KrnMapGlobal(LIBBASE->psd.pciio, LIBBASE->psd.pciio, 0x10000, MAP_CacheInhibit | MAP_Guarded | MAP_Readable | MAP_Writable);

    struct pHidd_PCI_AddHardwareDriver msg,*pmsg=&msg;

    LIBBASE->psd.hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    LIBBASE->psd.hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    if (LIBBASE->psd.hiddPCIDriverAB == 0 || LIBBASE->psd.hiddAB == 0)
    {
        D(bug("[PCI_Efika] ObtainAttrBases failed\n"));
        return FALSE;
    }

    msg.driverClass = LIBBASE->psd.driverClass;
    msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);
    D(bug("[PCI_Efika] Adding Driver to main the class OK\n"));

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    OOP_DoMethod(pci, (OOP_Msg)pmsg);
    OOP_DisposeObject(pci);

    D(bug("[PCI_Efika] All OK\n"));

    return TRUE;
}

static int PCIEfika_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[PCI_Efika] Class destruction\n"));

    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);

    return TRUE;
}

ADD2INITLIB(PCIEfika_InitClass, 0)
ADD2EXPUNGELIB(PCIEfika_ExpungeClass, 0)
