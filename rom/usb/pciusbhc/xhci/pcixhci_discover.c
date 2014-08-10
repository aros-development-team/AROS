/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI XHCI USB host controller
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include <aros/debug.h>
#include <aros/macros.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/stdc.h>
#include <proto/arossupport.h>

#include <devices/usb.h>
#include <devices/usb_hub.h>
#include <devices/newstyle.h>
#include <devices/usbhardware.h>

#include <asm/io.h>
#include <inttypes.h>

#include <hidd/pci.h>
#include <hidd/hidd.h>

#include "pcixhci_intern.h"

#include LC_LIBDEFS_FILE

/*
    Keep this one short and simple...
*/
static AROS_UFH3(void, GM_UNIQUENAME(Enumerator), AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(OOP_Object *, pciDevice, A2), AROS_UFHA(APTR, message, A1)) {
    AROS_USERFUNC_INIT

    LIBBASETYPE *LIBBASE = (LIBBASETYPE *)hook->h_Data;

    mybug(-1, ("\n[PCIXHCI] Enumerator: Found PCI XHCI host controller\n"));

    struct PCIXHCIUnit *unit;

    unit = AllocVec(sizeof(struct PCIXHCIUnit), MEMF_ANY|MEMF_CLEAR);
    if(unit != NULL) {
        /* PCI device is useless for us if we can't obtain it */
        if(HIDD_PCIDevice_Obtain(pciDevice, LIBBASE->library.lib_Node.ln_Name) == NULL) {
            mybug(-1, ("[PCIXHCI] Enumerator: Host controller free\n"));
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &unit->hc.intline);
            if(unit->hc.intline != 255) {
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Bus,             &unit->hc.bus);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Dev,             &unit->hc.dev);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Sub,             &unit->hc.sub);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver,  (IPTR *)&unit->hc.pcidriver);
                unit->hc.pcidevice = pciDevice;
                AddTail(&LIBBASE->unit_list, (struct Node *)unit);
                mybug(-1, ("[PCIXHCI] Enumerator: Host controller obtained\n"));
            } else {
                mybug(-1, ("[PCIXHCI] Enumerator: Host controller has bogus intline!\n"));
                HIDD_PCIDevice_Release(pciDevice);
                FreeVec(unit);
            }
        } else {
            mybug(-1, ("[PCIXHCI] Enumerator: Host controller could not be obtained\n"));
            FreeVec(unit);
        }
    } else {
        mybug(-1, ("\n[PCIXHCI] Enumerator: Failed to allocate unit controller structure!\n\n"));
    }

    AROS_USERFUNC_EXIT
}

BOOL PCIXHCI_Discover(LIBBASETYPEPTR LIBBASE) {
    mybug(0, ("[PCIXHCI] PCIXHCI_Discover: Entering function\n"));

    NEWLIST(&LIBBASE->unit_list);

    static struct TagItem tags[] = {
            { tHidd_PCI_Class,     PCI_BASE_CLASS_SERIAL },
            { tHidd_PCI_SubClass,  PCI_SUB_CLASS_USB },
            { tHidd_PCI_Interface, PCI_INTERFACE_XHCI },
            { TAG_DONE, 0UL }
    };

    struct Hook FindHook = {
            h_Entry: (IPTR (*)())GM_UNIQUENAME(Enumerator),
            h_Data:  LIBBASE,
    };

    HIDD_PCI_EnumDevices(LIBBASE->pci, &FindHook, (struct TagItem *)&tags);

    struct PCIXHCIUnit *unit;

    ForeachNode(&LIBBASE->unit_list, unit) {
        mybug(-1, ("[PCIXHCI] *pcidevice = %p\n", unit->hc.pcidevice));
        mybug(-1, ("[PCIXHCI] *pcidriver = %p\n", unit->hc.pcidriver));
        mybug(-1, ("[PCIXHCI]  bus       = %x\n", unit->hc.bus));
        mybug(-1, ("[PCIXHCI]  dev       = %x\n", unit->hc.dev));
        mybug(-1, ("[PCIXHCI]  sub       = %x\n", unit->hc.sub));
        mybug(-1, ("[PCIXHCI]  intline   = %d\n", unit->hc.intline));
    }

    if(!IsListEmpty(&LIBBASE->unit_list)) {
        mybug(-1, ("[PCIXHCI] Unit list is not empty\n"));
    } else {
        mybug(-1, ("[PCIXHCI] Unit list is empty\n"));
        return FALSE;
    }

    return TRUE;
}

struct PCIXHCIUnit *PCIXHCI_AddNewUnit(ULONG unitnum, UWORD bcdusb) {

    struct PCIXHCIUnit *unit;
    struct PCIXHCIPort *port;

    ULONG i, imax;

