/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved

    Desc: PCI access abstraction for AROS
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

static const char strPcixhciDevicePrefix[] = "pcixhci.device/";
static const char strPcixhciDeviceFormat[] = "pcixhci.device/%u";
static const char strProductPciPrefix[] = "PCI ";
static const char strProductXhci[] = "xHCI ";
static const char strProductUsb[] = " USB ";
static const char strProductHostController[] = " Host Controller";
static const char strEmpty[] = "";

extern int XHCIControllerOOPStartup(struct PCIDevice *hd);

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
    IPTR vendorid;
    IPTR productid;
    ULONG devid;

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Interface, &hcitype);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Bus, &bus);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Dev, &dev);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Sub, &sub);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &intline);

    devid = (bus << 16) | dev;

    pciusbDebug("PCI", "Found PCI device 0x%lx of type %ld, Intline=%ld\n", devid, hcitype, intline);

    if(intline == 255) {
        // we can't work without the correct interrupt line
        // BIOS needs plug & play os option disabled. Alternatively AROS must support ACPI reconfiguration
        pciusbDebug("PCI", "ERROR: PCI card has no interrupt line assigned by BIOS, disable Plug & Play OS!\n");
    } else {
        switch(hcitype) {
        case HCITYPE_XHCI:
            pciusbDebug("PCI", "Setting up device...\n");

            hc = AllocPooled(hd->hd_MemPool, sizeof(struct PCIController));
            if(hc) {
                hc->hc_IsoPTDCount = PCIUSB_ISO_PTD_COUNT;
                hc->hc_Device = hd;
                hc->hc_DevID = devid;
                hc->hc_FunctionNum = sub;
                hc->hc_PCIDeviceObject = pciDevice;
                hc->hc_PCIIntLine = intline;
                hc->hc_PCIMemIsExec = FALSE;
                hc->hc_Quirks = 0;
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &vendorid);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &productid);
                hc->hc_VendID = (UWORD)vendorid & 0xFFFF;
                hc->hc_ProdID = (UWORD)productid & 0xFFFF;

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
                hc->hc_ReadConfigByte = OOP_GetMethod(hc->hc_PCIDeviceObject, HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigByte,
                                                      &hc->hc_ReadConfigByte_Class);
                hc->hc_ReadConfigWord = OOP_GetMethod(hc->hc_PCIDeviceObject, HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord,
                                                      &hc->hc_ReadConfigWord_Class);
                hc->hc_ReadConfigLong = OOP_GetMethod(hc->hc_PCIDeviceObject, HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigLong,
                                                      &hc->hc_ReadConfigLong_Class);
                hc->hc_WriteConfigByte = OOP_GetMethod(hc->hc_PCIDeviceObject, HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigByte,
                                                       &hc->hc_WriteConfigByte_Class);
                hc->hc_WriteConfigWord = OOP_GetMethod(hc->hc_PCIDeviceObject, HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord,
                                                       &hc->hc_WriteConfigWord_Class);
                hc->hc_WriteConfigLong = OOP_GetMethod(hc->hc_PCIDeviceObject, HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigLong,
                                                       &hc->hc_WriteConfigLong_Class);
                hc->hc_AllocPCIMem = OOP_GetMethod(hc->hc_PCIDriverObject, HiddPCIDriverBase + moHidd_PCIDriver_AllocPCIMem,
                                                   &hc->hc_AllocPCIMem_Class);
                hc->hc_FreePCIMem = OOP_GetMethod(hc->hc_PCIDriverObject, HiddPCIDriverBase + moHidd_PCIDriver_FreePCIMem,
                                                  &hc->hc_FreePCIMem_Class);
                hc->hc_MapPCI = OOP_GetMethod(hc->hc_PCIDriverObject, HiddPCIDriverBase + moHidd_PCIDriver_MapPCI,
                                              &hc->hc_MapPCI_Class);
                hc->hc_CPUtoPCI = OOP_GetMethod(hc->hc_PCIDriverObject, HiddPCIDriverBase + moHidd_PCIDriver_CPUtoPCI,
                                                &hc->hc_CPUtoPCI_Class);
                hc->hc_PCItoCPU = OOP_GetMethod(hc->hc_PCIDriverObject, HiddPCIDriverBase + moHidd_PCIDriver_PCItoCPU,
                                                &hc->hc_PCItoCPU_Class);
# if !defined(__OOP_NOLIBBASE__) && !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
                AddTail(&hd->hd_TempHCIList, &hc->hc_Node);
            } else {
                pciusbDebug("PCI", "Failed to allocate storage for controller entry!\n");
            }
            break;

        default:
            break;
        }
    }

    if(!hc)
        pciusbDebug("PCI", "Unsupported HCI type %ld\n", hcitype);

    AROS_USERFUNC_EXIT
}

