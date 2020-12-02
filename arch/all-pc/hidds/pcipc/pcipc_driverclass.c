/*
    Copyright © 2004-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI direct bus driver, for i386/x86_64 native.
    Lang: English
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/acpica.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hardware/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

#include <string.h>

#include "pcipc.h"

#define DMMIO(x)

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

const char pcipcHWPCI[] = "IA32-native Direct Access PCI Bus Controller";
const char pcipcHWPCIE[] = "IA32-native PCI Express Controller";

struct acpiHostBridge
{
    struct Node         ahb_Node;
    struct MinList      ahb_irqRoutingTable;
};

/*
    We overload the New method in order to introduce the HardwareName attributes.
*/
OOP_Object *PCIPC__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct MinList *routingtable = (struct MinList *)GetTagData(aHidd_PCIDriver_IRQRoutingTable, 0, msg->attrList);
    struct pRoot_New ncMsg;
    struct TagItem ncTags[] =
    {
        { aHidd_Name,           (IPTR)"pcipc.hidd"			        },
        { TAG_DONE,             0 					        }
    };
    IPTR mmbase = 0;
    OOP_Object *controllerObj;

    D(bug("[PCIPC:Driver] %s: IRQ Routing Table @ 0x%p\n", __func__, routingtable);)

    ncMsg.mID      = msg->mID;
    ncMsg.attrList = ncTags;

    if (msg->attrList)
    {
        ncTags[1].ti_Tag  = TAG_MORE;
        ncTags[1].ti_Data = (IPTR)msg->attrList;
    }

    controllerObj = (OOP_Object *)OOP_DoSuperMethod(cl, o, &ncMsg.mID);
    if (controllerObj)
    {
        struct PCIPCBusData *data = OOP_INST_DATA(cl, controllerObj);
        struct Node *tmp1, *tmp2;

        D(bug("[PCIPC:Driver] %s: controller Object created @ 0x%p\n", __func__, controllerObj);)

        NEWLIST(&data->irqRoutingTable);
        if (routingtable)
        {
            D(bug("[PCIPC:Driver] %s: transferring IRQ routing information..\n", __func__);)
            ForeachNodeSafe(routingtable, tmp1, tmp2)
            {
                Remove(tmp1);
                ADDTAIL(&data->irqRoutingTable, tmp1);
            }
        }
    }
    D(bug("[PCIPC:Driver] %s: returning 0x%p\n", __func__, controllerObj);)

    return controllerObj;
}

void PCIPC__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct PCIPCBusData *data = OOP_INST_DATA(cl, o);
    struct pcipc_staticdata *psd = PSD(cl);
    ULONG idx;
    BOOL handled = FALSE;

    if (IS_HIDD_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_HardwareName:
                handled = TRUE;
                if (!data->ecam)
                    *msg->storage = (IPTR)pcipcHWPCI;
                else
                    *msg->storage = (IPTR)pcipcHWPCIE;
                break;
        }
    }        
    else if (IS_PCIDRV_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_PCIDriver_DeviceClass:
                handled = TRUE;
                *msg->storage = (IPTR)psd->pcipcDeviceClass;
                break;

            case aoHidd_PCIDriver_IRQRoutingTable:
                handled = TRUE;
                if (IsListEmpty(&data->irqRoutingTable))
                    *msg->storage = (IPTR)NULL;
                else
                    *msg->storage = (IPTR)&data->irqRoutingTable;
                break;
        }
    }
    if (!handled)
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

ULONG PCIPC__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, 
                                            struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    struct PCIPCBusData *data = OOP_INST_DATA(cl, o);
    struct PCIPCDeviceData *devdata = OOP_INST_DATA(PSD(cl)->pcipcDeviceClass, msg->device);

    if ((data->ecam && devdata->mmconfig) ||
        (devdata->mmconfig && (msg->reg < 0x100)))
    {
        /* use MMIO long read access for ECAM */
        ULONG configLong = *(ULONG volatile *) (devdata->mmconfig + (msg->reg & 0xffc));

        DMMIO(bug("[PCIPC:Driver] %s: MMIO Read @ %p = %08x\n", __func__, (devdata->mmconfig + (msg->reg & 0xffc)), configLong));

        return configLong;
    }
    else
    {
        /*
            Use legacy access if MMIO isnt possible.
        */
        return PSD(cl)->ReadConfigLong(msg->bus, msg->dev, msg->sub, msg->reg);
    }
}

