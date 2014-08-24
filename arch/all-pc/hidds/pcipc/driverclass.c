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
    if(PSD(cl)->mcfg_tbl) {

        const ACPI_TABLE_MCFG      *mcfg_tbl   = (APTR)PSD(cl)->mcfg_tbl;
        const ACPI_MCFG_ALLOCATION *mcfg_alloc = (APTR)mcfg_tbl +  sizeof(ACPI_TABLE_MCFG);
        //bug("mcfg_tbl %p\n",   mcfg_tbl);

        do {
            //bug("mcfg_alloc %p\n", mcfg_alloc);

            //bug("  Address        0x%08X\n", mcfg_alloc->Address);
            //bug("  PciSegment     0x%04X\n", mcfg_alloc->PciSegment);       /* What is this? I don't even... */
            //bug("  StartBusNumber 0x%02X\n", mcfg_alloc->StartBusNumber);
            //bug("  EndBusNumber   0x%02X\n", mcfg_alloc->EndBusNumber);

            if( (msg->bus <= mcfg_alloc->EndBusNumber) && (msg->bus >= mcfg_alloc->StartBusNumber) ) {
                //bug("mcfg_alloc has the bus number\n");
                /* FIXME: Check the validity of the extended configuration space */
                /* Address is actually QUAD, hope this will work on both 32-bit and 64-bit */
                //bug("bus %d, dev %d, func %d\n at %x\n", msg->bus, msg->dev, msg->sub, ((IPTR)mcfg_alloc->Address) + ((msg->bus<<20) | (msg->dev<<15) | (msg->sub<<12)));
                return ((IPTR)mcfg_alloc->Address) + (((msg->bus&255)<<20) | ((msg->dev&31)<<15) | ((msg->sub&7)<<12)) ; 
            }

            mcfg_alloc++;
        }while((APTR)mcfg_alloc < ((APTR)mcfg_tbl + mcfg_tbl->Header.Length));

    }
    return (IPTR)NULL;
}

ULONG PCPCI__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, 
					    struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    /*
        We NEED the device object as it houses the ExtendedConfig attribute per device.
        We know that the value stored in ExtendedConfig is the mmio base as returned by PCPCI__Hidd_PCIDriver__HasExtendedConfig.
        If we get ExtendedConfig we will use ECAM method.

        While the bus is being enumerated we automagically skip ECAM until ExtendedConfig attribute is set and we have a valid device object.
    */

    IPTR extendedconfig;

    OOP_GetAttr(msg->device, aHidd_PCIDevice_ExtendedConfig, &extendedconfig);

    if(extendedconfig) {
        //bug("PCPCI__Hidd_PCIDriver__ReadConfigLong dev->extendedconfig = %x\n", extendedconfig);
        /* This is the ECAM access method for long read others yeat unimplemented */
        ULONG *retlong = (APTR) (extendedconfig | (msg->reg & 0xffc));
        bug("ECAM retlong(%p) %x\n", retlong, *retlong);
        return *retlong;
    }
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
        if(AcpiGetTable("MCFG", 1, (ACPI_TABLE_HEADER **)&LIBBASE->psd.mcfg_tbl) != AE_OK) {
            LIBBASE->psd.mcfg_tbl = NULL;
        }
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

