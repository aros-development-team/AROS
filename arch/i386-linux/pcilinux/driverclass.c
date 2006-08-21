/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI direct driver for i386 linux.
    Lang: English
*/
#include <exec/types.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <aros/symbolsets.h>

#include "pci.h"

#define DEBUG 1
#include <aros/debug.h>

#ifdef HiddPCIDriverAttrBase
#undef HiddPCIDriverAttrBase
#endif // HiddPCIDriverAttrBase

#define	HiddPCIDriverAttrBase	(PSD(cl)->hiddPCIDriverAB)
#define HiddAttrBase		(PSD(cl)->hiddAB)

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
OOP_Object *PCILx__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New mymsg;
    
    struct TagItem mytags[] = {
	{ aHidd_Name, (IPTR)"PCILinux" },
	{ aHidd_HardwareName, (IPTR)"Linux direct access PCI driver" },
	{ aHidd_PCIDriver_DirectBus, FALSE },
	{ TAG_DONE, 0 }
    };

    mymsg.mID = msg->mID;
    mymsg.attrList = (struct TagItem *)&mytags;

    if (msg->attrList)
    {
        mytags[3].ti_Tag = TAG_MORE;
        mytags[3].ti_Data = (IPTR)msg->attrList;
    }
 
    msg = &mymsg;
 
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return o;
}

/*
    Some in/out magic to access PCI bus on PCs
*/

static inline ULONG inl(UWORD port)
{
    ULONG val;

    asm volatile ("inl %w1,%0":"=a"(val):"Nd"(port));

    return val;
}

static inline void outl(ULONG val, UWORD port)
{
    asm volatile ("outl %0,%w1"::"a"(val),"Nd"(port));
}

ULONG PCILx__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, 
    struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    ULONG orig,temp;

    Disable();    
    orig=inl(PCI_AddressPort);
    outl(CFGADD(msg->bus, msg->dev, msg->sub, msg->reg),PCI_AddressPort);
    temp=inl(PCI_DataPort);
    outl(orig, PCI_AddressPort);
    Enable();

    return temp;
}

void PCILx__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    ULONG orig;
    
    Disable();
    orig=inl(PCI_AddressPort);
    outl(CFGADD(msg->bus, msg->dev, msg->sub, msg->reg),PCI_AddressPort);
    outl(msg->val,PCI_DataPort);
    outl(orig, PCI_AddressPort);
    Enable();
}

IPTR PCILx__Hidd_PCIDriver__MapPCI(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_MapPCI *msg)
{
    ULONG offs = (IPTR)msg->PCIAddress >> 12;
    ULONG size = msg->Length;
    IPTR ret;

    D(bug("[PCILinux] PCIDriver::MapPCI(%x, %x)\n", offs, size));
    asm volatile(
	"push %%ebp; movl %%eax,%%ebp; movl %1,%%eax; int $0x80; pop %%ebp"
	:"=a"(ret)
	:"i"(192), "b"(0), "c"(size), "d"(0x03), "S"(0x01), "D"(PSD(cl)->fd), "0"(offs)
    );

    D(bug("[PCILinux] mmap syscall returned %x\n", ret));

    return ret;
}

VOID PCILx__Hidd_PCIDriver__UnmapPCI(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_UnmapPCI *msg)
{
    ULONG offs = (ULONG)msg->CPUAddress;
    ULONG size = msg->Length;

    asm volatile(
	"int $0x80"
	:
	:"a"(91),"b"(offs),"c"(size));
}

/* Class initialization and destruction */

#define psd (&LIBBASE->psd)

static int PCILx_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[PCILinux] deleting classes\n"));
    
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);

    return TRUE;
}
	
static int PCILx_InitClass(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *pci = NULL;

    D(bug("LinuxPCI: Driver initialization\n"));

    psd->hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    psd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);

    if (psd->hiddPCIDriverAB)
    {
	/*
	 * The class may be added to the system. Add the driver
	 * to PCI subsystem as well
	 */
	struct pHidd_PCI_AddHardwareDriver msg;
		
	/*
	 * PCI is suppose to destroy the class on its Dispose
	 */
	msg.driverClass = psd->driverClass;
	msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);

	// Add it for God's sake! ;)
	pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
	OOP_DoMethod(pci, (OOP_Msg)&msg);
	OOP_DisposeObject(pci);
    }
    else
	return FALSE;

    D(bug("LinuxPCI: Driver ClassPtr = %x\n", psd->driverClass));

    return TRUE;
}

ADD2INITLIB(PCILx_InitClass, 0)
ADD2EXPUNGELIB(PCILx_ExpungeClass, 0)
