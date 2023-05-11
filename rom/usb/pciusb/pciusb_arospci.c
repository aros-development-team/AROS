/* pci_aros.c - pci access abstraction for AROS by Chris Hodges
*/

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/bootloader.h>

#include <aros/symbolsets.h>
#include <exec/types.h>
#include <oop/oop.h>
#include <devices/timer.h>
#include <aros/bootloader.h>

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "uhwcmd.h"
#include "ohciproto.h"
#include "uhciproto.h"
#include "ehciproto.h"
#include "xhciproto.h"

#define NewList NEWLIST

#ifdef base
#undef base
#endif
#define base hd
#if defined(AROS_USE_LOGRES)
#ifdef LogHandle
#undef LogHandle
#endif
#ifdef LogResBase
#undef LogResBase
#endif
#define LogHandle (hd->hd_LogRHandle)
#define LogResBase (base->hd_LogResBase)
#endif
static void handleQuirks(struct PCIController *hc)
{
    struct PCIDevice *hd = hc->hc_Device;
    IPTR vendorid, productid, revisionid, subvendorid, subproductid, memsize;

    hc->hc_Quirks = 0;
    if (hc->hc_HCIType == HCITYPE_EHCI)
        hc->hc_Quirks |= (HCQ_EHCI_OVERLAY_CTRL_FILL|HCQ_EHCI_OVERLAY_INT_FILL|HCQ_EHCI_OVERLAY_BULK_FILL);

    // Check for VirtualBox's EHCI (identify as precisely as possible to avoid
    // applying incorrect quirks to real Intel ICH6 EHCI)
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_VendorID, &vendorid);
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_ProductID, &productid);
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_RevisionID, &revisionid);
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_SubsystemVendorID, &subvendorid);
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_SubsystemID, &subproductid);
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Size0, &memsize);
    if (vendorid == 0x8086 && productid == 0x265c && revisionid == 0
        && subvendorid == 0 && subproductid == 0 && memsize == 4096)
    {
        // This is needed for EHCI to work in VirtualBox
        hc->hc_Quirks &= ~(HCQ_EHCI_OVERLAY_CTRL_FILL|HCQ_EHCI_OVERLAY_INT_FILL|HCQ_EHCI_OVERLAY_BULK_FILL);

        // VirtualBox reports frame list size of 1024, but still issues interrupts at
        // speed of around 4 per second instead of every 1024 ms
        hc->hc_Quirks |= HCQ_EHCI_VBOX_FRAMEROOLOVER;
    }
    else if (vendorid == 0x9710)
    {
        /* Apply MosChip frame-counter register bug workaround */
        hc->hc_Quirks |= HCQ_EHCI_MOSC_FRAMECOUNTBUG;
    }    
}

