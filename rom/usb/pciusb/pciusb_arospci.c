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
#include "ohci/ohciproto.h"
#include "uhci/uhciproto.h"
#include "ehci/ehciproto.h"

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

    hc->hc_VendID = (UWORD)vendorid & 0xFFFF;
    hc->hc_ProdID = (UWORD)productid & 0xFFFF;

    if (vendorid == 0x8086 && productid == 0x265c && revisionid == 0
            && subvendorid == 0 && subproductid == 0 && memsize == 4096) {
        // This is needed for EHCI to work in VirtualBox
        hc->hc_Quirks &= ~(HCQ_EHCI_OVERLAY_CTRL_FILL|HCQ_EHCI_OVERLAY_INT_FILL|HCQ_EHCI_OVERLAY_BULK_FILL);

        // VirtualBox reports frame list size of 1024, but still issues interrupts at
        // speed of around 4 per second instead of every 1024 ms
        hc->hc_Quirks |= HCQ_EHCI_VBOX_FRAMEROOLOVER;
    } else if (vendorid == 0x9710) {
        /* Apply MosChip frame-counter register bug workaround */
        hc->hc_Quirks |= HCQ_EHCI_MOSC_FRAMECOUNTBUG;
    } else if (vendorid == 0x1106) {
        /* Apply fix for VIA Babble problem */
        switch(productid) {
        case 0x3038:     // VT83C572 UHCI
        case 0x3104:     // VT82C596 UHCI
        case 0x3107:     // VT82C686 UHCI
            if (revisionid < 0x10) {  // apply only to affected revisions
                hc->hc_Quirks |= HCQ_UHCI_VIA_BABBLE;
            }
            break;
        }
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

    if(intline == 255) {
        // we can't work without the correct interrupt line
        // BIOS needs plug & play os option disabled. Alternatively AROS must support ACPI reconfiguration
        pciusbDebug("PCI", "ERROR: PCI card has no interrupt line assigned by BIOS, disable Plug & Play OS!\n");
    } else {
        switch (hcitype) {
        case HCITYPE_OHCI:
        case HCITYPE_EHCI:
        case HCITYPE_UHCI:
            pciusbDebug("PCI", "Setting up device...\n");

            hc = AllocPooled(hd->hd_MemPool, sizeof(struct PCIController));
            if (hc) {
                hc->hc_IsoPTDCount = PCIUSB_ISO_PTD_COUNT;
                hc->hc_Device = hd;
                hc->hc_DevID = devid;
                hc->hc_FunctionNum = sub;
                hc->hc_HCIType = hcitype;
                hc->hc_PCIDeviceObject = pciDevice;
                hc->hc_PCIIntLine = intline;
                hc->hc_PCIMemIsExec = FALSE;

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
# if !defined(__OOP_NOLIBBASE__) && !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
                AddTail(&hd->hd_TempHCIList, &hc->hc_Node);

                handleQuirks(hc);
            } else {
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

    if((hd->hd_PCIHidd = OOP_NewObject(NULL, (STRPTR) CLID_Hidd_PCI, NULL))) {
        struct TagItem tags[] = {
            { tHidd_PCI_Class,      (PCI_CLASS_SERIAL_USB>>8) & 0xff },
            { tHidd_PCI_SubClass,   (PCI_CLASS_SERIAL_USB & 0xff) },
            { TAG_DONE, 0UL }
        };
        struct Hook findHook = {
            .h_Entry   = (IPTR (*)()) pciEnumerator,
            .h_Data    = hd,
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
    //
    // NOTE:
    //   EHCI companions (UHCI/OHCI) may be enumerated in any order by the PCI HIDD.
    //   Do not stop unit construction early just because an EHCI controller has been seen;
    //   we must aggregate *all* USB HCIs for the same (bus,dev) into the same unit.
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
            if (hu->hu_DevID == (ULONG)-1)
                hu->hu_DevID = hc->hc_DevID;

            if (hc->hc_DevID == hu->hu_DevID) {

                Remove(&hc->hc_Node);

                if ((usbContrClass) && (root)) {
                    struct TagItem usbc_tags[] = {
                        {aHidd_Name,                0               },
                        {aHidd_HardwareName,        0               },
                        {aHidd_Producer,            hc->hc_VendID   },
                        {aHidd_Product,             hc->hc_ProdID   },
                        {aHidd_DriverData,          0               },
                        {TAG_DONE,                  0               }
                    };
                    char *usb_chipset = "UHCI";
                    int usb_min = -1, usb_maj = 1;

                    hc->hc_Node.ln_Name = AllocVec(16 + 34, MEMF_CLEAR);
                    hc->hc_Node.ln_Pri = hc->hc_HCIType;
                    sprintf(hc->hc_Node.ln_Name, "pciusb.device/%u", (hu->hu_UnitNo & ~PCIUSBUNIT_MASK));
                    usbc_tags[0].ti_Data = (IPTR)hc->hc_Node.ln_Name;

                    usbc_tags[1].ti_Data = (IPTR)&hc->hc_Node.ln_Name[16];
                    switch (hc->hc_HCIType) {
                    case HCITYPE_OHCI: {
                        usb_chipset = "OHCI";
                        usb_min = 1;
                        break;
                    }

                    case HCITYPE_EHCI: {
                        usb_chipset = "EHCI";
                        usb_maj = 2;
                        usb_min = 0;
                        break;
                    }

                    }

                    sprintf((char *)usbc_tags[1].ti_Data, "PCI USB %u.",  usb_maj);
                    if (usb_min < 0)
                        sprintf((char *)(usbc_tags[1].ti_Data + 10), "x");
                    else
                        sprintf((char *)(usbc_tags[1].ti_Data + 10), "%u", usb_min);
                    sprintf((char *)(usbc_tags[1].ti_Data + 11), " %s Host controller",  usb_chipset);

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
    struct PCIController *ehci_list[16];
    struct PCIController *comp_list[16];
    struct PCIController *comp_by_func[16];
    UBYTE comp_used_ports[16];
    ULONG ehci_count = 0;
    ULONG comp_count = 0;

    BOOL allocgood = TRUE;
    ULONG usb11ports = 0;
    ULONG usb20ports = 0;
    ULONG cnt;
    ULONG i;

    ULONG ohcicnt = 0;
    ULONG uhcicnt = 0;
    ULONG ehcicnt = 0;
    STRPTR prodname;

    pciusbDebug("PCI", "Unit @ %p\n", hu);

    ForeachNode(&hu->hu_Controllers, hc) {
        CONST_STRPTR owner;

        owner = HIDD_PCIDevice_Obtain(hc->hc_PCIDeviceObject, hd->hd_Device.dd_Library.lib_Node.ln_Name);
        if (!owner)
            hc->hc_Flags |= HCF_ALLOCATED;
        else {
            pciusbWarn("PCI", "PCI Device already allocated <owner='%s'>\n", owner);
            allocgood = FALSE;
        }
    }

    if(allocgood) {
        // allocate necessary memory
        ForeachNode(&hu->hu_Controllers, hc) {
            switch(hc->hc_HCIType) {
            case HCITYPE_UHCI: {
                allocgood = uhciInit(hc,hu);
                if(allocgood) {
                    uhcicnt++;
                }
                break;
            }

            case HCITYPE_OHCI: {
                allocgood = ohciInit(hc,hu);
                if(allocgood) {
                    ohcicnt++;
                }
                break;
            }

            case HCITYPE_EHCI: {
                allocgood = ehciInit(hc,hu);
                if(allocgood) {
                    ehcicnt++;
                }
                break;
            }
            }
        }
    }

    if(!allocgood) {
        // free previously allocated boards
        ForeachNode(&hu->hu_Controllers, hc) {
            if (hc->hc_Flags & HCF_ALLOCATED) {
                hc->hc_Flags &= ~HCF_ALLOCATED;
                HIDD_PCIDevice_Release(hc->hc_PCIDeviceObject);
            }
        }
        return FALSE;
    }

    /*
     * Build global port mapping.
     */

    /* Clear per-unit root port maps. */
    for (cnt = 0; cnt < MAX_ROOT_PORTS; cnt++) {
        hu->hu_PortMap11[cnt] = NULL;
        hu->hu_PortMap20[cnt] = NULL;
        hu->hu_PortNum11[cnt] = 0xFF;
        hu->hu_PortNum20[cnt] = 0xFF;
        hu->hu_PortOwner[cnt] = HCITYPE_UHCI;
    }
    hu->hu_RootPortChanges = 0;

    for (i = 0; i < 16; i++) {
        comp_by_func[i] = NULL;
        comp_used_ports[i] = 0;
    }

    /* Collect controllers and reset per-controller coverage fields. */
    ForeachNode(&hu->hu_Controllers, hc) {
        hc->hc_GlobalPortMask = 0;
        hc->hc_GlobalPortFirst = 0xFFFF;
        hc->hc_GlobalPortLast = 0;
        if (hc->hc_HCIType == HCITYPE_EHCI) {
            if (ehci_count < 16)
                ehci_list[ehci_count++] = hc;
        } else if ((hc->hc_HCIType == HCITYPE_UHCI) || (hc->hc_HCIType == HCITYPE_OHCI)) {
            if (comp_count < 16)
                comp_list[comp_count++] = hc;
            comp_by_func[hc->hc_FunctionNum & 0xF] = hc;
            usb11ports += hc->hc_NumPorts;
        }
    }

    /* Deterministic ordering: sort by PCI function number. */
    for (i = 0; i + 1 < ehci_count; i++) {
        ULONG j;
        for (j = i + 1; j < ehci_count; j++) {
            if (ehci_list[j]->hc_FunctionNum < ehci_list[i]->hc_FunctionNum) {
                struct PCIController *tmp = ehci_list[i];
                ehci_list[i] = ehci_list[j];
                ehci_list[j] = tmp;
            }
        }
    }
    for (i = 0; i + 1 < comp_count; i++) {
        ULONG j;
        for (j = i + 1; j < comp_count; j++) {
            if (comp_list[j]->hc_FunctionNum < comp_list[i]->hc_FunctionNum) {
                struct PCIController *tmp = comp_list[i];
                comp_list[i] = comp_list[j];
                comp_list[j] = tmp;
            }
        }
    }

    /* USB 2.0 (EHCI) global port layout. */
    usb20ports = 0;
    for (i = 0; i < ehci_count; i++) {
        UWORD lp;
        struct PCIController *ehc = ehci_list[i];
        for (lp = 0; lp < ehc->hc_NumPorts && usb20ports < MAX_ROOT_PORTS; lp++) {
            ULONG gport = usb20ports++;
            hu->hu_PortMap20[gport] = ehc;
            hu->hu_PortNum20[gport] = (UBYTE)lp;
            ehc->hc_PortNum[lp] = (UBYTE)gport;

            ehc->hc_GlobalPortMask |= (1UL << gport);
            if (ehc->hc_GlobalPortFirst == 0xFFFF)
                ehc->hc_GlobalPortFirst = (UWORD)gport;
            ehc->hc_GlobalPortLast = (UWORD)gport;
        }
        if (ehc->hc_NumPorts && usb20ports >= MAX_ROOT_PORTS) {
            pciusbWarn("PCI", "EHCI function %ld: root-hub ports exceed MAX_ROOT_PORTS (%d); extra ports ignored\n",
                       ehc->hc_FunctionNum, MAX_ROOT_PORTS);
        }
    }

    /* Cap total USB1.1 port count to our table size. */
    if (usb11ports > MAX_ROOT_PORTS)
        usb11ports = MAX_ROOT_PORTS;

    /* USB 1.1 mapping: first derive from EHCI PORTROUTE where available. */
    for (cnt = 0; cnt < usb20ports; cnt++) {
        struct PCIController *ehc = hu->hu_PortMap20[cnt];
        struct EhciHCPrivate *ehp = ehc ? (struct EhciHCPrivate *)ehc->hc_CPrivate : NULL;
        UWORD lp;
        UWORD func;
        UBYTE clp;
        struct PCIController *chc;

        if (!ehc || !ehp || !ehp->ehc_ComplexRouting)
            continue;

        lp = hu->hu_PortNum20[cnt];
        func = (UWORD)((ehc->hc_portroute >> (lp << 2)) & 0xF);
        chc = comp_by_func[func];
        if (!chc)
            continue;

        clp = comp_used_ports[func];
        if (clp >= chc->hc_NumPorts) {
            pciusbWarn("PCI", "Companion function %ld: PORTROUTE wants port %ld but controller has only %ld port(s)\n",
                       (ULONG)func, (ULONG)clp, (ULONG)chc->hc_NumPorts);
            continue;
        }

        hu->hu_PortMap11[cnt] = chc;
        hu->hu_PortNum11[cnt] = clp;
        chc->hc_PortNum[clp] = (UBYTE)cnt;
        comp_used_ports[func] = clp + 1;

        chc->hc_GlobalPortMask |= (1UL << cnt);
        if (chc->hc_GlobalPortFirst == 0xFFFF)
            chc->hc_GlobalPortFirst = (UWORD)cnt;
        chc->hc_GlobalPortLast = (UWORD)cnt;
    }

    /*
     * Non-extended routing case (no PORTROUTE): use per-EHCI HCSPARAMS companion
     * information when available. This preserves correct port range assignment
     * for common integrated multi-function devices.
     */
    {
        ULONG comp_base = 0;

        for (i = 0; i < ehci_count; i++) {
            struct PCIController *ehc = ehci_list[i];
            struct EhciHCPrivate *ehp = ehc ? (struct EhciHCPrivate *)ehc->hc_CPrivate : NULL;
            UWORD ppc;
            UWORD ncc;
            UWORD lp;

            if (!ehc || !ehp || ehp->ehc_ComplexRouting)
                continue;

            ncc = ehp->ehc_EhciNumCompanions;
            ppc = ehp->ehc_EhciPortsPerComp;
            if (!ncc || !ppc)
                continue;

            if (comp_base >= comp_count)
                break;

            for (lp = 0; lp < ehc->hc_NumPorts && lp < MAX_ROOT_PORTS; lp++) {
                UBYTE gport = ehc->hc_PortNum[lp];
                UWORD comp_index;
                struct PCIController *chc;
                UBYTE clp;
                UBYTE func;

                if (gport == 0xFF || gport >= MAX_ROOT_PORTS)
                    continue;
                if (hu->hu_PortMap11[gport] != NULL)
                    continue;

                comp_index = (UWORD)(lp / ppc);
                if (comp_index >= ncc)
                    continue;
                if ((comp_base + comp_index) >= comp_count)
                    continue;

                chc = comp_list[comp_base + comp_index];
                clp = (UBYTE)(lp % ppc);

                if (clp >= chc->hc_NumPorts) {
                    pciusbWarn("PCI", "Companion function %ld: expected port %ld (ppc=%ld) but controller has only %ld port(s)\n",
                               (ULONG)chc->hc_FunctionNum, (ULONG)clp, (ULONG)ppc, (ULONG)chc->hc_NumPorts);
                    continue;
                }

                hu->hu_PortMap11[gport] = chc;
                hu->hu_PortNum11[gport] = clp;
                chc->hc_PortNum[clp] = gport;

                func = (UBYTE)(chc->hc_FunctionNum & 0xF);
                if (comp_used_ports[func] <= clp)
                    comp_used_ports[func] = clp + 1;

                chc->hc_GlobalPortMask |= (1UL << gport);
                if (chc->hc_GlobalPortFirst == 0xFFFF)
                    chc->hc_GlobalPortFirst = (UWORD)gport;
                chc->hc_GlobalPortLast = (UWORD)gport;
            }

            comp_base += ncc;
        }
    }

    /* Fill any remaining USB 1.1 global ports sequentially across companions (fallback). */
    {
        ULONG gport = 0;
        for (i = 0; i < comp_count && gport < MAX_ROOT_PORTS; i++) {
            struct PCIController *chc = comp_list[i];
            UBYTE func = (UBYTE)(chc->hc_FunctionNum & 0xF);
            UBYTE lp = comp_used_ports[func];

            while (gport < MAX_ROOT_PORTS && hu->hu_PortMap11[gport] != NULL)
                gport++;

            for (; lp < chc->hc_NumPorts && gport < MAX_ROOT_PORTS; lp++) {
                while (gport < MAX_ROOT_PORTS && hu->hu_PortMap11[gport] != NULL)
                    gport++;
                if (gport >= MAX_ROOT_PORTS)
                    break;

                hu->hu_PortMap11[gport] = chc;
                hu->hu_PortNum11[gport] = lp;
                chc->hc_PortNum[lp] = (UBYTE)gport;

                chc->hc_GlobalPortMask |= (1UL << gport);
                if (chc->hc_GlobalPortFirst == 0xFFFF)
                    chc->hc_GlobalPortFirst = (UWORD)gport;
                chc->hc_GlobalPortLast = (UWORD)gport;

                gport++;
            }
            comp_used_ports[func] = lp;
        }
    }

    if ((usb11ports != usb20ports) && usb20ports) {
        pciusbWarn("PCI", "EHCI Ports (%ld), do not match USB 1.1 Ports (%ld)!\n", usb20ports, usb11ports);
    }

    hu->hu_RootHub11Ports = (UWORD)usb11ports;
    hu->hu_RootHub20Ports = (UWORD)usb20ports;
    hu->hu_RootHubPorts = (hu->hu_RootHub11Ports > hu->hu_RootHub20Ports) ? hu->hu_RootHub11Ports : hu->hu_RootHub20Ports;
    if (hu->hu_RootHubPorts > MAX_ROOT_PORTS)
        hu->hu_RootHubPorts = MAX_ROOT_PORTS;

    /* Initial port ownership. */
    for (cnt = 0; cnt < hu->hu_RootHubPorts; cnt++) {
        struct PCIController *ehc = hu->hu_PortMap20[cnt];
        if (ehc) {
            UWORD lp = hu->hu_PortNum20[cnt];
            ULONG portsc = READREG32_LE(ehc->hc_RegBase, EHCI_PORTSC1 + (lp<<2));
            if (portsc & EHPF_NOTPORTOWNER) {
                struct PCIController *chc = hu->hu_PortMap11[cnt];
                hu->hu_PortOwner[cnt] = chc ? chc->hc_HCIType : HCITYPE_UHCI;
            } else {
                hu->hu_PortOwner[cnt] = HCITYPE_EHCI;
            }
        } else if (hu->hu_PortMap11[cnt]) {
            hu->hu_PortOwner[cnt] = hu->hu_PortMap11[cnt]->hc_HCIType;
        } else {
            hu->hu_PortOwner[cnt] = HCITYPE_UHCI;
        }
    }

    pciusbDebug("PCI", "Unit %ld: USB Board %08lx:\n", (hu->hu_UnitNo & ~PCIUSBUNIT_MASK), hu->hu_DevID);
    if (hu->hu_RootHub11Ports) {
        pciusbDebug("PCI", "Unit %ld: - %ld USB1.1 port(s)\n", (hu->hu_UnitNo & ~PCIUSBUNIT_MASK), hu->hu_RootHub11Ports);
    }
    if (hu->hu_RootHub20Ports) {
        pciusbDebug("PCI", "Unit %ld: - %ld USB2.0 port(s)\n", (hu->hu_UnitNo & ~PCIUSBUNIT_MASK), hu->hu_RootHub20Ports);
    }

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
        if(ohcicnt + uhcicnt >1) {
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

    // put em online
    ForeachNode(&hu->hu_Controllers, hc) {
        hc->hc_Flags |= HCF_ONLINE;
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
        switch (hc->hc_HCIType) {
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
    }

    //FIXME: (x/e/o/u)hciFree routines actually ONLY stops the chip NOT free anything as below...
    ForeachNode(&hu->hu_Controllers, hc) {
        if(hc->hc_PCIMem.me_Un.meu_Addr) {
            if (hc->hc_PCIMemIsExec) {
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
            if (hc->hc_Flags & HCF_MSI) {
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
    if (alloc->me_Un.meu_Addr) {
        IPTR addrAligned = (((IPTR)alloc->me_Un.meu_Addr + (align-1)) & ~(align-1));
        if ((addrAligned & (bounds - 1)) != ((addrAligned + Size -1 ) & (bounds-1))) {
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
