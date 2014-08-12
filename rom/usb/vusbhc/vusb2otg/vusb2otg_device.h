/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#ifndef VUSB2OTG_DEVICE_H
#define VUSB2OTG_DEVICE_H

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

#define MYBUG_LEVEL 0
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
WORD cmdGetString(struct IOUsbHWReq *ioreq, char *cstring);

struct VUSB2OTGPort {
    struct Node                  node;
    char                         name[256];
    ULONG                        number;
    ULONG                        state;
};

struct VUSB2OTGUnit {
    struct Node                  node;
    char                         name[256];
    ULONG                        number;
    ULONG                        state;

    BOOL                         hostmode;
    BOOL                         usb2otg;

    struct VUSB2OTGRootHub {
        struct List              port_list;
        ULONG                    port_count;

        UWORD                    addr;

        struct UsbStdDevDesc     devdesc;

        struct UsbStdBOSDesc     bosdesc;

        struct RHConfig {
            struct UsbStdCfgDesc cfgdesc;
            struct UsbStdIfDesc  ifdesc;
            struct UsbStdEPDesc  epdesc;
        }                        config;


        struct UsbHubDesc        hubdesc;

    }                            roothub;

};

struct VUSB2OTGBase {

    struct Device                device;
    struct List                  unit_list;
    ULONG                        unit_count;

};

#endif /* VUSB2OTG_DEVICE_H */