AROS_UFH3(void, pciEnumerator,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(OOP_Object *, pciDevice, A2),
          AROS_UFHA(APTR, message, A1))
{
    AROS_USERFUNC_INIT

    struct PCIDevice *hd = (struct PCIDevice *) hook->h_Data;
    struct PCIController *hc = NULL;
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

    pciusbDebug("PCI", "Found PCI device 0x%lx of type %ld, Intline=%ld\n", devid, hcitype, intline);

    if(intline == 255)
    {
        // we can't work without the correct interrupt line
        // BIOS needs plug & play os option disabled. Alternatively AROS must support ACPI reconfiguration
        pciusbDebug("PCI", "ERROR: PCI card has no interrupt line assigned by BIOS, disable Plug & Play OS!\n");
    }
    else
    {
        switch (hcitype)
        {
#if defined(PCIUSB_ENABLEXHCI)
        case HCITYPE_XHCI:
            if (!(hd->hd_Flags & HDF_ENABLEXHCI))
                break;
            // fall through
        case HCITYPE_OHCI:
#else
        case HCITYPE_OHCI:
#endif
        case HCITYPE_EHCI:
        case HCITYPE_UHCI:
            pciusbDebug("PCI", "Setting up device...\n");

            hc = AllocPooled(hd->hd_MemPool, sizeof(struct PCIController));
            if (hc)
            {
                hc->hc_Device = hd;
                hc->hc_DevID = devid;
                hc->hc_FunctionNum = sub;
                hc->hc_HCIType = hcitype;
                hc->hc_PCIDeviceObject = pciDevice;
                hc->hc_PCIIntLine = intline;

                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (IPTR *) &hc->hc_PCIDriverObject);

                NewList(&hc->hc_PeriodicTDQueue);

                NewList(&hc->hc_CtrlXFerQueue);
                NewList(&hc->hc_IntXFerQueue);
                NewList(&hc->hc_IsoXFerQueue);
                NewList(&hc->hc_BulkXFerQueue);
                NewList(&hc->hc_TDQueue);
                NewList(&hc->hc_AbortQueue);

                NewMinList(&hc->hc_RTIsoHandlers);

#if defined(USE_FAST_PCICFG)
# if !defined(__OOP_NOLIBBASE__) && !defined(__OOP_NOMETHODBASES__)
#  define __obj hc->hc_PCIDeviceObject
# endif
                hc->hc_ReadConfigByte = OOP_GetMethod(hc->hc_PCIDeviceObject, HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigByte, &hc->hc_ReadConfigByte_Class);
                hc->hc_ReadConfigWord = OOP_GetMethod(hc->hc_PCIDeviceObject, HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord, &hc->hc_ReadConfigWord_Class);
                hc->hc_ReadConfigLong = OOP_GetMethod(hc->hc_PCIDeviceObject, HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigLong, &hc->hc_ReadConfigLong_Class);
                hc->hc_WriteConfigByte = OOP_GetMethod(hc->hc_PCIDeviceObject, HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigByte, &hc->hc_WriteConfigByte_Class);
                hc->hc_WriteConfigWord = OOP_GetMethod(hc->hc_PCIDeviceObject, HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord, &hc->hc_WriteConfigWord_Class);
                hc->hc_WriteConfigLong = OOP_GetMethod(hc->hc_PCIDeviceObject, HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigLong, &hc->hc_WriteConfigLong_Class);
# if defined(PCIUSB_ENABLEXHCI)
                hc->hc_AllocPCIMem = OOP_GetMethod(hc->hc_PCIDriverObject, HiddPCIDriverBase + moHidd_PCIDriver_AllocPCIMem, &hc->hc_AllocPCIMem_Class);
                hc->hc_FreePCIMem = OOP_GetMethod(hc->hc_PCIDriverObject, HiddPCIDriverBase + moHidd_PCIDriver_FreePCIMem, &hc->hc_FreePCIMem_Class);
                hc->hc_MapPCI = OOP_GetMethod(hc->hc_PCIDriverObject, HiddPCIDriverBase + moHidd_PCIDriver_MapPCI, &hc->hc_MapPCI_Class);
                hc->hc_CPUtoPCI = OOP_GetMethod(hc->hc_PCIDriverObject, HiddPCIDriverBase + moHidd_PCIDriver_CPUtoPCI, &hc->hc_CPUtoPCI_Class);
                hc->hc_PCItoCPU = OOP_GetMethod(hc->hc_PCIDriverObject, HiddPCIDriverBase + moHidd_PCIDriver_PCItoCPU, &hc->hc_PCItoCPU_Class);
# endif
# if !defined(__OOP_NOLIBBASE__) && !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
                AddTail(&hd->hd_TempHCIList, &hc->hc_Node);

                handleQuirks(hc);
            }
            else
            {
                pciusbDebug("PCI", "Failed to allocate storage for controller entry!\n");
            }
            break;

        default:
            break;
        }
    }

    if (!hc)
        pciusbDebug("PCI", "Unsupported HCI type %ld\n", hcitype);

    AROS_USERFUNC_EXIT
}

