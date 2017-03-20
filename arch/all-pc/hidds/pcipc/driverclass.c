/*
    Copyright © 2004-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI direct driver for i386 native.
    Lang: English
*/

#define __OOP_NOATTRBASES__

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/acpica.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

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
	{ aHidd_Name,           (IPTR)"pcipc.hidd"			        },
	{ aHidd_HardwareName,   (IPTR)"IA32 native direct access PCI driver"    },
	{ TAG_DONE,             0 					        }
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

    if(PSD(cl)->pcipc_acpiMcfgTbl) {

        const ACPI_TABLE_MCFG      *mcfg_tbl   = (APTR)PSD(cl)->pcipc_acpiMcfgTbl;
        const ACPI_MCFG_ALLOCATION *mcfg_alloc = (APTR)mcfg_tbl +  sizeof(ACPI_TABLE_MCFG);

        do {
            if( (msg->bus <= mcfg_alloc->EndBusNumber) && (msg->bus >= mcfg_alloc->StartBusNumber) ) {

                /*
                    FIXME: Check the validity of the extended configuration space
                */

                ULONG *extcap;

                mmio = ((IPTR)mcfg_alloc->Address) + (((msg->bus&255)<<20) | ((msg->dev&31)<<15) | ((msg->sub&7)<<12));

                /*
                    Absence of any Extended Capabilities is required to be indicated
                    by an Extended Capability header with a Capability ID of 0000h,
                    a Capability Version of 0h, and a Next Capability Offset of 0h.

                    For PCI devices OnMyHardware(TM) extended capability header at 0x100 reads 0xffffffff.

                    0xffffffff is non valid extended capability header as it would point
                    the next capability outside configuration space.

                    If we get extended capability header set with all ones then we won't use ECAM.
                    (PCI device in mmio space, not PCIe)
                */

                extcap = (APTR) (mmio + 0x100);
                D(bug("HasExtendedConfig: bus %d dev %d sub %d extcap %08x\n", msg->bus, msg->dev, msg->sub, *extcap));
                if(*extcap == 0xffffffff) {
                    D(bug("    Device is PCI not PCIe\n"));
                    mmio = 0;
                }

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
        /* This is the ECAM access method for long read */
        volatile ULONG *longreg;

        longreg = (APTR) (mmio + (msg->reg & 0xffc));

        D(bug("ECAM.read longreg %p %08x\n", longreg, *longreg));
        return *longreg;
    }

    /*
        Last good long register without ECAM,
        macros in CAM methods take care of the alignment.
        we don't want to return some random value.
    */
    if(msg->reg < 0x100) {
        return PSD(cl)->ReadConfigLong(msg->bus, msg->dev, msg->sub, msg->reg);
    } else {
        return 0xffffffff;
    }
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
    IPTR mmio = 0;

    OOP_GetAttr(msg->device, aHidd_PCIDevice_ExtendedConfig, &mmio);
    if(mmio) {
        /* This is the ECAM access method for long write */
        volatile ULONG *longreg;
        longreg = (APTR) (mmio + (msg->reg & 0xffc));
        D(bug("ECAM.write.old longreg %p %08x  = %08x\n", longreg, *longreg, msg->val));
        *longreg = msg->val;
        D(bug("ECAM.write.new longreg %p %08x == %08x?\n", longreg, *longreg, msg->val));
    } else {
        /*
            Last good long register without ECAM,
            macros in CAM methods take care of the alignment.
            we don't want to store the value in some random address.
        */
        if(msg->reg < 0x100) {
            PSD(cl)->WriteConfigLong(msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
        }
    }
}

#undef _psd

/* Class initialization and destruction */

ACPI_STATUS PCPCI_ACPIDeviceCallback(ACPI_HANDLE Object, ULONG nesting_level, void *Context, void **RerturnValue)
{
    ACPI_BUFFER RetVal;
    RetVal.Length = ACPI_ALLOCATE_BUFFER;
    int status;

    D(bug("[PCI.PC] %s: Object = %p, nesting_level=%d, Context=%p\n", __func__, Object, nesting_level, Context);)

    status = AcpiEvaluateObject(Object, "_PRT", NULL, &RetVal);

    D(bug("[PCI.PC] %s: result of PRT evaluate=%d\n", __func__, status);)

    if (!ACPI_FAILURE(status))
    {
        ACPI_OBJECT *RObject = RetVal.Pointer;

        D(bug("[PCI.PC] %s: RetVal.Length=%d, RetVal.Pointer=%p\n", __func__, RetVal.Length, RetVal.Pointer);)
        D(bug("[PCI.PC] %s: Object->Type =%d, Object->Package.Count=%d\n", __func__, RObject->Type, RObject->Package.Count);)

        for (unsigned int i=0; i < RObject->Package.Count; i++)
        {
            ACPI_OBJECT *item = &RObject->Package.Elements[i];
            D(bug("[PCI.PC] %s:  %03d: %p Type=%d Count=%d \n        ", __func__, i, item, item->Type, item->Package.Count);)
            for (unsigned int j=0; j < item->Package.Count; j++)
            {
                ACPI_OBJECT *jitem = &item->Package.Elements[j];
                D(bug("%08x ", jitem->Integer.Value);)
            }
            D(bug("\n");)

        }
    }

    return 0;
}

static int PCPCI_InitClass(LIBBASETYPEPTR LIBBASE)
{
    struct pcipc_staticdata *_psd = &LIBBASE->psd;
    struct pHidd_PCI_AddHardwareDriver msg, *pmsg = &msg;
    OOP_Object *pci;

    D(bug("[PCI.PC] Driver initialization\n"));

    /* Open ACPI and cache the pointer to the MCFG table.. */
    ACPICABase = OpenLibrary("acpica.library", 0);
    if (ACPICABase)
        AcpiGetTable(ACPI_SIG_MCFG, 1, (ACPI_TABLE_HEADER **)&_psd->pcipc_acpiMcfgTbl);

    _psd->hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    _psd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    _psd->hidd_PCIDeviceAB = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    if (_psd->hiddPCIDriverAB == 0 || _psd->hiddAB == 0 || _psd->hidd_PCIDeviceAB == 0)
    {
	D(bug("[PCI.PC] ObtainAttrBases failed\n"));
	return FALSE;
    }

    if (ACPICABase)
        AcpiGetDevices("PNP0A03", PCPCI_ACPIDeviceCallback, LIBBASE, NULL);

    /* Default to using config mechanism 1 */
    _psd->ReadConfigLong  = ReadConfig1Long;
    _psd->WriteConfigLong = WriteConfig1Long;

    PCIPC_ProbeConfMech(&LIBBASE->psd);

    msg.driverClass = _psd->driverClass;
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
    struct pcipc_staticdata *_psd = &LIBBASE->psd;

    D(bug("[PCI.PC] Class destruction\n"));

    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);

    if (ACPICABase)
        CloseLibrary(ACPICABase);

    return TRUE;
}
	
ADD2INITLIB(PCPCI_InitClass, 0)
ADD2EXPUNGELIB(PCPCI_ExpungeClass, 0)

