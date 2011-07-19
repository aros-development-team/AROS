/* pci_aros.c - pci access abstraction for AROS by Chris Hodges
*/

#include <aros/bootloader.h>
#include <aros/symbolsets.h>
#include <exec/types.h>
#include <oop/oop.h>
#include <devices/timer.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hidd/irq.h>
#include <proto/bootloader.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <string.h>

#include "uhwcmd.h"

#define NewList NEWLIST

#undef HiddPCIDeviceAttrBase
//#undef HiddUSBDeviceAttrBase
//#undef HiddUSBHubAttrBase
//#undef HiddUSBDrvAttrBase
#undef HiddAttrBase

#define HiddPCIDeviceAttrBase (hd->hd_HiddPCIDeviceAB)
//#define HiddUSBDeviceAttrBase (hd->hd_HiddUSBDeviceAB)
//#define HiddUSBHubAttrBase (hd->hd_HiddUSBHubAB)
//#define HiddUSBDrvAttrBase (hd->hd_HiddUSBDrvAB)
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
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Dev, &sub);
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

           	NewList(&hc->hc_CtrlXFerQueue);
           	NewList(&hc->hc_IntXFerQueue);
           	NewList(&hc->hc_IsoXFerQueue);
           	NewList(&hc->hc_BulkXFerQueue);

           	NewList(&hc->hc_TDQueue);
           	NewList(&hc->hc_AbortQueue);
           	NewList(&hc->hc_PeriodicTDQueue);

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

    if(!(hd->hd_IRQHidd = OOP_NewObject(NULL, (STRPTR) CLID_Hidd_IRQ, NULL))) {
        KPRINTF(20, ("Unable to create IRQHidd object!\n"));
        return FALSE;
    }

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
        OOP_DisposeObject(hd->hd_IRQHidd);
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

        NewList(&hu->hu_Controllers);
        NewList(&hu->hu_RHIOQueue);

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
/* \\\ */

/* /// "PCIXReadConfigByte()" */
UBYTE PCIXReadConfigByte(struct PCIController *hc, UBYTE offset)
{
    struct pHidd_PCIDevice_ReadConfigByte msg;

    msg.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
    msg.reg = offset;

    return OOP_DoMethod(hc->hc_PCIDeviceObject, (OOP_Msg) &msg);
}
/* \\\ */

/* /// "PCIXReadConfigWord()" */
UWORD PCIXReadConfigWord(struct PCIController *hc, UBYTE offset)
{
    struct pHidd_PCIDevice_ReadConfigWord msg;

    msg.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord);
    msg.reg = offset;

    return OOP_DoMethod(hc->hc_PCIDeviceObject, (OOP_Msg) &msg);
}
/* \\\ */

/* /// "PCIXReadConfigLong()" */
ULONG PCIXReadConfigLong(struct PCIController *hc, UBYTE offset)
{
    struct pHidd_PCIDevice_ReadConfigLong msg;

    msg.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigLong);
    msg.reg = offset;

    return OOP_DoMethod(hc->hc_PCIDeviceObject, (OOP_Msg) &msg);
}
/* \\\ */

/* /// "PCIXWriteConfigByte()" */
void PCIXWriteConfigByte(struct PCIController *hc, ULONG offset, UBYTE value)
{
    struct pHidd_PCIDevice_WriteConfigByte msg;

    msg.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
    msg.reg = offset;
    msg.val = value;

    OOP_DoMethod(hc->hc_PCIDeviceObject, (OOP_Msg) &msg);
}
/* \\\ */

/* /// "PCIXWriteConfigWord()" */
void PCIXWriteConfigWord(struct PCIController *hc, ULONG offset, UWORD value)
{
    struct pHidd_PCIDevice_WriteConfigWord msg;

    msg.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord);
    msg.reg = offset;
    msg.val = value;

    OOP_DoMethod(hc->hc_PCIDeviceObject, (OOP_Msg) &msg);
}
/* \\\ */