/* /// "pciInit()" */
BOOL pciInit(struct PCIDevice *hd)
{
    OOP_Object                  *root;
    OOP_Class	       *usbContrClass;
    struct PCIController *hc;
    struct PCIController *nexthc;
    struct PCIUnit *hu;
    ULONG unitno = 0;

    pciusbDebug("PCI", "*** pciInit(%p) ***\n", hd);

    NewList(&hd->hd_TempHCIList);

    if((hd->hd_PCIHidd = OOP_NewObject(NULL, (STRPTR) CLID_Hidd_PCI, NULL)))
    {
        struct TagItem tags[] =
        {
            { tHidd_PCI_Class,      (PCI_CLASS_SERIAL_USB>>8) & 0xff },
            { tHidd_PCI_SubClass,   (PCI_CLASS_SERIAL_USB & 0xff) },
            { TAG_DONE, 0UL }
        };
        struct Hook findHook =
        {
             h_Entry:        (IPTR (*)()) pciEnumerator,
             h_Data:         hd,
        };
        pciusbDebug("PCI", "Searching for devices...\n");

        HIDD_PCI_EnumDevices(hd->hd_PCIHidd, &findHook, (struct TagItem *) &tags);
    } else {
        pciusbDebug("PCI", "Unable to create PCIHidd object!\n");
        return FALSE;
    }

    root = OOP_NewObject(NULL, CLID_Hidd_System, NULL);
    if (!root)
        root = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    pciusbDebug("PCI", "HW Root @  0x%p\n", root);
    usbContrClass = OOP_FindClass(CLID_Hidd_USBController);
    pciusbDebug("PCI", "USB Controller class @  0x%p\n", usbContrClass);

    // Create units with a list of host controllers having the same bus and device number.
    while(hd->hd_TempHCIList.lh_Head->ln_Succ)
    {
        BOOL    unitdone = FALSE, unithasv1 = FALSE, unithasv2 = FALSE;
#if defined(PCIUSB_ENABLEXHCI)
        BOOL    unithasv3 = FALSE;
#endif
        int     cnt;

        hu = AllocPooled(hd->hd_MemPool, sizeof(struct PCIUnit));
        if(!hu)
        {
            // actually, we should get rid of the allocated memory first, but I don't care as DeletePool() will take care of this eventually
            return FALSE;
        }
        hu->hu_Device = hd;
        hu->hu_UnitNo = unitno;

        NewList(&hu->hu_Controllers);
        NewList(&hu->hu_RHIOQueue);
        NewMinList(&hu->hu_FreeRTIsoNodes);
        for(cnt = 0; cnt < MAX_ROOT_PORTS; cnt++)
        {
            AddTail((struct List *) &hu->hu_FreeRTIsoNodes, (struct Node *)&hu->hu_RTIsoNodes[cnt].rtn_Node);
        }

        hu->hu_DevID = (ULONG)-1;
        ForeachNodeSafe(&hd->hd_TempHCIList, hc, nexthc)
        {
            if (hu->hu_DevID == (ULONG)-1)
                hu->hu_DevID = hc->hc_DevID;

            if (hc->hc_DevID == hu->hu_DevID)
            {
                BOOL ignore = FALSE;
                switch (hc->hc_HCIType)
                {
                    case HCITYPE_UHCI:
                    case HCITYPE_OHCI:
                        if (unithasv2)
                            unitdone = TRUE;
                        unithasv1 = TRUE;
                        break;
                    case HCITYPE_EHCI:
                        if (unithasv2)
                            unitdone = TRUE;
                        unithasv2 = TRUE;
                        break;
#if defined(PCIUSB_ENABLEXHCI)
                    case HCITYPE_XHCI:
                        if ((unithasv1) || (unithasv2))
                            unitdone = TRUE;
                        if ((!(hd->hd_Flags & HDF_ENABLEXHCI)) || unithasv3)
                            ignore = TRUE;
                        unithasv3 = TRUE;
                        break;
#endif
                }
                if (unitdone)
                    break;

                if (ignore)
                    continue;

                Remove(&hc->hc_Node);

                if ((usbContrClass) && (root))
                {
                    struct TagItem usbc_tags[] =
                    {
                        {aHidd_Name,                0       },
                        {aHidd_HardwareName,        0       },
                        {aHidd_Producer,            0       },
                #define USB_TAG_VEND 2
                        {aHidd_Product,             0       },
                #define USB_TAG_PROD 3
                        {aHidd_DriverData,          0       },
                #define USB_TAG_DATA 4
                        {TAG_DONE,                  0       }
                    };

                    hc->hc_Node.ln_Name = AllocVec(16, MEMF_CLEAR);
                    hc->hc_Node.ln_Pri = hc->hc_HCIType;
                    sprintf(hc->hc_Node.ln_Name, "pciusb.device/%u", (hu->hu_UnitNo & ~PCIUSBUNIT_MASK));
                    usbc_tags[0].ti_Data = (IPTR)hc->hc_Node.ln_Name;

                    usbc_tags[USB_TAG_VEND].ti_Data = 0;
                    usbc_tags[USB_TAG_PROD].ti_Data = hu->hu_DevID;

                    switch (hc->hc_HCIType)
                    {
                    case HCITYPE_UHCI:
                        {
                            usbc_tags[1].ti_Data = (IPTR)"PCI USB 1.x UHCI Host controller";
                            break;
                        }

                    case HCITYPE_OHCI:
                        {
                            usbc_tags[1].ti_Data = (IPTR)"PCI USB 1.1 OHCI Host controller";
                            break;
                        }

                    case HCITYPE_EHCI:
                        {
                            usbc_tags[1].ti_Data = (IPTR)"PCI USB 2.0 EHCI Host controller";
                            break;
                        }

                    case HCITYPE_XHCI:
                        {
                            usbc_tags[1].ti_Data = (IPTR)"PCI USB 3.x XHCI Host controller";
                            break;
                        }
                    }
                    HW_AddDriver(root, usbContrClass, usbc_tags);
                }
                hc->hc_Unit = hu;
                Enqueue(&hu->hu_Controllers, &hc->hc_Node);
            }
        }
        AddTail(&hd->hd_Units, (struct Node *) hu);
        unitno++;
    }
    return TRUE;
}
/* \\\ */

