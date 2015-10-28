/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id: vusbhci_device.h 49315 2014-08-12 09:53:29Z DizzyOfCRN $

    Desc:
    Lang: English
*/
#ifndef VUSBHCI_DEVICE_H
#define VUSBHCI_DEVICE_H

#include <aros/debug.h>
#include <aros/macros.h>

#include <proto/exec.h>
#include <proto/stdc.h>
#include <proto/arossupport.h>

#include <devices/usb.h>
#include <devices/usb_hub.h>
#include <devices/newstyle.h>
#include <devices/usbhardware.h>

#define RC_OK         0
#define RC_DONTREPLY -1

#define MYBUG_LEVEL 1
#define mybug(l, x) D(if ((l>=MYBUG_LEVEL)||(l==-1)) { do { { bug x; } } while (0); } )
#define mybug_unit(l, x) D(if ((l>=MYBUG_LEVEL)||(l==-1)) { do { { bug("%s %s: ", unit->name, __FUNCTION__); bug x; } } while (0); } )

BOOL cmdAbortIO(struct IOUsbHWReq *ioreq);
WORD cmdUsbReset(struct IOUsbHWReq *ioreq);
WORD cmdNSDeviceQuery(struct IOStdReq *ioreq);
WORD cmdQueryDevice(struct IOUsbHWReq *ioreq);
WORD cmdControlXFer(struct IOUsbHWReq *ioreq);
WORD cmdControlXFerRootHub(struct IOUsbHWReq *ioreq);
WORD cmdIntXFer(struct IOUsbHWReq *ioreq);
WORD cmdIntXFerRootHub(struct IOUsbHWReq *ioreq);
WORD cmdISOXFer(struct IOUsbHWReq *ioreq);
WORD cmdGetString(struct IOUsbHWReq *ioreq, char *cstring);

struct VUSBHCIPort {
    struct Node                  node;
    char                         name[256];
    ULONG                        number;
    ULONG                        state;
    BOOL                         attachment;
    BOOL                         detachment;
};

struct VUSBHCIUnit {
    struct Node                  node;
    char                         name[256];
    ULONG                        number;
    ULONG                        state;
    BOOL                         allocated;

    struct VUSBHCIRootHub {
        struct List              port_list;
        /* FIXME: Use roothub descriptor exlusively to store this kind of information, use of port_count is redundant */
        ULONG                    port_count;

        struct List              io_queue;

        UWORD                    addr;

        struct UsbStdDevDesc     devdesc;

        struct RHConfig {
            struct UsbStdCfgDesc cfgdesc;
            struct UsbStdIfDesc  ifdesc;
            struct UsbStdEPDesc  epdesc;
        }                        config;

        struct UsbHubDesc        hubdesc;

    }                            roothub;

};

struct VUSBHCIBase {

    struct Device                device;
    struct List                  unit_list;
    ULONG                        unit_count;

};

#endif /* VUSBHCI_DEVICE_H */
