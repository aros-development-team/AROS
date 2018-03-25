/*
    Copyright © 2004-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI direct driver for i386/x86_64 native.
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

#define DECAM(x)

#undef HiddPCIDriverAttrBase
#undef HiddAttrBase
#undef HiddPCIDeviceAttrBase

#define	HiddPCIDriverAttrBase (PSD(cl)->hiddPCIDriverAB)
#define HiddAttrBase          (PSD(cl)->hiddAB)
#define HiddPCIDeviceAttrBase (PSD(cl)->hidd_PCIDeviceAB)

/*
 * N.B. Do not move/remove/refactor the following variable unless you fully
 * understand the implications. It must be an explicit global variable for the
 * following reasons:
 *  - Linking with the linklib: acpica.library is a stack-call library, which
 *    means that its functions are always called through a stub linklib.
 *    Linking with the linklib will fail if ACPICABase is not a global variable.
 *  - acpica.library is optional: this class must still be able to run if
 *    acpica.library is unavailable. If ACPICABase is not defined as a global
 *    variable, the autoinit system will create one. However, in that case the
 *    autoinit system will also take over the opening of the library, and not
 *    allow this class to be loaded if the library isn't found.
 */
struct Library *ACPICABase;

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

void PCPCI__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_PCIDRV_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_PCIDriver_IRQRoutingTable:
                if (IsListEmpty(&PSD(cl)->pcipc_irqRoutingTable))
                    *msg->storage = (IPTR)NULL;
                else
                    *msg->storage = (IPTR)&PSD(cl)->pcipc_irqRoutingTable;
                break;

            default:
                OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
                break;
        }
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
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
                D(bug("[PCI.PC] %s: bus %d dev %d sub %d extcap %08x\n", __func__, msg->bus, msg->dev, msg->sub, *extcap));
                if(*extcap == 0xffffffff) {
                    D(bug("    Device is PCI not PCIe\n"));
                    mmio = 0;
                }

                break;
            }else{
                D(bug("[PCI.PC] %s: Device not found! bus %d dev %d sub %d \n", __func__, msg->bus, msg->dev, msg->sub));
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
        ULONG configLong;

        configLong = *(ULONG volatile *) (mmio + (msg->reg & 0xffc));

        DECAM(bug("[PCI.PC] %s: ECAM Read @ %p = %08x\n", __func__, (mmio + (msg->reg & 0xffc)), configLong));

        return configLong;
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
        ULONG volatile *longreg;
        longreg = (ULONG volatile *) (mmio + (msg->reg & 0xffc));
        DECAM(bug("[PCI.PC] %s: ECAM.write.old longreg %p %08x  = %08x\n", __func__, longreg, *longreg, msg->val));
        *longreg = msg->val;
        DECAM(bug("[PCI.PC] %s: ECAM.write.new longreg %p %08x == %08x?\n", __func__, longreg, *longreg, msg->val));
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

/* Parse an individual routing entry */
static void EnumPCIIRQ(struct pcipc_staticdata *psd,
    ACPI_PCI_ROUTING_TABLE *item, UWORD bus_num)
{
    struct PCI_IRQRoutingEntry *n =
        AllocVec(sizeof(struct PCI_IRQRoutingEntry), MEMF_CLEAR | MEMF_ANY);

    if (n)
    {
        n->re_PCIBusNum = bus_num;
        n->re_PCIDevNum = (item->Address >> 16) & 0xFFFF;

        D(bug("[PCI.PC] %s:  %02x:%02x.x", __func__, n->re_PCIBusNum,
            n->re_PCIDevNum);)

        n->re_IRQPin = item->Pin + 1;
        D(bug(" INT%c", 'A' + n->re_IRQPin - 1));

        if (strlen(item->Source) > 0)
        {
            D(bug(" '%s'\n", item->Source));
            FreeVec(n);
        }
        else
        {
            D(bug(" using GSI %u\n", item->SourceIndex));
            n->re_IRQ = item->SourceIndex;
            ADDTAIL(&psd->pcipc_irqRoutingTable, n);
        }
    }
}

static void FindIRQRouting(struct pcipc_staticdata *psd, ACPI_HANDLE parent,
    UBYTE bus_num)
{
    ACPI_HANDLE child = NULL;
    ACPI_DEVICE_INFO *dev_info;
    ACPI_BUFFER buffer;
    ACPI_PCI_ROUTING_TABLE *entry;
    UBYTE dev_num, func_num, child_bus_num;
    BOOL is_bridge;
    ULONG address;

    D(bug("[PCI.PC] %s: Scanning bus %d\n", __func__, bus_num);)

    /* Get routing table for current bus */
    buffer.Length = ACPI_ALLOCATE_BUFFER;
    if (AcpiGetIrqRoutingTable(parent, &buffer) == AE_OK)
    {
        D(bug("[PCI.PC] %s: Found _PRT\n", __func__);)

        /* Translate routing table entries into nodes for our own list */
        for (entry = buffer.Pointer; entry->Length != 0;
            entry = (APTR)entry + entry->Length)
        {
            EnumPCIIRQ(psd, entry, bus_num);
        }

        FreeVec(buffer.Pointer);
    }

    /* Walk current bus's devices, and process the buses on the other side of
     * any bridges found recursively */
    while (AcpiGetNextObject(ACPI_TYPE_DEVICE, parent, child, &child) == AE_OK)
    {
        /* Get device:function part of PCI address */
        if (AcpiGetObjectInfo(child, &dev_info) == AE_OK)
        {
            if ((dev_info->Valid & ACPI_VALID_ADR) != 0)
            {
                address = (ULONG)dev_info->Address;
                dev_num = address >> 16 & 0xff;
                func_num = address & 0xff;
                FreeVec(dev_info);

                /* Check if this is a PCI-PCI bridge */
                is_bridge = ReadConfigWord(psd, bus_num, dev_num, func_num,
                    PCIBR_SUBCLASS) == 0x0604;

                /* Look for more routing tables */
                if (is_bridge)
                {
                    D(bug("[PCI.PC] %s: Found a bridge at %02x:%02x.%x\n",
                        __func__, bus_num, dev_num, func_num);)

                    /* Get this bridge's bus number */
                    child_bus_num = ReadConfigByte(psd, bus_num, dev_num, func_num,
                        PCIBR_SECBUS);
                    FindIRQRouting(psd, child, child_bus_num);
                }
            }
        }

    }

    return;
}

/* Parse a routing table */
static ACPI_STATUS ACPIDeviceCallback(ACPI_HANDLE handle, ULONG nesting_level,
    void *context, void **return_value)
{
    struct pcipc_staticdata *psd = (struct pcipc_staticdata *)context;

    FindIRQRouting(psd, handle, 0);

    return AE_OK;
}

static int PCPCI_InitClass(LIBBASETYPEPTR LIBBASE)
{
    struct pcipc_staticdata *_psd = &LIBBASE->psd;
    struct pHidd_PCI_AddHardwareDriver msg, *pmsg = &msg;
    OOP_Object *pci;

    NEWLIST(&_psd->pcipc_irqRoutingTable);

    D(bug("[PCI.PC] %s()\n", __func__));

    /* Open ACPI and cache the pointer to the MCFG table */
    ACPICABase = OpenLibrary("acpica.library", 0);

    if (ACPICABase)
        AcpiGetTable(ACPI_SIG_MCFG, 1, (ACPI_TABLE_HEADER **)&_psd->pcipc_acpiMcfgTbl);

    _psd->hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    _psd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    _psd->hidd_PCIDeviceAB = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (_psd->hiddPCIDriverAB == 0 || _psd->hiddAB == 0 || _psd->hidd_PCIDeviceAB == 0)
    {
        D(bug("[PCI.PC] %s: ObtainAttrBases failed\n", __func__));
        return FALSE;
    }

    /* Default to using config mechanism 1 */
    _psd->ReadConfigLong  = ReadConfig1Long;
    _psd->WriteConfigLong = WriteConfig1Long;

    PCIPC_ProbeConfMech(&LIBBASE->psd);

    /* Find routing tables */
    if (ACPICABase)
    {
        AcpiGetDevices("PNP0A03", ACPIDeviceCallback, _psd, NULL);
    }

    msg.driverClass = _psd->driverClass;
    msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);
    D(bug("[PCI.PC] %s: Registering Driver with PCI base class...\n", __func__));

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    OOP_DoMethod(pci, (OOP_Msg)pmsg);
    OOP_DisposeObject(pci);

    D(bug("[PCI.PC] %s: Driver initialization finished\n", __func__));

    return TRUE;
}

static int PCPCI_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[PCI.PC] %s()\n", __func__));

    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);

    if (ACPICABase)
        CloseLibrary(ACPICABase);

    return TRUE;
}
	
ADD2INITLIB(PCPCI_InitClass, 0)
ADD2EXPUNGELIB(PCPCI_ExpungeClass, 0)