/* /// "pciStrcat()" */
STRPTR pciStrcat(STRPTR d, STRPTR s)
{
    while(*d) d++;
    while((*d++ = *s++));
    return --d;
}
/* \\\ */

/* /// "pciAllocUnit()" */
BOOL pciAllocUnit(struct PCIUnit *hu)
{
    struct PCIDevice *hd = hu->hu_Device;
    struct PCIController *hc;

    BOOL allocgood = TRUE;
    ULONG usb11ports = 0;
    ULONG usb20ports = 0;
#if defined(PCIUSB_ENABLEXHCI)
    ULONG xhciports = 0;
#endif
    ULONG cnt;

    ULONG ohcicnt = 0;
    ULONG uhcicnt = 0;
    ULONG ehcicnt = 0;
#if defined(PCIUSB_ENABLEXHCI)
    ULONG xhcicnt = 0;
#endif
    STRPTR prodname;

    pciusbDebug("PCI", "Unit @ %p\n", hu);

    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        CONST_STRPTR owner;
        
        owner = HIDD_PCIDevice_Obtain(hc->hc_PCIDeviceObject, hd->hd_Device.dd_Library.lib_Node.ln_Name);
        if (!owner)
            hc->hc_Flags |= HCF_ALLOCATED;
        else
        {
            pciusbWarn("PCI", "PCI Device already allocated <owner='%s'>\n", owner);
            allocgood = FALSE;
        }

        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    if(allocgood)
    {
        // allocate necessary memory
        hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
        while(hc->hc_Node.ln_Succ)
        {
            switch(hc->hc_HCIType)
            {
                case HCITYPE_UHCI:
                {
                    allocgood = uhciInit(hc,hu);
                    if(allocgood) {
                        uhcicnt++;
                    }
                    break;
                }

                case HCITYPE_OHCI:
                {
                    allocgood = ohciInit(hc,hu);
                    if(allocgood) {
                        ohcicnt++;
                    }
                    break;
                }

                case HCITYPE_EHCI:
                {
                    if(usb20ports) {
                        pciusbWarn("PCI", "More than one EHCI controller per board?!?\n");
                    }
                    allocgood = ehciInit(hc,hu);
                    if(allocgood) {
                        ehcicnt++;
                        usb20ports = hc->hc_NumPorts;
                    }
                    break;
                }
#if defined(PCIUSB_ENABLEXHCI)
                case HCITYPE_XHCI:
                {
                    if(xhciports) {
                        pciusbWarn("PCI", "More than one XHCI controller per board?!?\n");
                    }
                    allocgood = xhciInit(hc,hu);
                    if(allocgood) {
                        xhcicnt++;
                        xhciports = hc->hc_NumPorts;
                    }
                    break;
                }
#endif
            }
            hc = (struct PCIController *) hc->hc_Node.ln_Succ;
        }
    }

    if(!allocgood)
    {
        // free previously allocated boards
        hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
        while(hc->hc_Node.ln_Succ)
        {
            if (hc->hc_Flags & HCF_ALLOCATED)
            {
                hc->hc_Flags &= ~HCF_ALLOCATED;
                HIDD_PCIDevice_Release(hc->hc_PCIDeviceObject);
            }

            hc = (struct PCIController *) hc->hc_Node.ln_Succ;
        }
        return FALSE;
    }

    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        if((hc->hc_HCIType == HCITYPE_UHCI) || (hc->hc_HCIType == HCITYPE_OHCI))
        {
            if(hc->hc_complexrouting)
            {
                ULONG locport = 0;
                for(cnt = 0; cnt < usb20ports; cnt++)
                {
                    if(((hc->hc_portroute >> (cnt<<2)) & 0xf) == hc->hc_FunctionNum)
                    {
                        pciusbDebug("PCI", "CHC %ld Port %ld assigned to global Port %ld\n", hc->hc_FunctionNum, locport, cnt);
                        hu->hu_PortMap11[cnt] = hc;
                        hu->hu_PortNum11[cnt] = locport;
                        hc->hc_PortNum[locport] = cnt;
                        locport++;
                    }
                }
            } else {
                for(cnt = usb11ports; cnt < usb11ports + hc->hc_NumPorts; cnt++)
                {
                    hu->hu_PortMap11[cnt] = hc;
                    hu->hu_PortNum11[cnt] = cnt - usb11ports;
                    hc->hc_PortNum[cnt - usb11ports] = cnt;
                }
            }
            usb11ports += hc->hc_NumPorts;
        }
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }
    if((usb11ports != usb20ports) && usb20ports)
    {
        pciusbWarn("PCI", "EHCI Ports (%ld), do not match USB 1.1 Ports (%ld)!\n", usb20ports, usb11ports);
    }

    hu->hu_RootHub11Ports = usb11ports;
    hu->hu_RootHub20Ports = usb20ports;
    hu->hu_RootHubPorts = (usb11ports > usb20ports) ? usb11ports : usb20ports;
