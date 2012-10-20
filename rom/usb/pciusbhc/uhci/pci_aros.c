/* pci_aros.c - pci access abstraction for AROS by Chris Hodges
*/

#include <aros/bootloader.h>
#include <aros/symbolsets.h>
#include <exec/types.h>
#include <oop/oop.h>
#include <devices/timer.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <proto/bootloader.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <string.h>

#include "uhwcmd.h"

#undef HiddPCIDeviceAttrBase
#undef HiddAttrBase

#define HiddPCIDeviceAttrBase (hd->hd_HiddPCIDeviceAB)
#define HiddAttrBase (hd->hd_HiddAB)

#define PCI_BASE_CLASS_SERIAL   0x0c
#define PCI_SUB_CLASS_USB       0x03
#define PCI_INTERFACE_UHCI      0x00

AROS_UFH3(void, pciEnumerator,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(OOP_Object *, pciDevice, A2),
          AROS_UFHA(APTR, message, A1))
{
    AROS_USERFUNC_INIT

    struct PCIDevice *hd = (struct PCIDevice *) hook->h_Data;
    struct PCIController *hc;
    IPTR hcitype;
    IPTR bus;
    IPTR dev;
    IPTR sub;
    IPTR intline;
    ULONG devid;

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Interface, &hcitype);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Bus, &bus);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Dev, &dev);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Sub, &sub);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &intline);

    devid = (bus<<16)|dev;

    KPRINTF(10, ("Found PCI device 0x%lx of type %ld, Intline=%ld\n", devid, hcitype, intline));

    if(intline == 255) {
        // we can't work without the correct interrupt line
        // BIOS needs plug & play os option disabled. Alternatively AROS must support APIC reconfiguration
        KPRINTF(200, ("ERROR: PCI card has no interrupt line assigned by BIOS, disable Plug & Play OS!\n"));
    }else{
	    KPRINTF(10, ("Setting up device...\n"));

        hc = AllocPooled(hd->hd_MemPool, sizeof(struct PCIController));
        if (hc) {
            hc->hc_Device = hd;
          	hc->hc_DevID = devid;
           	hc->hc_FunctionNum = sub;
           	hc->hc_HCIType = hcitype;
           	hc->hc_PCIDeviceObject = pciDevice;
           	hc->hc_PCIIntLine = intline;

           	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (IPTR *) &hc->hc_PCIDriverObject);

           	NEWLIST(&hc->hc_CtrlXFerQueue);
           	NEWLIST(&hc->hc_IntXFerQueue);
           	NEWLIST(&hc->hc_IsoXFerQueue);
           	NEWLIST(&hc->hc_BulkXFerQueue);

           	NEWLIST(&hc->hc_TDQueue);
           	NEWLIST(&hc->hc_AbortQueue);
           	NEWLIST(&hc->hc_PeriodicTDQueue);

           	AddTail(&hd->hd_TempHCIList, &hc->hc_Node);
        }
    }

    AROS_USERFUNC_EXIT
}

BOOL pciInit(struct PCIDevice *hd) {
    struct PCIController *hc;
    struct PCIController *nexthc;
    struct PCIUnit *hu;
    ULONG unitno = 0;

    KPRINTF(10, ("*** pciInit(%p) ***\n", hd));

    NEWLIST(&hd->hd_TempHCIList);

    if((hd->hd_PCIHidd = OOP_NewObject(NULL, (STRPTR) CLID_Hidd_PCI, NULL))) {
        struct TagItem tags[] = {
            { tHidd_PCI_Class,      PCI_BASE_CLASS_SERIAL },
            { tHidd_PCI_SubClass,   PCI_SUB_CLASS_USB },
            { tHidd_PCI_Interface,  PCI_INTERFACE_UHCI },
            { TAG_DONE, 0UL }
        };

        struct OOP_ABDescr attrbases[] = {
            { (STRPTR) IID_Hidd,            &hd->hd_HiddAB },
            { (STRPTR) IID_Hidd_PCIDevice,  &hd->hd_HiddPCIDeviceAB },
            { NULL, NULL }
        };

        struct Hook findHook = {
             h_Entry:        (IPTR (*)()) pciEnumerator,
             h_Data:         hd,
        };

        OOP_ObtainAttrBases(attrbases);

        KPRINTF(20, ("Searching for devices...\n"));

        HIDD_PCI_EnumDevices(hd->hd_PCIHidd, &findHook, (struct TagItem *) &tags);
    } else {
        KPRINTF(20, ("Unable to create PCIHidd object!\n"));
        return FALSE;
    }

    // Create units with a list of host controllers having the same bus and device number.
    while(hd->hd_TempHCIList.lh_Head->ln_Succ) {
        hu = AllocPooled(hd->hd_MemPool, sizeof(struct PCIUnit));
        if(!hu) {
            // actually, we should get rid of the allocated memory first, but I don't care as DeletePool() will take care of this eventually
            return FALSE;
        }
        hu->hu_Device = hd;
        hu->hu_UnitNo = unitno;
        hu->hu_DevID = ((struct PCIController *) hd->hd_TempHCIList.lh_Head)->hc_DevID;

        NEWLIST(&hu->hu_Controllers);
        NEWLIST(&hu->hu_RHIOQueue);

        hc = (struct PCIController *) hd->hd_TempHCIList.lh_Head;
        while((nexthc = (struct PCIController *) hc->hc_Node.ln_Succ)) {
            if(hc->hc_DevID == hu->hu_DevID) {
                Remove(&hc->hc_Node);
                hc->hc_Unit = hu;
                AddTail(&hu->hu_Controllers, &hc->hc_Node);
            }
            hc = nexthc;
        }
        AddTail(&hd->hd_Units, (struct Node *) hu);
        unitno++;
    }
    return TRUE;
}

