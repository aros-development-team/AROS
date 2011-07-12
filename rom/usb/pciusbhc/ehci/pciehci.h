/*
    Copyright © 2011, The AROS Development Team. All rights reserved
    $Id$
*/

#ifndef PCIEHCI_H
#define PCIEHCI_H

#include <aros/io.h>
#include <aros/system.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <exec/exec.h>

#include <devices/timer.h>
#include <devices/newstyle.h>
#include <devices/usbhardware.h>

#include <hidd/pci.h>

#include <proto/exec.h>
#include <proto/debug.h>
#include <proto/oop.h>
#include <proto/utility.h>

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

#define DBL_DEVIO   5
#define DBL_UHWIO   10
#define DEBUG       DBL_DEVIO

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
    APTR            ehc_devicebase;

    OOP_Object     *ehc_pcideviceobject;
    OOP_Object     *ehc_pcidriverobject;

    ULONG           ehc_EhciUsbCmd;
    ULONG          *ehc_EhciFrameList;
    struct EhciQH  *ehc_EhciQHPool;
    struct EhciTD  *ehc_EhciTDPool;

    struct EhciQH  *ehc_EhciAsyncQH;
    struct EhciQH  *ehc_EhciIntQH[11];
    struct EhciQH  *ehc_EhciTermQH;
    volatile BOOL   ehc_AsyncAdvanced;
    struct EhciQH  *ehc_EhciAsyncFreeQH;
    struct EhciTD  *ehc_ShortPktEndTD;

    ULONG           ehc_FrameCounter;
    struct List     ehc_TDQueue;
    struct List     ehc_AbortQueue;
    struct List     ehc_PeriodicTDQueue;
    struct MinList  ehc_CtrlXFerQueue;
    struct MinList  ehc_IntXFerQueue;
    struct MinList  ehc_IsoXFerQueue;
    struct MinList  ehc_BulkXFerQueue;

    volatile APTR   ehc_opregbase;

    IPTR            ehc_pcibus, ehc_pcidev, ehc_pcisub, ehc_intline;
};

struct Unitnode {
    struct MinNode  ehu_unitnode;;
    APTR            ehu_unitptr;
};

struct ehu_unit {                       /* EHCI Unit Structure (ehu_) */
    struct Unit     ehu_devunit;

    struct Unitnode ehu_unitnode;

    ULONG           ehu_unitnumber;

    struct timerequest    hu_NakTimeoutReq;
    struct MsgPort        hu_NakTimeoutMsgPort;
    struct Interrupt      hu_NakTimeoutInt;

    BOOL            ehu_unitallocated;  /* Unit opened */
    IPTR            ehu_pcibus, ehu_pcidev;

    UWORD           ehu_RootHubAddr;    /* Root Hub Address */
    ULONG           ehu_FrameCounter;   /* Common frame counter */
    struct IOUsbHWReq    *ehu_DevBusyReq[128*16*2]; /* pointer to io assigned to the Endpoint */
    ULONG           ehu_NakTimeoutFrame[128*16*2]; /* Nak Timeout framenumber */
    UBYTE           ehu_DevDataToggle[128*16*2]; /* Data toggle bit for endpoints */

    struct MinList  ehu_cntrlist;
};

/* pciehci.device base*/
struct pciehcibase {                    /* EHCI Device Structure (ehd_) */
    struct Device   ehd_device;

    struct MinList  ehd_unitlist;       /* Host Controller List */

    APTR            ehd_mempool;

    OOP_Object     *ehd_pcihidd;
    OOP_AttrBase    ehd_hiddattrbase;
    OOP_AttrBase    ehd_hiddpcideviceab;
};

#endif /* PCIEHCI_H */
