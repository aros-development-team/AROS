/*
    Copyright � 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#ifndef PCIXHCI_INTERN_H
#define PCIXHCI_INTERN_H

#include <aros/io.h>
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
#include <devices/timer.h>

#include <asm/io.h>
#include <inttypes.h>

#include <hidd/pci.h>
#include <hidd/hidd.h>

#include LC_LIBDEFS_FILE

#define RC_OK         0
#define RC_DONTREPLY -1

#define MYBUG_LEVEL 1
#define mybug(l, x) D(if ((l>=MYBUG_LEVEL)||(l==-1)) { do { { bug x; } } while (0); } )
#define mybug_unit(l, x) D(if ((l>=MYBUG_LEVEL)||(l==-1)) { do { { bug("%s %s: ", unit->name, __FUNCTION__); bug x; } } while (0); } )

#define PCI_BASE_CLASS_SERIAL 0x0c
#define PCI_SUB_CLASS_USB     0x03
#define PCI_INTERFACE_XHCI    0x30

struct PCIXHCIPort {
    struct Node                  node;
    char                         name[256];
    ULONG                        number;
    ULONG                        status;
};

struct PCIXHCIRootHub {
    struct List                  port_list;

    struct List                  intxferqueue_list;

    UWORD                        addr;

    struct UsbStdDevDesc         devdesc;

    struct UsbStdBOSDesc         bosdesc;

    struct RHConfig {
        struct UsbStdCfgDesc     cfgdesc;
        struct UsbStdIfDesc      ifdesc;
        struct UsbStdEPDesc      epdesc;
    }                            config;

    struct UsbSSHubDesc          hubdesc;

};

struct PCIXHCIHostController {
    OOP_Object                   *pcidevice;
    OOP_Object                   *pcidriver;

    volatile APTR                 capability_base;
    volatile APTR                 operational_base;
    volatile APTR                 doorbell_base;
    volatile APTR                 runtime_base;

    IPTR                          bus;
    IPTR                          dev;
    IPTR                          sub;
    IPTR                          intline;

    ULONG                         pagesize;
    ULONG                         maxslots;
    ULONG                         maxintrs;
    ULONG                         maxscratchpads;
    ULONG                         maxeventringsegments;

    UQUAD                        *dcbaa;     /* Device Context Base Address Array */
    UQUAD                        *spbaba;    /* Scratch Pad Buffer Address Base Array */

    struct PCIXHCIEventRingTable *eventringsegmenttbl;

    char                          intname[256];
    struct Interrupt              inthandler;
};                                

struct PCIXHCIUnit {
    struct Node                  node;
    char                         name[256];
    ULONG                        number;
    ULONG                        state;

    struct PCIXHCIBase          *pcixhcibase;
    struct PCIXHCIRootHub        roothub;
    struct PCIXHCIHostController hc;

    struct timerequest          *tr;

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

VOID PCIXHCI_PCIE(struct PCIXHCIUnit *unit);

BOOL PCIXHCI_Discover(struct PCIXHCIBase *PCIXHCIBase);
BOOL PCIXHCI_HCReset(struct PCIXHCIUnit *unit);
BOOL PCIXHCI_HCHalt(struct PCIXHCIUnit *unit);
BOOL PCIXHCI_GetFromBIOS(struct PCIXHCIUnit *unit);
BOOL PCIXHCI_HCInit(struct PCIXHCIUnit *unit);
BOOL PCIXHCI_FindPorts(struct PCIXHCIUnit *unit);
BOOL PCIXHCI_PortPower(struct PCIXHCIUnit *unit, ULONG portnum, BOOL poweron);
IPTR PCIXHCI_SearchExtendedCap(struct PCIXHCIUnit *unit, ULONG id, IPTR extcapoff);
void PCIXHCI_Delay(struct PCIXHCIUnit *unit, ULONG msec);

void FreeVecOnBoundary(APTR onboundary);
APTR AllocVecOnBoundary(ULONG size, ULONG boundary);
BOOL PCIXHCI_CreateTimer(struct PCIXHCIUnit *unit);
void PCIXHCI_DeleteTimer(struct PCIXHCIUnit *unit);

VOID PCIXHCI_AllocaterInit(struct PCIXHCIUnit *unit);
struct PCIXHCIAlloc *PCIXHCI_AllocateMemory(struct PCIXHCIUnit *unit, IPTR bytesize, ULONG alignmentmin, IPTR boundary, STRPTR allocname);

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