/* /// "pciInit()" */
BOOL pciInit(struct PCIDevice *hd)
{
    OOP_Object                  *root;
    struct PCIController *hc;
    struct PCIController *nexthc;
    struct PCIUnit *hu;
    ULONG unitno = 0;

    pciusbDebug("PCI", "*** pciInit(%p) ***\n", hd);

    NewList(&hd->hd_TempHCIList);

    if((hd->hd_PCIHidd = OOP_NewObject(NULL, (STRPTR) CLID_Hidd_PCI, NULL))) {
        struct TagItem tags[] = {
            { tHidd_PCI_Class,    (PCI_CLASS_SERIAL_USB >> 8) & 0xff },
            { tHidd_PCI_SubClass, (PCI_CLASS_SERIAL_USB & 0xff) },
            { TAG_DONE, 0UL }
        };
        struct Hook findHook = {
            .h_Entry   = (IPTR(*)()) pciEnumerator,
            .h_Data    = hd,
        };
        pciusbDebug("PCI", "Searching for devices...\n");

        HIDD_PCI_EnumDevices(hd->hd_PCIHidd, &findHook, (struct TagItem *) &tags);
    } else {
        pciusbDebug("PCI", "Unable to create PCIHidd object!\n");
        return FALSE;
    }

    root = OOP_NewObject(NULL, CLID_Hidd_System, NULL);
    if(!root)
        root = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    pciusbDebug("PCI", "HW Root @  0x%p\n", root);
    XHCIControllerOOPStartup(hd);
    pciusbDebug("PCI", "xHCI USB Controller class @  0x%p\n", hd->hd_USBXHCIControllerClass);

    // Create units with a list of host controllers having the same bus and device number.
    while(hd->hd_TempHCIList.lh_Head->ln_Succ) {
        int     cnt;

        hu = AllocPooled(hd->hd_MemPool, sizeof(struct PCIUnit));
        if(!hu) {
            // actually, we should get rid of the allocated memory first, but I don't care as DeletePool() will take care of this eventually
            return FALSE;
        }
        hu->hu_Device = hd;
        hu->hu_UnitNo = unitno;

        NewList(&hu->hu_Controllers);
        NewList(&hu->hu_RHIOQueue);
        NewMinList(&hu->hu_FreeRTIsoNodes);
        for(cnt = 0; cnt < MAX_ROOT_PORTS; cnt++) {
            hu->hu_RTIsoNodes[cnt].rtn_PTDs = NULL;
            hu->hu_RTIsoNodes[cnt].rtn_PTDCount = 0;
            AddTail((struct List *) &hu->hu_FreeRTIsoNodes, (struct Node *)&hu->hu_RTIsoNodes[cnt].rtn_Node);
        }

        hu->hu_DevID = (ULONG)-1;
        ForeachNodeSafe(&hd->hd_TempHCIList, hc, nexthc) {
            if(hu->hu_DevID == (ULONG)-1)
                hu->hu_DevID = hc->hc_DevID;

            if(hc->hc_DevID == hu->hu_DevID) {
                Remove(&hc->hc_Node);

                if((hd->hd_USBXHCIControllerClass) && (root)) {
                    struct TagItem usbc_tags[] = {
                        {aHidd_Name,                0               },
                        {aHidd_Producer,            hc->hc_VendID   },
                        {aHidd_Product,             hc->hc_ProdID   },
                        {aHidd_DriverData, (IPTR)hc        },
                        {TAG_DONE,                  0               }
                    };
                    int name_len = sizeof(strPcixhciDevicePrefix) - 1 + 10 + 1;
                    hc->hc_Node.ln_Name = AllocVec(name_len, MEMF_CLEAR);
                    hc->hc_Node.ln_Pri = HCITYPE_XHCI;
                    sprintf(hc->hc_Node.ln_Name, strPcixhciDeviceFormat,
                            (hu->hu_UnitNo & ~PCIUSBUNIT_MASK));
                    usbc_tags[0].ti_Data = (IPTR)hc->hc_Node.ln_Name;
                    pciusbWarn("PCI", "Instantiating xHCI controller class for '%s'\n", hc->hc_Node.ln_Name);
                    HW_AddDriver(root, hd->hd_USBXHCIControllerClass, usbc_tags);
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

static void pciusbUpdateVersion(UBYTE major, UBYTE minor, UBYTE *bestMajor, UBYTE *bestMinor)
{
    if((major > *bestMajor) || ((major == *bestMajor) && (minor > *bestMinor))) {
        *bestMajor = major;
        *bestMinor = minor;
    }
}

static void pciusbAppendVersion(STRPTR dest, UBYTE major, UBYTE minor)
{
    STRPTR vers = pciStrcat(dest, (char *)strEmpty);
    vers[0] = (char)('0' + (major % 10));
    vers[1] = '.';
    vers[2] = (char)('0' + (minor % 10));
    vers[3] = 0;
}

/* /// "pciAllocUnit()" */
BOOL pciAllocUnit(struct PCIUnit *hu)
{
    struct PCIDevice *hd = hu->hu_Device;
    struct PCIController *hc;

    BOOL allocgood = TRUE;
    ULONG xhciports = 0;
    ULONG cnt;

    ULONG xhcicnt = 0;
    UBYTE hciMajor = 0;
    UBYTE hciMinor = 0;
    UBYTE usbMajor = 0;
    UBYTE usbMinor = 0;
    STRPTR prodname;

    pciusbDebug("PCI", "Unit @ %p\n", hu);

    ForeachNode(&hu->hu_Controllers, hc) {
        CONST_STRPTR owner;

        owner = HIDD_PCIDevice_Obtain(hc->hc_PCIDeviceObject, hd->hd_Device.dd_Library.lib_Node.ln_Name);
        if(!owner)
            hc->hc_Flags |= HCF_ALLOCATED;
        else {
            pciusbWarn("PCI", "PCI Device already allocated <owner='%s'>\n", owner);
            allocgood = FALSE;
        }
    }

    if(allocgood) {
        // allocate necessary memory
        ForeachNode(&hu->hu_Controllers, hc) {
            if(xhciports) {
                pciusbWarn("PCI", "More than one xHCI controller per board?!?\n");
            }
            allocgood = xhciInit(hc, hu, hu->hu_TimerReq);
            if(allocgood) {
                xhcicnt++;
                xhciports += hc->hc_NumPorts;
                pciusbUpdateVersion(hc->hc_HCIVersionMajor, hc->hc_HCIVersionMinor,
                                    &hciMajor, &hciMinor);
                pciusbUpdateVersion(hc->hc_USBVersionMajor, hc->hc_USBVersionMinor,
                                    &usbMajor, &usbMinor);
            }
        }
    }

    if(!allocgood) {
        // free previously allocated boards
        ForeachNode(&hu->hu_Controllers, hc) {
            if(hc->hc_Flags & HCF_ALLOCATED) {
                hc->hc_Flags &= ~HCF_ALLOCATED;
                HIDD_PCIDevice_Release(hc->hc_PCIDeviceObject);
            }
        }
        return FALSE;
    }

    ULONG portbase = 0;
    ForeachNode(&hu->hu_Controllers, hc) {
        for(cnt = portbase; cnt < portbase + hc->hc_NumPorts; cnt++) {
            hu->hu_PortMapX[cnt] = hc;
            hc->hc_PortNum[cnt - portbase] = cnt;
        }
        portbase += hc->hc_NumPorts;
    }

    hu->hu_RootHubXPorts = xhciports;
    hu->hu_RootHubPorts = xhciports;

    pciusbDebug("PCI", "Unit %ld: USB Board %08lx:\n", (hu->hu_UnitNo & ~PCIUSBUNIT_MASK), hu->hu_DevID);
    if(hu->hu_RootHubXPorts) {
        pciusbDebug("PCI", "Unit %ld: - %ld USB%u.%u+ port(s)\n", (hu->hu_UnitNo & ~PCIUSBUNIT_MASK), usbMajor, usbMinor,
                    hu->hu_RootHubXPorts);
    }

    hu->hu_FrameCounter = 1;
    hu->hu_RootHubAddr = 0;

    // create product name of device
    prodname = hu->hu_ProductName;
    *prodname = 0;
    pciStrcat(prodname, (char *)strProductPciPrefix);
    pciStrcat(prodname, (char *)strProductXhci);
    pciusbAppendVersion(prodname, hciMajor, hciMinor);

    // put em online
    ForeachNode(&hu->hu_Controllers, hc) {
        hc->hc_Flags |= HCF_ONLINE;
    }

    // now add the USB version information to the product name.
    pciStrcat(prodname, (char *)strProductUsb);
    pciusbAppendVersion(prodname, usbMajor, usbMinor);
    pciStrcat(prodname, (char *)strProductHostController);
    pciusbDebug("PCI", "Unit allocated!\n");

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

    pciusbDebug("PCI", "Freeing unit @ 0x%p", hu);

    // put em offline
    ForeachNode(&hu->hu_Controllers, hc) {
        hc->hc_Flags &= ~HCF_ONLINE;
        xhciFree(hc, hu);
    }

    //FIXME: (x/e/o/u)hciFree routines actually ONLY stops the chip NOT free anything as below...
    ForeachNode(&hu->hu_Controllers, hc) {
        if(hc->hc_PCIMem.me_Un.meu_Addr) {
            if(hc->hc_PCIMemIsExec) {
                FreeMem(hc->hc_PCIMem.me_Un.meu_Addr, hc->hc_PCIMem.me_Length);
                hc->hc_PCIMemIsExec = FALSE;
            } else {
                HIDD_PCIDriver_FreePCIMem(hc->hc_PCIDriverObject, hc->hc_PCIMem.me_Un.meu_Addr);
            }
            hc->hc_PCIMem.me_Un.meu_Addr = NULL;
        }
    }

    // disable and free board
    ForeachNode(&hu->hu_Controllers, hc) {
        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciDeactivate); // deactivate busmaster and IO/Mem
        if(hc->hc_PCIIntHandler.is_Node.ln_Name) {
#if defined(HCF_MSI)
            if(hc->hc_Flags & HCF_MSI) {
                RemIntServer(INTB_KERNEL + hc->hc_PCIIntLine, &hc->hc_PCIIntHandler);
                HIDD_PCIDevice_ReleaseVectors(hc->hc_PCIDeviceObject);
                hc->hc_Flags &= ~HCF_MSI;
            } else {
#endif
                HIDD_PCIDevice_RemoveInterrupt(hc->hc_PCIDeviceObject, &hc->hc_PCIIntHandler);
#if defined(HCF_MSI)
            }
#endif
            hc->hc_PCIIntHandler.is_Node.ln_Name = NULL;
        }

        hc->hc_Flags &= ~HCF_ALLOCATED;
        HIDD_PCIDevice_Release(hc->hc_PCIDeviceObject);
    }
}
/* \\\ */

/* /// "pciExpunge()" */
void pciExpunge(struct PCIDevice *hd)
{
    struct PCIController *hc, *ct;
    struct PCIUnit *hu, *ut;

    pciusbDebug("PCI", "Device @ 0x%p", hd);

    ForeachNodeSafe(&hd->hd_Units, hu, ut) {
        Remove((struct Node *) hu);
        ForeachNodeSafe(&hu->hu_Controllers, hc, ct) {
            Remove(&hc->hc_Node);
            FreePooled(hd->hd_MemPool, hc, sizeof(struct PCIController));
        }
        FreePooled(hd->hd_MemPool, hu, sizeof(struct PCIUnit));
    }
    if(hd->hd_PCIHidd) {
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
    if(alloc->me_Un.meu_Addr) {
        IPTR addrAligned = (((IPTR)alloc->me_Un.meu_Addr + (align - 1)) & ~(align - 1));
        if((addrAligned & (bounds - 1)) != ((addrAligned + Size - 1) & (bounds - 1))) {
            pciusbDebug("PCI", "Re-allocating using bounds\n");
            FREEPCIMEM(hc, hc->hc_PCIDriverObject, alloc->me_Un.meu_Addr);
            alloc->me_Length = bounds + Size;
            alloc->me_Un.meu_Addr = ALLOCPCIMEM(hc, hc->hc_PCIDriverObject, alloc->me_Length);
            addrAligned = (((IPTR)alloc->me_Un.meu_Addr + (bounds - 1)) & ~(bounds - 1));
        }
        if(alloc->me_Un.meu_Addr) {
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
