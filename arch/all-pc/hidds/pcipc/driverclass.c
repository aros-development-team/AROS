/*
    Copyright © 2004-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI direct driver for i386 native.
    Lang: English
*/

#define __OOP_NOATTRBASES__

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/acpica.h>

#include "pci.h"

#undef HiddPCIDriverAttrBase
#undef HiddAttrBase
#undef HiddPCIDeviceAttrBase

#define	HiddPCIDriverAttrBase (PSD(cl)->hiddPCIDriverAB)
#define HiddAttrBase          (PSD(cl)->hiddAB)
#define HiddPCIDeviceAttrBase (PSD(cl)->hidd_PCIDeviceAB)

/*
    We overload the New method in order to introduce the Hidd Name and
    HardwareName attributes.
*/
OOP_Object *PCPCI__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New mymsg;
    struct TagItem mytags[] =
    {
	{ aHidd_Name	    , (IPTR)"PCINative"				   },
	{ aHidd_HardwareName, (IPTR)"IA32 native direct access PCI driver" },
	{ TAG_DONE	    , 0 					   }
    };

    mymsg.mID      = msg->mID;
    mymsg.attrList = mytags;

    if (msg->attrList)
    {
        mytags[2].ti_Tag  = TAG_MORE;
        mytags[2].ti_Data = (IPTR)msg->attrList;
    }
 
    return (OOP_Object *)OOP_DoSuperMethod(cl, o, &mymsg.mID);
}

IPTR PCPCI__Hidd_PCIDriver__HasExtendedConfig(OOP_Class *cl, OOP_Object *o,
					    struct pHidd_PCIDriver_HasExtendedConfig *msg)
{
    /*
        Give false positive for testing purposes
        This should check if the device has mmio space declared with mcfg_table and mcfg_allocation, in
        which case we should return the right mmio base addressable in ECAM functions.
    */
    return 0x280177;
}

ULONG PCPCI__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, 
					    struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    /*
        We NEED the device object as it houses the ExtededConfig attribute per device.
        We know that the value stored in ExtendedConfig is the mmio base as returned by PCPCI__Hidd_PCIDriver__HasExtendedConfig.
        If we get ExtendedConfig we will use ECAM method.
    */

    IPTR extendedconfig;

    OOP_GetAttr(msg->device, aHidd_PCIDevice_ExtendedConfig, &extendedconfig);
    D(bug("PCPCI__Hidd_PCIDriver__ReadConfigLong dev->extendedconfig = %x\n", extendedconfig));

    return PSD(cl)->ReadConfigLong(msg->bus, msg->dev, msg->sub, msg->reg);
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

    temp.ul = PSD(cl)->ReadConfigLong(msg->bus, msg->dev, msg->sub, msg->reg); 
    return temp.ub[msg->reg & 3];
}

void PCPCI__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
					    struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    PSD(cl)->WriteConfigLong(msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
}

/* Class initialization and destruction */

static int PCPCI_InitClass(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *pci;
    
    D(bug("[PCI.PC] Driver initialization\n"));

    struct pHidd_PCI_AddHardwareDriver msg,*pmsg=&msg;

    struct Library *ACPICABase;

    /*
        We only (try to) fetch the mcfg_table, no need to keep ACPI library open.
    */
    ACPICABase = OpenLibrary("acpica.library", 0);
    if(ACPICABase) {
        CloseLibrary(ACPICABase);
    }

    LIBBASE->psd.hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    LIBBASE->psd.hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    LIBBASE->psd.hidd_PCIDeviceAB = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    if (LIBBASE->psd.hiddPCIDriverAB == 0 || LIBBASE->psd.hiddAB == 0 || LIBBASE->psd.hidd_PCIDeviceAB == 0)
    {
	D(bug("[PCI.PC] ObtainAttrBases failed\n"));
	return FALSE;
    }

    /* By default we use mechanism 1 */
    LIBBASE->psd.ReadConfigLong  = ReadConfig1Long;
    LIBBASE->psd.WriteConfigLong = WriteConfig1Long;

    ProbePCI(&LIBBASE->psd);

    msg.driverClass = LIBBASE->psd.driverClass;
    msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);
    D(bug("[PCI.PC] Registering Driver with PCI base class..\n"));

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    OOP_DoMethod(pci, (OOP_Msg)pmsg);
    OOP_DisposeObject(pci);

    D(bug("[PCI.PC] Driver initialization finished\n"));

    return TRUE;
}

static int PCPCI_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[PCI.PC] Class destruction\n"));

    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);
    
    return TRUE;
}
	
ADD2INITLIB(PCPCI_InitClass, 0)
ADD2EXPUNGELIB(PCPCI_ExpungeClass, 0)

