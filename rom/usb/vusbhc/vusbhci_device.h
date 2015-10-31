/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#ifndef VUSBHCI_DEVICE_H
#define VUSBHCI_DEVICE_H

#include <aros/debug.h>
#include <aros/macros.h>

#include <proto/exec.h>
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

struct VUSBHCIUnit {
    struct Node                  node;
    char                         name[256];
    ULONG                        state;
    BOOL                         allocated;

    struct VUSBHCIRootHub {
        struct List              io_queue;

        UWORD                    addr;

        BOOL                     attached;
        BOOL                     portchange;

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

    struct Task                 *handler_task;
    BOOL                         handler_task_run;

    struct VUSBHCIUnit          *usbunit200;
    struct VUSBHCIUnit          *usbunit300;
};

#endif /* VUSBHCI_DEVICE_H */
