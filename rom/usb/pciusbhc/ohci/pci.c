/*
   Copyright © 2002-2009, Chris Hodges. All rights reserved.
   Copyright © 2009-2012, The AROS Development Team. All rights reserved.
   $Id$
*/

#include <aros/bootloader.h>
#include <exec/types.h>
#include <oop/oop.h>
#include <devices/timer.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <resources/acpi.h>

#include <proto/acpi.h>
#include <proto/bootloader.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>

#include <string.h>

#include "debug.h"
#include "chip.h"
#include "dev.h"

#include "chip_protos.h"
#include "cmd_protos.h"

#ifdef __i386__
#define HAVE_ACPI
#endif
#ifdef __x86_64__
#define HAVE_ACPI
#endif

#undef HiddPCIDeviceAttrBase
#undef HiddAttrBase

#define HiddPCIDeviceAttrBase (hd->hd_HiddPCIDeviceAB)
#define HiddAttrBase (hd->hd_HiddAB)

static TEXT product_name[] = "PCI OHCI USB 1.1 Host Controller";

AROS_UFH3(void, pciEnumerator,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, pciDevice, A2), AROS_UFHA(APTR, message, A1))
{
    AROS_USERFUNC_INIT

    struct PCIDevice *hd = (struct PCIDevice *)hook->h_Data;
    struct PCIController *hc;
    IPTR bus;
    IPTR dev;
    IPTR sub;
    IPTR intline;
    ULONG devid;
    UWORD i;

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Bus, &bus);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Dev, &dev);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Sub, &sub);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &intline);

    devid = (bus << 16) | dev;

    KPRINTF(10, ("Found PCI device 0x%lx Intline=%ld\n", devid, intline));

    if (intline == 255)
    {
        // we can't work without the correct interrupt line.
        // BIOS needs plug & play os option disabled.
        // Alternatively AROS must support APIC reconfiguration
        KPRINTF(200, ("ERROR: PCI card has no interrupt line assigned "
            "by BIOS, disable Plug & Play OS!\n"));
    }
    else
    {
        KPRINTF(10, ("Setting up device...\n"));

        hc = AllocPooled(hd->hd_MemPool, sizeof(struct PCIController));
        if (hc)
        {
            hc->hc_Device = hd;
            hc->hc_DevID = devid;
            hc->hc_FunctionNum = sub;
            hc->hc_PCIDeviceObject = pciDevice;
            hc->hc_PCIIntLine = intline;

            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver,
                (IPTR *) & hc->hc_PCIDriverObject);

            for (i = 0; i < XFER_COUNT; i++)
                NewList(&hc->hc_XferQueues[i]);
            NewList(&hc->hc_TDQueue);
            NewList(&hc->hc_AbortQueue);
            NewList(&hc->hc_PeriodicTDQueue);
            NewList(&hc->hc_RetireQueue);
            AddTail(&hd->hd_TempHCIList, &hc->hc_Node);
        }
    }

    AROS_USERFUNC_EXIT
}

/* /// "pciInit()" */
BOOL pciInit(struct PCIDevice *hd)
{
    struct PCIController *hc;
    struct PCIController *nexthc;
    struct PCIUnit *hu;
    ULONG unitno = 0;

    KPRINTF(10, ("*** pciInit(%p) ***\n", hd));

    NewList(&hd->hd_TempHCIList);

    if ((hd->hd_PCIHidd =
            OOP_NewObject(NULL, (STRPTR) CLID_Hidd_PCI, NULL)))
    {
        struct TagItem tags[] = {
            {tHidd_PCI_Class, (PCI_CLASS_SERIAL_USB >> 8) & 0xff},
            {tHidd_PCI_SubClass, (PCI_CLASS_SERIAL_USB & 0xff)},
            {tHidd_PCI_Interface, 0x10},
            {TAG_DONE, 0UL}
        };

        struct OOP_ABDescr attrbases[] = {
            {(STRPTR) IID_Hidd, &hd->hd_HiddAB},
            {(STRPTR) IID_Hidd_PCIDevice, &hd->hd_HiddPCIDeviceAB},
            {NULL, NULL}
        };

        struct Hook findHook = {
          h_Entry:(IPTR(*)())pciEnumerator,
          h_Data:hd,
        };

        OOP_ObtainAttrBases(attrbases);

        KPRINTF(20, ("Searching for devices...\n"));

        HIDD_PCI_EnumDevices(hd->hd_PCIHidd, &findHook,
            (struct TagItem *)&tags);
    }
    else
    {
        KPRINTF(20, ("Unable to create PCIHidd object!\n"));
        return FALSE;
    }

    // Create units with a list of host controllers having the same bus
    // and device number.
    while (hd->hd_TempHCIList.lh_Head->ln_Succ)
    {
        hu = AllocPooled(hd->hd_MemPool, sizeof(struct PCIUnit));
        if (!hu)
        {
            // actually, we should get rid of the allocated memory first,
            // but we don't care as DeletePool() will take care of this
            // eventually
            return FALSE;
        }
        hu->hu_Device = hd;
        hu->hu_UnitNo = unitno;
        hu->hu_DevID =
            ((struct PCIController *)hd->hd_TempHCIList.lh_Head)->hc_DevID;

        NewList(&hu->hu_Controllers);
        NewList(&hu->hu_RHIOQueue);

        hc = (struct PCIController *)hd->hd_TempHCIList.lh_Head;
        while ((nexthc = (struct PCIController *)hc->hc_Node.ln_Succ))
        {
            if (hc->hc_DevID == hu->hu_DevID)
            {
                Remove(&hc->hc_Node);
                hc->hc_Unit = hu;
                AddTail(&hu->hu_Controllers, &hc->hc_Node);
            }
            hc = nexthc;
        }
        AddTail(&hd->hd_Units, (struct Node *)hu);
        unitno++;
    }
    return TRUE;
}
/* \\\ */

