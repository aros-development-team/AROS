#ifndef PCIUHCI_H
#define PCIUHCI_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for pciuhci.device
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include LC_LIBDEFS_FILE

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/initializers.h>
#include <dos/dos.h>

#include <devices/timer.h>
#include <utility/utility.h>

#include <devices/usbhardware.h>
#include <devices/newstyle.h>

#include <oop/oop.h>

#include "debug.h"

/* Reply the iorequest with success */
#define RC_OK         0

/* Magic cookie, don't set error fields & don't reply the ioreq */
#define RC_DONTREPLY  -1

#define MAX_ROOT_PORTS   16

/* The unit node - private */
struct PCIUnit {
    struct Unit           hu_Unit;
    LONG                  hu_UnitNo;

    struct PCIDevice     *hu_Device;         /* Uplink */

    struct MsgPort       *hu_MsgPort;
    struct timerequest   *hu_TimerReq;       /* Timer I/O Request */

    struct timerequest    hu_LateIOReq;      /* Timer I/O Request */
    struct MsgPort        hu_LateMsgPort;

    struct timerequest    hu_NakTimeoutReq;
    struct MsgPort        hu_NakTimeoutMsgPort;
    struct Interrupt      hu_NakTimeoutInt;

    BOOL                  hu_UnitAllocated;  /* Unit opened */

    ULONG                 hu_DevID;          /* Device ID (BusID+DevNo) */
    struct List           hu_Controllers;    /* List of controllers */

    UWORD                 hu_RootHubPorts;
    UWORD                 hu_RootHubAddr;    /* Root Hub Address */
    UWORD                 hu_RootPortChanges; /* Merged root hub changes */

    struct List           hu_RHIOQueue;      /* Root Hub Pending IO Requests */

    struct PCIController *hu_PortMap11[MAX_ROOT_PORTS]; /* Maps from Global Port to USB 1.1 controller */
    UBYTE                 hu_PortNum11[MAX_ROOT_PORTS]; /* Maps from Global Port to USB 1.1 companion controller port */

    struct PCIController *hu_DevControllers[128]; /* maps from Device address to controller */
    struct IOUsbHWReq    *hu_DevBusyReq[128*16*2]; /* pointer to io assigned to the Endpoint */
    ULONG                 hu_NakTimeoutFrame[128*16*2]; /* Nak Timeout framenumber */
    UBYTE                 hu_DevDataToggle[128*16*2]; /* Data toggle bit for endpoints */
};

#define HCITYPE_UHCI     0x00

struct PCIController {
    struct Node           hc_Node;
    struct PCIDevice     *hc_Device;        /* Uplink */
    struct PCIUnit       *hc_Unit;          /* Uplink */

    OOP_Object           *hc_PCIDeviceObject;
    OOP_Object           *hc_PCIDriverObject;
    ULONG                 hc_DevID;
    UWORD                 hc_FunctionNum;
    UWORD                 hc_HCIType;
    UWORD                 hc_NumPorts;
    UWORD                 hc_Flags;         /* See below */

    volatile APTR         hc_RegBase;

    APTR                  hc_PCIMem;
    ULONG                 hc_PCIMemSize;
    ULONG                 hc_PCIVirtualAdjust;
    IPTR                  hc_PCIIntLine;
    struct Interrupt      hc_PCIIntHandler;
    ULONG                 hc_PCIIntEnMask;

    ULONG                *hc_UhciFrameList;
    struct UhciQH        *hc_UhciQHPool;
    struct UhciTD        *hc_UhciTDPool;

    struct UhciQH        *hc_UhciCtrlQH;
    struct UhciQH        *hc_UhciBulkQH;
    struct UhciQH        *hc_UhciIntQH[9];
    struct UhciTD        *hc_UhciIsoTD;
    struct UhciQH        *hc_UhciTermQH;


    volatile BOOL         hc_AsyncAdvanced;

    struct EhciTD        *hc_ShortPktEndTD;

    ULONG                 hc_FrameCounter;
    struct List           hc_TDQueue;
    struct List           hc_AbortQueue;
    struct List           hc_PeriodicTDQueue;
    struct List           hc_CtrlXFerQueue;
    struct List           hc_IntXFerQueue;
    struct List           hc_IsoXFerQueue;
    struct List           hc_BulkXFerQueue;

    struct Interrupt      hc_CompleteInt;
    struct Interrupt      hc_ResetInt;

    UBYTE                 hc_PortNumGlobal[MAX_ROOT_PORTS]; /* Contains per unit assigned port number, HC has local ports 0 to 1  */
    UWORD                 hc_PortChangeMap[MAX_ROOT_PORTS]; /* Port Change Map */

    BOOL                  hc_complexrouting;
    UQUAD                 hc_portroute;

};

/* hc_Flags */
#define HCF_ALLOCATED   0x0001  /* PCI board allocated */
#define HCF_ONLINE      0x0002  /* Online */
#define HCF_STOP_BULK   0x0004  /* Bulk transfers stopped */
#define HCF_STOP_CTRL   0x0008  /* Control transfers stopped */
#define HCF_ABORT       0x0010  /* Aborted requests available */

/* The device node - private
*/
struct PCIDevice {
    struct Library      hd_Library;       /* standard */
    UWORD               hd_Flags;         /* various flags */

    struct List         hd_TempHCIList;
    OOP_Object         *hd_PCIHidd;
    OOP_AttrBase        hd_HiddAB;
    OOP_AttrBase        hd_HiddPCIDeviceAB;

    BOOL                hd_ScanDone;      /* PCI scan done? */
    APTR                hd_MemPool;       /* Memory Pool */

	struct List         hd_Units;         /* List of units */
};

/* hd_Flags */
#define HDF_FORCEPOWER	0x01

#endif /* PCIUHCI_H */
