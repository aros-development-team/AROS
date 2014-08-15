/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#ifndef PCIXHCI_INTERN_H
#define PCIXHCI_INTERN_H

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

#include LC_LIBDEFS_FILE

/* Number of host controllers */
#define PCIXHCI_NUMCONTROLLERS 1

/* Number of ports per host controller (USB2.0/USB3.0) */
//#define PCIXHCI_NUMPORTS20 2
#define PCIXHCI_NUMPORTS30 4

#define RC_OK         0
#define RC_DONTREPLY -1

#define MYBUG_LEVEL 0
#define mybug(l, x) D(if ((l>=MYBUG_LEVEL)||(l==-1)) { do { { bug x; } } while (0); } )
#define mybug_unit(l, x) D(if ((l>=MYBUG_LEVEL)||(l==-1)) { do { { bug("%s %s: ", unit->name, __FUNCTION__); bug x; } } while (0); } )

#define PCI_BASE_CLASS_SERIAL 0x0c
#define PCI_SUB_CLASS_USB     0x03
#define PCI_INTERFACE_XHCI    0x30

struct PCIXHCIPort {
    struct Node                  node;
    char                         name[256];
    ULONG                        number;
    ULONG                        state;
};

struct PCIXHCIRootHub {
    struct List                  port_list;
    ULONG                        port_count;

    UWORD                        addr;

    struct UsbStdDevDesc         devdesc;

    struct RHConfig {
        struct UsbStdCfgDesc     cfgdesc;
        struct UsbStdIfDesc      ifdesc;
        struct UsbStdEPDesc      epdesc;
    }                            config;

    struct UsbSSHubDesc          hubdesc;

};

struct PCIXHCIHostController {
    OOP_Object                  *pcidevice;
    OOP_Object                  *pcidriver;

    volatile APTR                pcibase0;

    IPTR bus;
    IPTR dev;
    IPTR sub;
    IPTR intline;
};                                

struct PCIXHCIUnit {
    struct Node                  node;
    char                         name[256];
    ULONG                        number;
    ULONG                        state;

    struct PCIXHCIBase          *pcixhcibase;
    struct PCIXHCIRootHub        roothub;
    struct PCIXHCIHostController hc;
};

struct PCIXHCIBase {

    struct Library               library;
    struct List                  unit_list;

    OOP_Object                  *pci;
    OOP_AttrBase                 HiddAB;
    OOP_AttrBase                 HiddPCIDeviceAB;

};

#undef HiddAttrBase
#undef HiddPCIDeviceAttrBase
#define HiddAttrBase (LIBBASE->HiddAB)
#define HiddPCIDeviceAttrBase (LIBBASE->HiddPCIDeviceAB)

BOOL PCIXHCI_Discover(struct PCIXHCIBase *PCIXHCIBase);
BOOL PCIXHCI_HCReset(struct PCIXHCIUnit *unit);
BOOL PCIXHCI_HCInit(struct PCIXHCIUnit *unit);

BOOL cmdAbortIO(struct IOUsbHWReq *ioreq);
WORD cmdReset(struct IOUsbHWReq *ioreq);
WORD cmdUsbReset(struct IOUsbHWReq *ioreq);
WORD cmdNSDeviceQuery(struct IOStdReq *ioreq);
WORD cmdQueryDevice(struct IOUsbHWReq *ioreq);
WORD cmdControlXFer(struct IOUsbHWReq *ioreq);
WORD cmdControlXFerRootHub(struct IOUsbHWReq *ioreq);
WORD cmdIntXFer(struct IOUsbHWReq *ioreq);
WORD cmdIntXFerRootHub(struct IOUsbHWReq *ioreq);
WORD cmdGetString(struct IOUsbHWReq *ioreq, char *cstring);

#endif /* PCIXHCI_INTERN_H */