#if defined(PCIUSB_ENABLEXHCI)
    hu->hu_RootHubXPorts = xhciports;
    if (hu->hu_RootHubXPorts > hu->hu_RootHubPorts)
        hu->hu_RootHubPorts = hu->hu_RootHubXPorts;
#endif

    for(cnt = 0; cnt < hu->hu_RootHubPorts; cnt++)
    {
        hu->hu_PortOwner[cnt] = 
#if defined(PCIUSB_ENABLEXHCI)
                                                hu->hu_PortMapX[cnt] ? HCITYPE_XHCI : 
#endif
                                                hu->hu_PortMap20[cnt] ? HCITYPE_EHCI : HCITYPE_UHCI;
    }

    pciusbDebug("PCI", "Unit %ld: USB Board %08lx has %ld USB1.1 and %ld USB2.0 ports!\n", (hu->hu_UnitNo & ~PCIUSBUNIT_MASK), hu->hu_DevID, hu->hu_RootHub11Ports, hu->hu_RootHub20Ports);

    hu->hu_FrameCounter = 1;
    hu->hu_RootHubAddr = 0;

    // create product name of device
    BOOL havetype = FALSE;
    int usbmaj = 1, usbmin = 0;
    prodname = hu->hu_ProductName;
    *prodname = 0;
    pciStrcat(prodname, "PCI ");
    if(ohcicnt + uhcicnt) {
        havetype = TRUE;
        if(ohcicnt + uhcicnt >1)
        {
            prodname[4] = ohcicnt + uhcicnt + '0';
            prodname[5] = 'x';
            prodname[6] = 0;
        }
        pciStrcat(prodname, ohcicnt ? "OHCI" : "UHCI");
        usbmin = 1;
    }
    if(ehcicnt) {
        if (havetype)
            pciStrcat(prodname, "/");
        havetype = TRUE;
        usbmaj = 2;
        usbmin = 0;
        pciStrcat(prodname, "EHCI");
    }