/* /// "PCIXWriteConfigLong()" */
void PCIXWriteConfigLong(struct PCIController *hc, ULONG offset, ULONG value)
{
    struct pHidd_PCIDevice_WriteConfigLong msg;

    msg.mID = OOP_GetMethodID(CLID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigLong);
    msg.reg = offset;
    msg.val = value;

    OOP_DoMethod(hc->hc_PCIDeviceObject, (OOP_Msg) &msg);
}
/* \\\ */

/* /// "pciStrcat()" */
void pciStrcat(STRPTR d, STRPTR s)
{
    while(*d) d++;
    while((*d++ = *s++));
}
/* \\\ */

/* /// "pciAllocUnit()" */
BOOL pciAllocUnit(struct PCIUnit *hu)
{
#if 0
    struct PCIDevice *hd = hu->hu_Device;
#endif
    struct PCIController *hc;

    BOOL allocgood = TRUE;
    ULONG usb11ports = 0;
//    ULONG usb20ports = 0;

    ULONG cnt;

    ULONG uhcicnt = 0;

    STRPTR prodname;

    KPRINTF(10, ("*** pciAllocUnit(%p) ***\n", hu));
        // allocate necessary memory
        hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
        while(hc->hc_Node.ln_Succ) {
            allocgood = uhciInit(hc,hu);
            if(allocgood) {
                uhcicnt++;
            }
            hc = (struct PCIController *) hc->hc_Node.ln_Succ;
        }

    if(!allocgood) {
        return FALSE;
    }

    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ) {
        for(cnt = usb11ports; cnt < usb11ports + hc->hc_NumPorts; cnt++) {
            hu->hu_PortMap11[cnt] = hc;
            hu->hu_PortNum11[cnt] = cnt - usb11ports;
            hc->hc_PortNumGlobal[cnt - usb11ports] = cnt;
        }
        usb11ports += hc->hc_NumPorts;
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    hu->hu_RootHub11Ports = usb11ports;
    hu->hu_RootHubPorts = usb11ports;

//    KPRINTF(10, ("Unit %ld: USB Board %08lx has %ld USB1.1 and %ld USB2.0 ports!\n", hu->hu_UnitNo, hu->hu_DevID, hu->hu_RootHub11Ports, hu->hu_RootHub20Ports));

    hu->hu_RootHubAddr = 0;

    // put em online
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
    	hc->hc_Flags |= HCF_ONLINE;
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    // create product name of device
    prodname = hu->hu_ProductName;
    *prodname = 0;
    pciStrcat(prodname, "PCI UHCI USB 1.1 Host Controller");

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

    //FIXME: (x/e/o/u)hciFree routines actually ONLY stops the chip NOT free anything as below...
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
        if(hc->hc_PCIIntHandler.h_Node.ln_Name) {
            HIDD_IRQ_RemHandler(hd->hd_IRQHidd, &hc->hc_PCIIntHandler);
            hc->hc_PCIIntHandler.h_Node.ln_Name = NULL;
        }

        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }
}
/* \\\ */

/* /// "pciExpunge()" */
void pciExpunge(struct PCIDevice *hd)
{
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
    if(hd->hd_IRQHidd) {
        OOP_DisposeObject(hd->hd_IRQHidd);
    }
}
/* \\\ */

/* /// "pciGetPhysical()" */
APTR pciGetPhysical(struct PCIController *hc, APTR virtaddr)
{
    //struct PCIDevice *hd = hc->hc_Device;
    return(HIDD_PCIDriver_CPUtoPCI(hc->hc_PCIDriverObject, virtaddr));
}
/* \\\ */

/*
 * Process some AROS-specific arguments.
 * 'usbpoweron' helps to bring up USB ports on IntelMac,
 * whose firmware sets them up incorrectly.
 */
static int getArguments(struct PCIDevice *base)
{
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