BOOL pciAllocUnit(struct PCIUnit *hu) {

    struct PCIController *hc;

    BOOL allocgood = TRUE;
    ULONG usb11ports = 0;

    ULONG cnt;

    KPRINTF(10, ("*** pciAllocUnit(%p) ***\n", hu));

    // allocate necessary memory
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ) {
        allocgood = uhciInit(hc,hu);
        if(allocgood) {
            for(cnt = usb11ports; cnt < usb11ports + hc->hc_NumPorts; cnt++) {
                hu->hu_PortMap11[cnt] = hc;
                hu->hu_PortNum11[cnt] = cnt - usb11ports;
                hc->hc_PortNumGlobal[cnt - usb11ports] = cnt;
                KPRINTF2(200,("Mapping ports\n"));
                KPRINTF2(200,(" Map11[%ld]= %p\n", cnt, hc));
                KPRINTF2(200,(" Num11[%ld]= %ld\n", cnt, (cnt-usb11ports)));
                KPRINTF2(200,(" Glo11[%ld]= %ld\n", (cnt-usb11ports), cnt));
            }
            usb11ports += hc->hc_NumPorts;
        }else{
            return FALSE;
        }
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    hu->hu_RootHubPorts = usb11ports;
    hu->hu_RootHubAddr = 0;

    // put em online
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ) {
    	hc->hc_Flags |= HCF_ONLINE;
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    KPRINTF(10, ("Unit allocated!\n"));

    return TRUE;
}

void pciFreeUnit(struct PCIUnit *hu) {
    struct PCIDevice *hd = hu->hu_Device;
    struct PCIController *hc;

    struct TagItem pciDeactivate[] = {
            { aHidd_PCIDevice_isIO,     FALSE },
            { aHidd_PCIDevice_isMEM,    FALSE },
            { aHidd_PCIDevice_isMaster, FALSE },
            { TAG_DONE, 0UL },
    };

    KPRINTF(10, ("*** pciFreeUnit(%p) ***\n", hu));

    // put em offline
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        hc->hc_Flags &= ~HCF_ONLINE;
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    uhciFree(hc, hu);

    //FIXME: uhciFree routine actually ONLY stops the chip NOT free anything as code below...
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ) {
        if(hc->hc_PCIMem) {
            HIDD_PCIDriver_FreePCIMem(hc->hc_PCIDriverObject, hc->hc_PCIMem);
            hc->hc_PCIMem = NULL;
        }
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    // disable and free board
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciDeactivate); // deactivate busmaster and IO/Mem
        if(hc->hc_PCIIntHandler.is_Node.ln_Name) {
            RemIntServer(INTB_KERNEL + hc->hc_PCIIntLine, &hc->hc_PCIIntHandler);
            hc->hc_PCIIntHandler.is_Node.ln_Name = NULL;
        }

        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }
}

void pciExpunge(struct PCIDevice *hd) {
    struct PCIController *hc;
    struct PCIUnit *hu;

    KPRINTF(10, ("*** pciExpunge(%p) ***\n", hd));

    hu = (struct PCIUnit *) hd->hd_Units.lh_Head;
    while(((struct Node *) hu)->ln_Succ) {
        Remove((struct Node *) hu);
        hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
        while(hc->hc_Node.ln_Succ) {
            Remove(&hc->hc_Node);
            FreePooled(hd->hd_MemPool, hc, sizeof(struct PCIController));
            hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
        }
        FreePooled(hd->hd_MemPool, hu, sizeof(struct PCIUnit));
        hu = (struct PCIUnit *) hd->hd_Units.lh_Head;
    }
    if(hd->hd_PCIHidd) {
        struct OOP_ABDescr attrbases[] =
        {
            { (STRPTR) IID_Hidd,            &hd->hd_HiddAB },
            { (STRPTR) IID_Hidd_PCIDevice,  &hd->hd_HiddPCIDeviceAB },
            { NULL, NULL }
        };

        OOP_ReleaseAttrBases(attrbases);

        OOP_DisposeObject(hd->hd_PCIHidd);
    }
}

APTR pciGetPhysical(struct PCIController *hc, APTR virtaddr) {
    //struct PCIDevice *hd = hc->hc_Device;
    return(HIDD_PCIDriver_CPUtoPCI(hc->hc_PCIDriverObject, virtaddr));
}

/*
 * Process some AROS-specific arguments.
 * 'usbpoweron' helps to bring up USB ports on IntelMac,
 * whose firmware sets them up incorrectly.
 */
static int getArguments(struct PCIDevice *base) {
    APTR BootLoaderBase = OpenResource("bootloader.resource");

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