#if defined(PCIUSB_ENABLEXHCI)
    if (xhcicnt) {
        if (havetype)
            pciStrcat(prodname, "/");
        usbmaj = 3;
        usbmin = 0;
        pciStrcat(prodname, "XHCI");
    }
#endif

    // put em online
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        hc->hc_Flags |= HCF_ONLINE;
#if defined(PCIUSB_ENABLEXHCI)
        if (hc->hc_HCIType == HCITYPE_XHCI)
        {
            UBYTE hcUSBVers = READCONFIGBYTE(hc, hc->hc_PCIDeviceObject, XHCI_SBRN);
            if (((hcUSBVers & 0xF0) >> 4) > usbmaj)
            {
                usbmaj = ((hcUSBVers & 0xF0) >> 4);
                usbmin = (hcUSBVers & 0xF);
            }
            else if ((((hcUSBVers & 0xF0) >> 4) == usbmaj) && ((hcUSBVers & 0xF) > usbmin))
            {
                usbmin = (hcUSBVers & 0xF);
            }
        }
#endif
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    // now add the USB version information to the product name.
    STRPTR prodversstr = pciStrcat(prodname, " USB ");
    prodversstr[0] = usbmaj + '0';
    prodversstr[1] = '.';
    prodversstr[2] = usbmin + '0';
    prodversstr[3] = 0;
    pciStrcat(prodname, " Host Controller");
    pciusbDebug("PCI", "Unit allocated!\n");

    return TRUE;
}
/* \\\ */

/* /// "pciFreeUnit()" */
void pciFreeUnit(struct PCIUnit *hu)
{
    struct PCIDevice *hd = hu->hu_Device;
    struct PCIController *hc;

    struct TagItem pciDeactivate[] =
    {
            { aHidd_PCIDevice_isIO,     FALSE },
            { aHidd_PCIDevice_isMEM,    FALSE },
            { aHidd_PCIDevice_isMaster, FALSE },
            { TAG_DONE, 0UL },
    };

    pciusbDebug("PCI", "Freeing unit @ 0x%p", hu);

    // put em offline
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        hc->hc_Flags &= ~HCF_ONLINE;
        switch (hc->hc_HCIType)
        {
#if defined(PCIUSB_ENABLEXHCI)
            case HCITYPE_XHCI:
                xhciFree(hc, hu);
                break;
#endif
            case HCITYPE_EHCI:
                ehciFree(hc, hu);
                break;
            case HCITYPE_OHCI:
                ohciFree(hc, hu);
                break;
            case HCITYPE_UHCI:
                uhciFree(hc, hu);
                break;
        }
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    //FIXME: (x/e/o/u)hciFree routines actually ONLY stops the chip NOT free anything as below...
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ) {
        if(hc->hc_PCIMem.me_Un.meu_Addr) {
            HIDD_PCIDriver_FreePCIMem(hc->hc_PCIDriverObject, hc->hc_PCIMem.me_Un.meu_Addr);
            hc->hc_PCIMem.me_Un.meu_Addr = NULL;
        }
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

    // disable and free board
    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciDeactivate); // deactivate busmaster and IO/Mem
        if(hc->hc_PCIIntHandler.is_Node.ln_Name)
        {
            HIDD_PCIDevice_RemoveInterrupt(hc->hc_PCIDeviceObject, &hc->hc_PCIIntHandler);
            hc->hc_PCIIntHandler.is_Node.ln_Name = NULL;
        }

        hc->hc_Flags &= ~HCF_ALLOCATED;
        HIDD_PCIDevice_Release(hc->hc_PCIDeviceObject);
        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }
}
/* \\\ */