    unit = AllocVec(sizeof(struct PCIXHCIUnit), MEMF_ANY|MEMF_CLEAR);
    if(unit == NULL) {
        mybug(-1, ("[PCIXHCI] PCIXHCI_AddNewUnit: Failed to create new unit structure\n"));
        return NULL;
    } else {
        unit->node.ln_Type = NT_USER;
        unit->number = unitnum;
        unit->node.ln_Name = (STRPTR)&unit->name;
        unit->state = UHSF_SUSPENDED;

        NEWLIST(&unit->roothub.port_list);

        /* Set the correct bcdUSB and bcdDevice for the hub device descriptor */
        if(bcdusb>=0x0300) {
            unit->roothub.devdesc.bcdUSB    = AROS_WORD2LE(0x0300);
        } else {
            unit->roothub.devdesc.bcdUSB    = AROS_WORD2LE(0x0200);
        }

        unit->roothub.devdesc.bcdDevice = AROS_WORD2LE(bcdusb);

        sprintf(unit->name, "PCIXHCI_USB%x%x[%d]", (AROS_LE2WORD(unit->roothub.devdesc.bcdUSB)>>8)&0xf, (AROS_LE2WORD(unit->roothub.devdesc.bcdUSB)>>4)&0xf, unit->number);

        #ifdef PCIXHCI_NUMPORTS20
        if( (bcdusb >= 0x0200) && (bcdusb < 0x0300) ) {
            unit->roothub.devdesc.bMaxPacketSize0      = 8;
            unit->roothub.devdesc.bDeviceProtocol      = 1;
            unit->roothub.config.epdesc.wMaxPacketSize = AROS_WORD2LE(8);
            imax = PCIXHCI_NUMPORTS20;
        } else {
            unit->roothub.devdesc.bMaxPacketSize0      = 9;
            unit->roothub.devdesc.bDeviceProtocol      = 3;
            unit->roothub.config.epdesc.wMaxPacketSize = AROS_WORD2LE(1024);
            imax = PCIXHCI_NUMPORTS30;
        }
        #else
        unit->roothub.devdesc.bMaxPacketSize0      = 9;
        unit->roothub.devdesc.bDeviceProtocol      = 3;
        unit->roothub.config.epdesc.wMaxPacketSize = AROS_WORD2LE(1024);
        imax = PCIXHCI_NUMPORTS30;
        #endif

        for (i=0; i<imax; i++) {

            port = PCIXHCI_AddNewPort(unit, i);
            if(port == NULL) {
                mybug(-1, ("[PCIXHCI] PCIXHCI_AddNewUnit: Failed to create new port structure\n"));

                /*
                    Free previous ports if any exists and delete this unit
                */

                ForeachNode(&unit->roothub.port_list, port) {
                    mybug(-1, ("[PCIXHCI] PCIXHCI_AddNewUnit: Removing port structure %s at %p\n", port->node.ln_Name, port));
                    REMOVE(port);
                    FreeVec(port);
                }
                FreeVec(unit);
                return NULL;
            } else {
                AddTail(&unit->roothub.port_list,(struct Node *)port);
                unit->roothub.port_count++;
            }
        }

        /* This is our root hub device descriptor */
        unit->roothub.devdesc.bLength                       = sizeof(struct UsbStdDevDesc);
        unit->roothub.devdesc.bDescriptorType               = UDT_DEVICE;
        //unit->roothub.devdesc.bcdUSB                        = AROS_WORD2LE(0xJJMN);
        unit->roothub.devdesc.bDeviceClass                  = HUB_CLASSCODE;
        //unit->roothub.devdesc.bDeviceSubClass               = 0;
        //unit->roothub.devdesc.bDeviceProtocol               = 0;
        //unit->roothub.devdesc.bMaxPacketSize0               = 9; // Valid values are 8, 9(SuperSpeed), 16, 32, 64
        //unit->roothub.devdesc.idVendor                      = AROS_WORD2LE(0x0000);
        //unit->roothub.devdesc.idProduct                     = AROS_WORD2LE(0x0000);
        //unit->roothub.devdesc.bcdDevice                     = AROS_WORD2LE(0xJJMN);
        unit->roothub.devdesc.iManufacturer                 = 1;
        unit->roothub.devdesc.iProduct                      = 2;
        //unit->roothub.devdesc.iSerialNumber                 = 0;
        unit->roothub.devdesc.bNumConfigurations            = 1;

        /* This is our root hub config descriptor */
        unit->roothub.config.cfgdesc.bLength                = sizeof(struct UsbStdCfgDesc);
        unit->roothub.config.cfgdesc.bLength                = sizeof(struct UsbStdCfgDesc);
        unit->roothub.config.cfgdesc.bDescriptorType        = UDT_CONFIGURATION;
        unit->roothub.config.cfgdesc.wTotalLength           = AROS_WORD2LE(sizeof(struct RHConfig));
        unit->roothub.config.cfgdesc.bNumInterfaces         = 1;
        unit->roothub.config.cfgdesc.bConfigurationValue    = 1;
        unit->roothub.config.cfgdesc.iConfiguration         = 3;
        unit->roothub.config.cfgdesc.bmAttributes           = (USCAF_ONE|USCAF_SELF_POWERED);
        //unit->roothub.config.cfgdesc.bMaxPower              = 0;

        unit->roothub.config.ifdesc.bLength                 = sizeof(struct UsbStdIfDesc);
        unit->roothub.config.ifdesc.bDescriptorType         = UDT_INTERFACE;
        //unit->roothub.config.ifdesc.bInterfaceNumber        = 0;
        //unit->roothub.config.ifdesc.bAlternateSetting       = 0;
        unit->roothub.config.ifdesc.bNumEndpoints           = 1;
        unit->roothub.config.ifdesc.bInterfaceClass         = HUB_CLASSCODE;
        //unit->roothub.config.ifdesc.bInterfaceSubClass      = 0;
        //unit->roothub.config.ifdesc.bInterfaceProtocol      = 0;
        unit->roothub.config.ifdesc.iInterface              = 4;

        unit->roothub.config.epdesc.bLength                 = sizeof(struct UsbStdEPDesc);
        unit->roothub.config.epdesc.bDescriptorType         = UDT_ENDPOINT;
        unit->roothub.config.epdesc.bEndpointAddress        = (URTF_IN|1);
        unit->roothub.config.epdesc.bmAttributes            = USEAF_INTERRUPT;
        //unit->roothub.config.epdesc.wMaxPacketSize          = AROS_WORD2LE(8);
        unit->roothub.config.epdesc.bInterval               = 12;

        /* This is our root hub hub descriptor */
        if( (bcdusb >= 0x0200) && (bcdusb < 0x0300) ) {
            unit->roothub.hubdesc.usb20.bLength             = sizeof(struct UsbHubDesc);
            unit->roothub.hubdesc.usb20.bDescriptorType     = UDT_HUB;
            unit->roothub.hubdesc.usb20.bNbrPorts           = (UBYTE) unit->roothub.port_count;
            unit->roothub.hubdesc.usb20.wHubCharacteristics = AROS_WORD2LE(UHCF_INDIVID_POWER|UHCF_INDIVID_OVP);
            //unit->roothub.hubdesc.usb20.bPwrOn2PwrGood      = 0;
            unit->roothub.hubdesc.usb20.bHubContrCurrent    = 1;
            unit->roothub.hubdesc.usb20.DeviceRemovable     = 0;
            //unit->roothub.hubdesc.usb20.PortPwrCtrlMask     = 0;
        } else {
            unit->roothub.hubdesc.usb30.bLength             = sizeof(struct UsbSSHubDesc);
            unit->roothub.hubdesc.usb30.bDescriptorType     = UDT_SSHUB;
            unit->roothub.hubdesc.usb30.bNbrPorts           = (UBYTE) unit->roothub.port_count;;
            unit->roothub.hubdesc.usb30.wHubCharacteristics = AROS_WORD2LE(UHCF_INDIVID_POWER|UHCF_INDIVID_OVP);
            //unit->roothub.hubdesc.usb30.bPwrOn2PwrGood      = 0;
            unit->roothub.hubdesc.usb30.bHubContrCurrent    = 10;
            //unit->roothub.hubdesc.usb30.bHubHdrDecLat       = 0;
            //unit->roothub.hubdesc.usb30.wHubDelay           = 0;
            //unit->roothub.hubdesc.usb30.DeviceRemovable     = 0;
        }

        D( mybug(0, ("[PCIXHCI] PCIXHCI_AddNewUnit:\n"));
        mybug(0, ("        Created new unit numbered %d at %p\n",unit->number, unit));
        mybug(0, ("        Unit node name %s\n", unit->node.ln_Name));

        switch(unit->state) {
            case UHSF_SUSPENDED:
                mybug(0, ("        Unit state: UHSF_SUSPENDED\n"));
                break;
            case UHSF_OPERATIONAL:
                mybug(0, ("        Unit state: UHSF_OPERATIONAL\n"));
                break;
            default:
                mybug(0, ("        Unit state: %lx (Error?)\n", unit->state));
                break;
        } );

        return unit;
    }
}

struct PCIXHCIPort *PCIXHCI_AddNewPort(struct PCIXHCIUnit *unit, ULONG portnum) {
    struct PCIXHCIPort *port;

    port = AllocVec(sizeof(struct PCIXHCIPort), MEMF_ANY|MEMF_CLEAR);
    if(port == NULL) {
        mybug(-1, ("[PCIXHCI] PCIXHCI_AddNewPort: Failed to create new port structure\n"));
        return NULL;
    } else {
        port->node.ln_Type = NT_USER;
        /* Poseidon treats port number 0 as a place for roothub */
        port->number = portnum+1;

        sprintf(port->name, "PCIXHCI_USB%x%x[%d:%d]", (AROS_LE2WORD(unit->roothub.devdesc.bcdUSB)>>8)&0xf, (AROS_LE2WORD(unit->roothub.devdesc.bcdUSB)>>4)&0xf, unit->number, port->number);
        port->node.ln_Name = (STRPTR)&port->name;
    }

    mybug(0, ("[PCIXHCI] PCIXHCI_AddNewPort:\n"));
    mybug(0, ("        Created new port numbered %d at %p\n",port->number, port));
    mybug(0, ("        Port node name %s\n", port->node.ln_Name));

    return port;
}


