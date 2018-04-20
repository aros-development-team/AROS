/*
    Copyright © 2015-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef VUSBHCI_DEVICE_H
#define VUSBHCI_DEVICE_H

#include <aros/debug.h>
#include <aros/macros.h>

#include <exec/semaphores.h>

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

struct VUSBHCIUnit {
    struct Node                  node;
    CONST_STRPTR                 name;
    ULONG                        state;
    BOOL                         allocated;

    BOOL                         ctrlxfer_pending;
    BOOL                         intrxfer_pending;
    BOOL                         bulkxfer_pending;
    BOOL                         isocxfer_pending;

    struct SignalSemaphore       ctrlxfer_queue_lock;
    struct SignalSemaphore       intrxfer_queue_lock;
    struct SignalSemaphore       bulkxfer_queue_lock;
    struct SignalSemaphore       isocxfer_queue_lock;

    struct List                  ctrlxfer_queue;
    struct List                  intrxfer_queue;
    struct List                  bulkxfer_queue;
    struct List                  isocxfer_queue;

    struct VUSBHCIRootHub {

        struct SignalSemaphore   intrxfer_queue_lock;
        struct List              intrxfer_queue; /* Status Change endpoint */

        UWORD                    addr;

        struct UsbStdDevDesc     devdesc;

        struct RHConfig {
            struct UsbStdCfgDesc cfgdesc;
            struct UsbStdIfDesc  ifdesc;
            struct UsbStdEPDesc  epdesc;
        }                        config;

        union {
            struct UsbHubDesc        hubdesc;
            struct UsbSSHubDesc    sshubdesc;
        };

        struct UsbHubStatus      hubstatus;

        struct UsbPortStatus     portstatus;

    }                            roothub;

};

struct VUSBHCIBase {
    struct Device                device;

    struct Task                 *handler_task;
    BOOL                         handler_task_run;

    //struct Library              *HostLibBase;

    struct VUSBHCIUnit          *usbunit200;
    struct VUSBHCIUnit          *usbunit300;
};

BOOL cmdAbortIO(struct IOUsbHWReq *ioreq);
WORD cmdUsbReset(struct IOUsbHWReq *ioreq);
WORD cmdNSDeviceQuery(struct IOStdReq *ioreq);
WORD cmdQueryDevice(struct IOUsbHWReq *ioreq);
WORD cmdControlXFerRootHub(struct IOUsbHWReq *ioreq);
WORD cmdIntXFerRootHub(struct IOUsbHWReq *ioreq);
WORD cmdIntXFer(struct IOUsbHWReq *ioreq);
WORD cmdControlXFer(struct IOUsbHWReq *ioreq);
WORD cmdBulkXFer(struct IOUsbHWReq *ioreq);
WORD cmdISOXFer(struct IOUsbHWReq *ioreq);
void uhwCheckRootHubChanges(struct VUSBHCIUnit *unit);

void call_libusb_event_handler(void);
int do_libusb_ctrl_transfer(struct IOUsbHWReq *ioreq);
int do_libusb_intr_transfer(struct IOUsbHWReq *ioreq);
int do_libusb_bulk_transfer(struct IOUsbHWReq *ioreq);
int do_libusb_isoc_transfer(struct IOUsbHWReq *ioreq);

BOOL libusb_bridge_init(struct VUSBHCIBase *VUSBHCIBase);
VOID libusb_bridge_cleanup();

#endif /* VUSBHCI_DEVICE_H */