/* /// "pciExpunge()" */
void pciExpunge(struct PCIDevice *hd)
{
    struct PCIController *hc;
    struct PCIUnit *hu;

    pciusbDebug("PCI", "Device @ 0x%p", hd);

    hu = (struct PCIUnit *) hd->hd_Units.lh_Head;
    while(((struct Node *) hu)->ln_Succ)
    {
        Remove((struct Node *) hu);
        hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
        while(hc->hc_Node.ln_Succ)
        {
            Remove(&hc->hc_Node);
            FreePooled(hd->hd_MemPool, hc, sizeof(struct PCIController));
            hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
        }
        FreePooled(hd->hd_MemPool, hu, sizeof(struct PCIUnit));
        hu = (struct PCIUnit *) hd->hd_Units.lh_Head;
    }
    if(hd->hd_PCIHidd)
    {
        OOP_DisposeObject(hd->hd_PCIHidd);
    }
}
/* \\\ */

#undef base
#define base (hc->hc_Device)
#undef LogHandle
#define LogHandle hc->hc_LogRHandle

BOOL PCIXAddInterrupt(struct PCIController *hc, struct Interrupt *interrupt)
{
    struct PCIDevice *hd = hc->hc_Device;

    return HIDD_PCIDevice_AddInterrupt(hc->hc_PCIDeviceObject, interrupt);
}

/* /// "pciAllocAligned()" */
APTR pciAllocAligned(struct PCIController *hc, struct MemEntry *alloc, ULONG Size, ULONG align, ULONG bounds)
{
    alloc->me_Length = align + Size;
    alloc->me_Un.meu_Addr = ALLOCPCIMEM(hc, hc->hc_PCIDriverObject, alloc->me_Length);
    if (alloc->me_Un.meu_Addr) {
        IPTR addrAligned = (((IPTR)alloc->me_Un.meu_Addr + (align-1)) & ~(align-1));
        if ((addrAligned & (bounds - 1)) != ((addrAligned + Size -1 ) & (bounds-1)))
        {
            pciusbDebug("PCI", "Re-allocating using bounds\n");
            FREEPCIMEM(hc, hc->hc_PCIDriverObject, alloc->me_Un.meu_Addr);
            alloc->me_Length = bounds + Size;
            alloc->me_Un.meu_Addr = ALLOCPCIMEM(hc, hc->hc_PCIDriverObject, alloc->me_Length);
            addrAligned = (((IPTR)alloc->me_Un.meu_Addr + (bounds-1)) & ~(bounds-1));
        }
        if (alloc->me_Un.meu_Addr) {
            pciusbDebug("PCI", "Allocated @ %p <0x%p, %u>\n", addrAligned, alloc->me_Un.meu_Addr, alloc->me_Length);
            return (APTR)addrAligned;
        }
    }
    pciusbDebug("PCI", "Unable to allocate suitably aligned memory\n");
    return NULL;
}
/* \\\ */

/* /// "pciGetPhysical()" */
APTR pciGetPhysical(struct PCIController *hc, APTR virtaddr)
{
    //struct PCIDevice *hd = hc->hc_Device;
    return(HIDD_PCIDriver_CPUtoPCI(hc->hc_PCIDriverObject, virtaddr));
}
/* \\\ */