UWORD PCIPC__Hidd_PCIDriver__ReadConfigWord(OOP_Class *cl, OOP_Object *o, 
                                            struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    return ReadConfigWord(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
}

UBYTE PCIPC__Hidd_PCIDriver__ReadConfigByte(OOP_Class *cl, OOP_Object *o, 
                                            struct pHidd_PCIDriver_ReadConfigByte *msg)
{
    pcicfg temp;

    temp.ul = PSD(cl)->ReadConfigLong(msg->bus, msg->dev, msg->sub, (msg->reg & ~3)); 
    return temp.ub[msg->reg & 3];
}

void PCIPC__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    struct PCIPCBusData *data = OOP_INST_DATA(cl, o);
    struct PCIPCDeviceData *devdata = OOP_INST_DATA(PSD(cl)->pcipcDeviceClass, msg->device);

    if ((data->ecam && devdata->mmconfig) ||
        (devdata->mmconfig && (msg->reg < 0x100)))
    {
       /* use MMIO long read access for ECAM */
        ULONG volatile *longreg;
        longreg = (ULONG volatile *) (devdata->mmconfig + (msg->reg & 0xffc));
        DMMIO(bug("[PCIPC:Driver] %s: MMIO Write %08x @ %p\n", __func__, msg->val, longreg));
        *longreg = msg->val;
    }
    else
    {
        /*
            Use legacy access if MMIO isnt possible.
        */
        PSD(cl)->WriteConfigLong(msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
    }
}

#undef _psd

/* Class initialization and destruction */

/* Parse an individual routing entry */
static void EnumPCIIRQ(struct pcipc_staticdata *psd, struct acpiHostBridge *ahb,
    ACPI_PCI_ROUTING_TABLE *item, UWORD bus_num)
{
    struct PCI_IRQRoutingEntry *n =
        AllocVec(sizeof(struct PCI_IRQRoutingEntry), MEMF_CLEAR | MEMF_ANY);

    if (n)
    {
        n->re_PCIBusNum = bus_num;
        n->re_PCIDevNum = (item->Address >> 16) & 0xFFFF;

        D(bug("[PCIPC:Driver] %s:  %02x:%02x.x", __func__, n->re_PCIBusNum,
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
            ADDTAIL(&ahb->ahb_irqRoutingTable, n);
        }
    }
}

static void FindIRQRouting(struct pcipc_staticdata *psd, struct acpiHostBridge *ahb, ACPI_HANDLE parent,
    UBYTE bus_num)
{
    ACPI_HANDLE child = NULL;
    ACPI_DEVICE_INFO *dev_info;
    ACPI_BUFFER buffer;
    ACPI_PCI_ROUTING_TABLE *entry;
    UBYTE dev_num, func_num, child_bus_num;
    BOOL is_bridge;
    ULONG address;

    D(bug("[PCIPC:Driver] %s: Scanning bus %d\n", __func__, bus_num);)

    /* Get routing table for current bus */
    buffer.Length = ACPI_ALLOCATE_BUFFER;
    if (AcpiGetIrqRoutingTable(parent, &buffer) == AE_OK)
    {
        D(bug("[PCIPC:Driver] %s: Found _PRT\n", __func__);)
        /* Translate routing table entries into nodes for our own list */
        for (entry = buffer.Pointer; entry->Length != 0;
            entry = (APTR)entry + entry->Length)
        {
            EnumPCIIRQ(psd, ahb, entry, bus_num);
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

                /* Check if this is a PCI-PCI bridge */
                is_bridge = ReadConfigWord(psd, bus_num, dev_num, func_num,
                    PCIBR_SUBCLASS) == 0x0604;

                /* Look for more routing tables */
                if (is_bridge)
                {
                    D(bug("[PCIPC:Driver] %s: Found a bridge at %02x:%02x.%x\n",
                        __func__, bus_num, dev_num, func_num);)

                    /* Get this bridge's bus number */
                    child_bus_num = ReadConfigByte(psd, bus_num, dev_num, func_num,
                        PCIBR_SECBUS);
                    FindIRQRouting(psd, ahb, child, child_bus_num);
                }
            }
            FreeVec(dev_info);
        }
    }

    return;
}

#undef UtilityBase
#define UtilityBase (psd->utilityBase)

static unsigned int ACPIHostBridgeBusNoCallback(struct acpi_resource *resource, void *data)
{
    D(bug("[PCIPC:Driver] %s()\n", __func__);)
    if ((resource->Length > 0) && (resource->Type == ACPI_BUS_NUMBER_RANGE))
    {
        LONG *bus = (ULONG *)data;
        D(bug("[PCIPC:Driver] %s: Bus Range = %u->%u\n", __func__, resource->Data.Address64.Address.Minimum, resource->Data.Address64.Address.Maximum);)
        *bus = (LONG)resource->Data.Address64.Address.Minimum;
    }
    return AE_OK;
}

static void ACPIHostBridgeEval(ACPI_HANDLE handle, struct acpiHostBridge *hbNode)
{
    ACPI_BUFFER buffer;
    ACPI_STATUS Status;
    buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;
    LONG busNo = -1;

    Status = AcpiEvaluateObject(handle, METHOD_NAME__BBN,
        NULL, &buffer);
    if (!ACPI_FAILURE (Status))
    {
        D(
            bug("[PCIPC:Driver] %s: _BBN method returned (%p)\n",
                    __func__, buffer.Pointer);
            bug("[PCIPC:Driver] %s:     = %p\n",
                    __func__, *(ULONG *)buffer.Pointer);
        )
        hbNode->ahb_Node.ln_Type = (UBYTE)(*(LONG *)buffer.Pointer);
        FreeVec(buffer.Pointer);
        buffer.Pointer = NULL;
    }

    /* Check the devices resource settings for the Bus number .. */
    Status = AcpiWalkResources(handle, METHOD_NAME__CRS, ACPIHostBridgeBusNoCallback, &busNo);
    if ((!ACPI_FAILURE (Status)) && (busNo != -1))
    {
        D(bug("[PCIPC:Driver] %s: CRS returned bus #%u\n", __func__, (UBYTE)busNo);)
    }

    if ((hbNode->ahb_Node.ln_Type != (UBYTE)busNo) && (busNo != -1))
        hbNode->ahb_Node.ln_Type = (UBYTE)busNo;

    /* Attempt to find the ECAM address, if this is a
     * hot plug capable host bridge */
    buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;
    Status = AcpiEvaluateObject(handle, METHOD_NAME__CBA,
        NULL, &buffer);
    if (!ACPI_FAILURE (Status))
    {
        D(
            bug("[PCIPC:Driver] %s: _CBA method returned\n",
                    __func__);
            bug("[PCIPC:Driver] %s:     = %p\n",
                    __func__, *(ULONG *)buffer.Pointer);
        )
        FreeVec(buffer.Pointer);
        buffer.Pointer = NULL;
        /* Attempt to find the segment number ... */
        buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;
        Status = AcpiEvaluateObject(handle, METHOD_NAME__SEG,
            NULL, &buffer);
        if (!ACPI_FAILURE (Status))
        {
            D(
                bug("[PCIPC:Driver] %s: _SEG method returned\n",
                        __func__);
                bug("[PCIPC:Driver] %s:     = %p\n",
                        __func__, *(ULONG *)buffer.Pointer);
            )
            FreeVec(buffer.Pointer);
            buffer.Pointer = NULL;
        }
        else
        {
            D(bug("[PCIPC:Driver] %s: _SEG failed (%08x)\n", __func__, Status);)
        }
    }
    else
    {
        D(bug("[PCIPC:Driver] %s: _CBA failed (%08x)\n", __func__, Status);)
    }
}

/* Parse a routing table */
static ACPI_STATUS ACPIHostBridgeCallbackA(ACPI_HANDLE handle, ULONG nesting_level,
    void *context, void **return_value)
{
    struct pcipc_staticdata *psd = (struct pcipc_staticdata *)context;
    struct acpiHostBridge *hbNode, *tmp;
    ACPI_DEVICE_INFO *hbInfo;
    ACPI_STATUS Status;

    D(bug("[PCIPC:Driver] %s: Host bridge handle @ 0x%p\n", __func__, handle);)

    /* Get device:function part of PCI address */
    Status = AcpiGetObjectInfo(handle, &hbInfo);
    if (!ACPI_FAILURE (Status))
    {
        if ((hbInfo->Valid & (ACPI_VALID_ADR | ACPI_VALID_HID)) == (ACPI_VALID_ADR | ACPI_VALID_HID))
        {
            BOOL found = FALSE;
            
            hbNode = AllocVec(sizeof(struct acpiHostBridge) + hbInfo->HardwareId.Length + 1, MEMF_CLEAR);
            hbNode->ahb_Node.ln_Name = (APTR)((IPTR)hbNode + sizeof(struct acpiHostBridge));
            CopyMem(hbInfo->HardwareId.String, hbNode->ahb_Node.ln_Name, hbInfo->HardwareId.Length);
            hbNode->ahb_Node.ln_Type = -1;
            NEWLIST(&hbNode->ahb_irqRoutingTable);

            D(bug("[PCIPC:Driver] %s: host bridge hid '%s' (type %u)\n",
                __func__, hbNode->ahb_Node.ln_Name, hbInfo->Type);)

            ACPIHostBridgeEval(handle, hbNode);

            if (hbNode->ahb_Node.ln_Type == -1)
                hbNode->ahb_Node.ln_Type = 1;
            hbNode->ahb_Node.ln_Pri = -hbNode->ahb_Node.ln_Type;
            D(bug("[PCIPC:Driver] %s: bus id #%u\n", __func__, hbNode->ahb_Node.ln_Type);)
            ForeachNode((struct List *)return_value, tmp)
            {
                if ((!Stricmp(tmp->ahb_Node.ln_Name, hbNode->ahb_Node.ln_Name)) &&
                    (tmp->ahb_Node.ln_Type == hbNode->ahb_Node.ln_Type))
                {
                    found = TRUE;
                }
            }

            if (!found)
            {
                FindIRQRouting(psd, hbNode, handle, (hbInfo->Address >> 20) & 0xff);

                Enqueue((struct List *)return_value, &hbNode->ahb_Node);
            }
            else
            {
                D(bug("[PCIPC:Driver] %s: host bridge (bus id %u) already registered!\n",
                        __func__, hbNode->ahb_Node.ln_Type);)
                FreeVec(hbNode);
            }
        }
        FreeVec(hbInfo);
    }

    return AE_OK;
}

#undef OOPBase
#undef HiddPCIDriverAttrBase
#define HiddPCIDriverAttrBase (_psd->hiddPCIDriverAB)

static int PCIPC_InitClass(LIBBASETYPEPTR LIBBASE)
{
    struct Library *OOPBase = LIBBASE->psd.OOPBase;
    struct pcipc_staticdata *_psd = &LIBBASE->psd;
    struct pHidd_PCI_AddHardwareDriver msg, *pmsg = &msg;
    struct List acpiHBridges;
    ACPI_STATUS status;
    OOP_Object *pci;

    D(bug("[PCIPC:Driver] %s()\n", __func__));

    D(bug("[PCIPC:Driver] %s: ACPI Host Bridge list @ 0x%p\n", __func__, &acpiHBridges);)

    _psd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    _psd->hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    _psd->hidd_PCIDeviceAB = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (_psd->hiddPCIDriverAB == 0 || _psd->hiddAB == 0 || _psd->hidd_PCIDeviceAB == 0)
    {
        bug("[PCIPC:Driver] %s: ObtainAttrBases failed\n", __func__);
        return FALSE;
    }

    /* Open ACPI and cache the pointer to the MCFG table */
    ACPICABase = OpenLibrary("acpica.library", 0);
    if (ACPICABase)
    {
        AcpiGetTable(ACPI_SIG_MCFG, 1, (ACPI_TABLE_HEADER **)&_psd->pcipc_acpiMcfgTbl);
        if (ACPI_FAILURE(status)) {
            bug("[PCIPC:Driver] %s: No ACPI MCFG table\n", __func__);
            /* not a critical failure .. */
        }
    }

    /* Default to using config mechanism 1 */
    _psd->ReadConfigLong  = ReadConfig1Long;
    _psd->WriteConfigLong = WriteConfig1Long;
    PCIPC_ProbeConfMech(&LIBBASE->psd);

    /* Find ACPI PCI Host Bridges and their routing tables */
    if (ACPICABase)
    {
        NEWLIST(&acpiHBridges);
        status = AcpiGetDevices("PNP0A03", ACPIHostBridgeCallbackA, _psd, (void **)&acpiHBridges);
        if (ACPI_FAILURE(status)) {
            D(bug("[PCIPC:Driver] %s: No PNP0A03 host bridge information available\n", __func__);)
            /* not a critical failure .. */
        }

        status = AcpiGetDevices("PNP0A08", ACPIHostBridgeCallbackA, _psd, (void **)&acpiHBridges);
        if (ACPI_FAILURE(status)) {
            D(bug("[PCIPC:Driver] %s: No PNP0A08 host bridge information available\n", __func__);)
            /* not a critical failure .. */
        }
    }

    if ((pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL)) != NULL)
    {
        D(bug("[PCIPC:Driver] %s: PCI base class @ 0x%p\n", __func__, pci));
        msg.driverClass = _psd->pcipcDriverClass;
        msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);
        if (!ACPICABase || IsListEmpty(&acpiHBridges))
        {
            D(bug("[PCIPC:Driver] %s: Registering legacy Driver Instance with PCI base class...\n", __func__));
            OOP_DoMethod(pci, (OOP_Msg)pmsg);
        }
        else
        {
            struct acpiHostBridge *hbNode, *tmpNode;
            struct TagItem ptags[] =
            {
                { aHidd_PCIDriver_IRQRoutingTable,      0 },
                { TAG_DONE,                             0 }  
            };            
            msg.instanceTags = ptags;
            ForeachNodeSafe(&acpiHBridges, hbNode, tmpNode)
            {
                Remove(&hbNode->ahb_Node);
                ptags[0].ti_Data = (IPTR)&hbNode->ahb_irqRoutingTable;
                D(bug("[PCIPC:Driver] %s: Registering ACPI Driver Instance(s) with PCI base class...\n", __func__);)
                OOP_DoMethod(pci, (OOP_Msg)pmsg);
                FreeVec(hbNode);
            }
            D(bug("[PCIPC:Driver] %s: ACPI Instance(s) Registered\n", __func__);)
        }
        OOP_DisposeObject(pci);
        D(bug("[PCIPC:Driver] %s: Driver initialization finished\n", __func__);)
        return TRUE;
    }

    D(bug("[PCIPC:Driver] %s: PCI unavailable!\n", __func__));

    return FALSE;
}

static int PCIPC_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    struct Library *OOPBase = LIBBASE->psd.OOPBase;

    D(bug("[PCIPC:Driver] %s()\n", __func__));

    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);

    if (ACPICABase)
        CloseLibrary(ACPICABase);

    return TRUE;
}

ADD2INITLIB(PCIPC_InitClass, 10)
ADD2EXPUNGELIB(PCIPC_ExpungeClass, 0)