/* /// "pciAllocUnit()" */
BOOL pciAllocUnit(struct PCIUnit * hu)
{
    struct PCIController *hc;

    BOOL allocgood = TRUE;
    ULONG usb11ports = 0;
    ULONG usb20ports = 0;
    ULONG cnt;

    ULONG ohcicnt = 0;

    KPRINTF(10, ("*** pciAllocUnit(%p) ***\n", hu));

    // allocate necessary memory
    hc = (struct PCIController *)hu->hu_Controllers.lh_Head;
    while (hc->hc_Node.ln_Succ)
    {
        allocgood = InitController(hc, hu);
        if (allocgood)
        {
            ohcicnt++;
        }

        hc = (struct PCIController *)hc->hc_Node.ln_Succ;
    }

    if (!allocgood)
    {
        return FALSE;
    }

    hc = (struct PCIController *)hu->hu_Controllers.lh_Head;
    while (hc->hc_Node.ln_Succ)
    {
        if (hc->hc_complexrouting)
        {
            ULONG locport = 0;
            for (cnt = 0; cnt < usb20ports; cnt++)
            {
                if (((hc->hc_portroute >> (cnt << 2)) & 0xf) ==
                    hc->hc_FunctionNum)
                {
                    KPRINTF(10,
                        ("CHC %ld Port %ld assigned to global Port %ld\n",
                            hc->hc_FunctionNum, locport, cnt));
                    hu->hu_PortMap11[cnt] = hc;
                    hu->hu_PortNum11[cnt] = locport;
                    hc->hc_PortNum20[locport] = cnt;
                    locport++;
                }
            }
        }
        else
        {
            for (cnt = usb11ports; cnt < usb11ports + hc->hc_NumPorts;
                cnt++)
            {
                hu->hu_PortMap11[cnt] = hc;
                hu->hu_PortNum11[cnt] = cnt - usb11ports;
                hc->hc_PortNum20[cnt - usb11ports] = cnt;
            }
        }
        usb11ports += hc->hc_NumPorts;
        hc = (struct PCIController *)hc->hc_Node.ln_Succ;
    }
    if ((usb11ports != usb20ports) && usb20ports)
    {
        KPRINTF(20,
            ("Warning! #EHCI Ports (%ld) does not match USB 1.1 Ports (%ld)!\n",
                usb20ports, usb11ports));
    }

    hu->hu_RootHub11Ports = usb11ports;
    hu->hu_RootHub20Ports = usb20ports;
    hu->hu_RootHubPorts =
        (usb11ports > usb20ports) ? usb11ports : usb20ports;
    for (cnt = 0; cnt < hu->hu_RootHubPorts; cnt++)
    {
        hu->hu_EhciOwned[cnt] = hu->hu_PortMap20[cnt] ? TRUE : FALSE;
    }

    KPRINTF(10, ("Unit %ld: USB Board %08lx has %ld USB1.1 ports\n",
            hu->hu_UnitNo, hu->hu_DevID, hu->hu_RootHub11Ports));

    hu->hu_FrameCounter = 1;
    hu->hu_RootHubAddr = 0;

    // put em online
    hc = (struct PCIController *)hu->hu_Controllers.lh_Head;
    while (hc->hc_Node.ln_Succ)
    {
        hc->hc_Flags |= HCF_ONLINE;
        hc = (struct PCIController *)hc->hc_Node.ln_Succ;
    }

    // create product name of device
    CopyMem(product_name, hu->hu_ProductName, sizeof(product_name));

    KPRINTF(10, ("Unit allocated!\n"));

    return TRUE;
}
/* \\\ */

