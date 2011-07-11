/*
    Copyright © 2011, The AROS Development Team. All rights reserved
    $Id$
*/

#ifndef PCIEHCI_H
#define PCIEHCI_H

#include <aros/system.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <exec/exec.h>
#include <devices/newstyle.h>
#include <devices/usbhardware.h>
#include <hidd/pci.h>

#include <proto/exec.h>
#include <proto/debug.h>
#include <proto/oop.h>

#define PCI_BASE_CLASS_SERIAL   0x0c
#define PCI_SUB_CLASS_USB       0x03
#define PCI_INTERFACE_EHCI      0x20
//#define PCI_INTERFACE_EHCI      0x00  /* Really UHCI, used to check unit node creation. Remove later */

#define RC_OK         0                 /* Reply the iorequest with success */
#define RC_DONTREPLY  -1                /* Magic cookie, don't set error fields & don't reply the ioreq */

#undef HiddAttrBase
#undef HiddPCIDeviceAttrBase

#define HiddAttrBase            (ehd->ehd_hiddattrbase)
#define HiddPCIDeviceAttrBase   (ehd->ehd_hiddpcideviceab)

#define DBL_DEVIO 10
#define DEBUG DBL_DEVIO

#ifdef DEBUG
#define KPRINTF(l, x) do { if ((l) >= DEBUG) \
     { KPrintF("%s/%lu: ", __FUNCTION__, __LINE__); KPrintF x;} } while (0)
#define KPRINTF2(l, x) do { if ((l) >= DEBUG) \
     { KPrintF x;} } while (0)
#else /* !DEBUG */
#define KPRINTF(l, x) ((void) 0)
#define KPRINTF2(l, x) ((void) 0)
#endif /* DEBUG */


struct ehc_controller {                 /* EHCI Controller Struct (ehc_) */
    struct MinNode  ehc_contrnode;

    APTR            ehc_unitptr;

    IPTR            ehc_pcibus, ehc_pcidev, ehc_pcisub;
};

struct ehu_unit {                       /* EHCI Unit Structure (ehu_) */
    struct MinNode  ehu_unitnode;

    ULONG           ehu_unitnumber;
    BOOL            ehu_unitallocated;  /* Unit opened */
    IPTR            ehu_pcibus, ehu_pcidev;

    struct List     ehu_cntrlist;
};

/* pciehci.device base*/
struct pciehcibase {                    /* EHCI Device Structure (ehd_) */
    struct Device   ehd_device;

    struct List     ehd_unitlist;       /* Host Controller List */

    APTR            ehd_mempool;

    OOP_Object     *ehd_pcihidd;
    OOP_AttrBase    ehd_hiddattrbase;
    OOP_AttrBase    ehd_hiddpcideviceab;
};

#endif /* PCIEHCI_H */
