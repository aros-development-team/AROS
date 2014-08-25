/*
    Copyright © 2004-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI direct driver for i386 native.
    Lang: English
*/

#define __OOP_NOATTRBASES__
//#define DEBUG 1
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
    IPTR mmio = 0;

    if(PSD(cl)->mcfg_tbl) {

        const ACPI_TABLE_MCFG      *mcfg_tbl   = (APTR)PSD(cl)->mcfg_tbl;
        const ACPI_MCFG_ALLOCATION *mcfg_alloc = (APTR)mcfg_tbl +  sizeof(ACPI_TABLE_MCFG);

        do {
            if( (msg->bus <= mcfg_alloc->EndBusNumber) && (msg->bus >= mcfg_alloc->StartBusNumber) ) {

                /*
                    FIXME: Check the validity of the extended configuration space
                */

                ULONG *val, *val2;

                mmio = ((IPTR)mcfg_alloc->Address) + (((msg->bus&255)<<20) | ((msg->dev&31)<<15) | ((msg->sub&7)<<12));

                val = (APTR) (mmio + 0x100);
                val2 = (APTR) (mmio);

                D(bug("%p %08x\n", val2, *val2));
                D(bug("bus %d dev %d sub %d %p MMIO + 0x100 = %08x\n", msg->bus, msg->dev, msg->sub, val, *val));

                break;
            }else{
                D(bug("HasExtendedConfig: Device not found! bus %d dev %d sub %d \n", msg->bus, msg->dev, msg->sub));
            }

            mcfg_alloc++;
        }while((APTR)mcfg_alloc < ((APTR)mcfg_tbl + mcfg_tbl->Header.Length));

    }
    return mmio;
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

    IPTR mmio = 0;

    OOP_GetAttr(msg->device, aHidd_PCIDevice_ExtendedConfig, &mmio);
    if(mmio) {
        /* This is the ECAM access method for long read others yeat unimplemented */
        ULONG *retlong, *val;

        val = (APTR) (mmio);
        retlong = (APTR) (mmio + (msg->reg & 0xffc));

        D(bug("ECAM retlong %08x %p %08x\n", *val, retlong, *retlong));
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