/* /// "pciFreeUnit()" */
void pciFreeUnit(struct PCIUnit *hu)
{
    struct PCIDevice *hd = hu->hu_Device;
    struct PCIController *hc;

    struct TagItem pciDeactivate[] = {
        {aHidd_PCIDevice_isIO, FALSE},
        {aHidd_PCIDevice_isMEM, FALSE},
        {aHidd_PCIDevice_isMaster, FALSE},
        {TAG_DONE, 0UL},
    };

    KPRINTF(10, ("*** pciFreeUnit(%p) ***\n", hu));

    // put em offline
    hc = (struct PCIController *)hu->hu_Controllers.lh_Head;
    while (hc->hc_Node.ln_Succ)
    {
        hc->hc_Flags &= ~HCF_ONLINE;
        hc = (struct PCIController *)hc->hc_Node.ln_Succ;
    }

    FreeController(hc, hu);

    // FIXME: (x/e/o/u)hciFree routines actually ONLY stops the chip NOT
    // free anything as below...
    hc = (struct PCIController *)hu->hu_Controllers.lh_Head;
    while (hc->hc_Node.ln_Succ)
    {
        if (hc->hc_PCIMem)
        {
            HIDD_PCIDriver_FreePCIMem(hc->hc_PCIDriverObject,
                hc->hc_PCIMem);
            hc->hc_PCIMem = NULL;
        }
        hc = (struct PCIController *)hc->hc_Node.ln_Succ;
    }

    // disable and free board
    hc = (struct PCIController *)hu->hu_Controllers.lh_Head;
    while (hc->hc_Node.ln_Succ)
    {
        // deactivate busmaster and IO/Mem
        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *)pciDeactivate);
        if (hc->hc_PCIIntHandler.is_Node.ln_Name)
        {
            RemIntServer(INTB_KERNEL + hc->hc_PCIIntLine,
                &hc->hc_PCIIntHandler);
            hc->hc_PCIIntHandler.is_Node.ln_Name = NULL;
        }
        hc = (struct PCIController *)hc->hc_Node.ln_Succ;
    }
}
/* \\\ */

/* /// "pciExpunge()" */
void pciExpunge(struct PCIDevice *hd)
{
    struct PCIController *hc;
    struct PCIUnit *hu;

    KPRINTF(10, ("*** pciExpunge(%p) ***\n", hd));

    hu = (struct PCIUnit *)hd->hd_Units.lh_Head;
    while (((struct Node *)hu)->ln_Succ)
    {
        Remove((struct Node *)hu);
        hc = (struct PCIController *)hu->hu_Controllers.lh_Head;
        while (hc->hc_Node.ln_Succ)
        {
            Remove(&hc->hc_Node);
            FreePooled(hd->hd_MemPool, hc, sizeof(struct PCIController));
            hc = (struct PCIController *)hu->hu_Controllers.lh_Head;
        }
        FreePooled(hd->hd_MemPool, hu, sizeof(struct PCIUnit));
        hu = (struct PCIUnit *)hd->hd_Units.lh_Head;
    }
    if (hd->hd_PCIHidd)
    {
        struct OOP_ABDescr attrbases[] = {
            {(STRPTR) IID_Hidd, &hd->hd_HiddAB},
            {(STRPTR) IID_Hidd_PCIDevice, &hd->hd_HiddPCIDeviceAB},
            {NULL, NULL}
        };

        OOP_ReleaseAttrBases(attrbases);

        OOP_DisposeObject(hd->hd_PCIHidd);
    }
}
/* \\\ */

/* /// "pciGetPhysical()" */
APTR pciGetPhysical(struct PCIController *hc, APTR virtaddr)
{
    //struct PCIDevice *hd = hc->hc_Device;
    return HIDD_PCIDriver_CPUtoPCI(hc->hc_PCIDriverObject, virtaddr);
}
/* \\\ */

/*
 * Process some AROS-specific arguments.
 * 'usbpoweron' helps to bring up USB ports on IntelMac,
 * whose firmware sets them up incorrectly.
 */
static int getArguments(struct PCIDevice *base)
{
    APTR BootLoaderBase;
#ifdef HAVE_ACPI
    struct ACPIBase *ACPIBase;

    ACPIBase = OpenResource("acpi.resource");
    if (ACPIBase)
    {
        /*
         * Use ACPI IDs to identify known machines which need HDF_FORCEPOWER
         * to work. Currently we know only MacMini.
         */
        struct ACPI_TABLE_DEF_HEADER *dsdt =
            ACPI_FindSDT(ACPI_MAKE_ID('D', 'S', 'D', 'T'));

        if (dsdt)
        {
            /* Yes, the last byte in ID is zero */
            if (strcmp(dsdt->oem_table_id, "Macmini") == 0)
            {
                base->hd_Flags = HDF_FORCEPOWER;
                return TRUE;
            }
        }
    }
#endif

    BootLoaderBase = OpenResource("bootloader.resource");
    if (BootLoaderBase)
    {
        struct List *args = GetBootInfo(BL_Args);

        if (args)
        {
            struct Node *node;

            for (node = args->lh_Head; node->ln_Succ; node = node->ln_Succ)
            {
                if (stricmp(node->ln_Name, "forceusbpower") == 0)
                {
                    base->hd_Flags = HDF_FORCEPOWER;
                    break;
                }
            }
        }
    }

    return TRUE;
}

ADD2INITLIB(getArguments, 10)
