#ifndef PCIUSB_H
#define PCIUSB_H

/*
 *----------------------------------------------------------------------------
 *			   Includes for pciusb.device
 *----------------------------------------------------------------------------
 *		     By Chris Hodges <chrisly@platon42.de>
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
#define RC_OK	      0

/* Magic cookie, don't set error fields & don't reply the ioreq */
#define RC_DONTREPLY  -1

#define MAX_ROOT_PORTS	 16
#define MAX_USB3_PORTS	 255

#define PCI_CLASS_SERIAL_USB 0x0c03

/* The unit node - private */
struct PCIUnit
{
    struct Unit		  hu_Unit;
    LONG		  hu_UnitNo;

    struct PCIDevice	 *hu_Device;	     /* Uplink */

    struct MsgPort	 *hu_MsgPort;
    struct timerequest	 *hu_TimerReq;	     /* Timer I/O Request */

    struct timerequest	  hu_LateIOReq;	     /* Timer I/O Request */
    struct MsgPort	  hu_LateMsgPort;

    struct timerequest	  hu_NakTimeoutReq;
    struct MsgPort	  hu_NakTimeoutMsgPort;
    struct Interrupt	  hu_NakTimeoutInt;

    BOOL		  hu_UnitAllocated;  /* Unit opened */

    ULONG		  hu_DevID;	     /* Device ID (BusID+DevNo) */
    struct List		  hu_Controllers;    /* List of controllers */
    UWORD		  hu_RootHub11Ports;
    UWORD		  hu_RootHub20Ports;
#ifdef AROS_USB30_CODE
    UWORD		  hu_RootHub30Ports;
#endif
    UWORD		  hu_RootHubPorts;
    UWORD		  hu_RootHubAddr;    /* Root Hub Address */
    UWORD		  hu_RootPortChanges; /* Merged root hub changes */
    ULONG		  hu_FrameCounter;   /* Common frame counter */
    struct List		  hu_RHIOQueue;	     /* Root Hub Pending IO Requests */

    struct PCIController *hu_PortMap11[MAX_ROOT_PORTS]; /* Maps from Global Port to USB 1.1 controller */
    struct PCIController *hu_PortMap20[MAX_ROOT_PORTS]; /* Maps from Global Port to USB 2.0 controller */
#ifdef AROS_USB30_CODE
    struct PCIController *hu_PortMap30[MAX_USB3_PORTS]; /* Maps from Global Port to USB 3.0 controller */
#endif
    UBYTE		  hu_PortNum11[MAX_ROOT_PORTS]; /* Maps from Global Port to USB 1.1 companion controller port */
    UBYTE		  hu_EhciOwned[MAX_ROOT_PORTS]; /* TRUE, if currently owned by EHCI */
    UBYTE		  hu_ProductName[80]; /* for Query device */
    struct PCIController *hu_DevControllers[128]; /* maps from Device address to controller */
    struct IOUsbHWReq	 *hu_DevBusyReq[128*16*2]; /* pointer to io assigned to the Endpoint */
    ULONG		  hu_NakTimeoutFrame[128*16*2]; /* Nak Timeout framenumber */
    UBYTE		  hu_DevDataToggle[128*16*2]; /* Data toggle bit for endpoints */
};

#define HCITYPE_UHCI	 0x00
#define HCITYPE_OHCI	 0x10
#define HCITYPE_EHCI	 0x20
#ifdef AROS_USB30_CODE
#define HCITYPE_XHCI	 0x30
#endif

struct PCIController
{
    struct Node		  hc_Node;
    struct PCIDevice	 *hc_Device;	    /* Uplink */
    struct PCIUnit	 *hc_Unit;	    /* Uplink */

    OOP_Object		 *hc_PCIDeviceObject;
    OOP_Object		 *hc_PCIDriverObject;
    ULONG		  hc_DevID;
    UWORD		  hc_FunctionNum;
    UWORD		  hc_HCIType;
    UWORD		  hc_NumPorts;
    UWORD		  hc_Flags;	    /* See below */

    volatile APTR	  hc_RegBase;

    #ifdef AROS_USB30_CODE
    volatile APTR	  xhc_capregbase;
    volatile APTR	  xhc_opregbase;
    volatile APTR	  xhc_doorbellbase;
    volatile APTR	  xhc_runtimebase;

    ULONG		  xhc_pagesize;

    ULONG		  xhc_maxslots;

    UQUAD		 *xhc_dcbaa;
    ULONG		  xhc_scratchpads;
    UQUAD		 *xhc_scratchpadarray;

    BOOL		  xhc_contextsize64;
 
    UWORD		  xhc_NumPorts;
    UWORD		  xhc_NumPorts20;
    UWORD		  xhc_NumPorts30;
    #endif

    APTR		  hc_PCIMem;
    ULONG		  hc_PCIMemSize;
    IPTR		  hc_PCIVirtualAdjust;
    IPTR		  hc_PCIIntLine;
    struct Interrupt	  hc_PCIIntHandler;
    ULONG		  hc_PCIIntEnMask;

    ULONG		 *hc_UhciFrameList;
    struct UhciQH	 *hc_UhciQHPool;
    struct UhciTD	 *hc_UhciTDPool;

    struct UhciQH	 *hc_UhciCtrlQH;
    struct UhciQH	 *hc_UhciBulkQH;
    struct UhciQH	 *hc_UhciIntQH[9];
    struct UhciTD	 *hc_UhciIsoTD;
    struct UhciQH	 *hc_UhciTermQH;

    ULONG		  hc_EhciUsbCmd;
    ULONG		 *hc_EhciFrameList;
    struct EhciQH	 *hc_EhciQHPool;
    struct EhciTD	 *hc_EhciTDPool;

    struct EhciQH	 *hc_EhciAsyncQH;
    struct EhciQH	 *hc_EhciIntQH[11];
    struct EhciQH	 *hc_EhciTermQH;
    volatile BOOL	  hc_AsyncAdvanced;
    struct EhciQH	 *hc_EhciAsyncFreeQH;
    struct EhciTD	 *hc_ShortPktEndTD;

    struct OhciED	 *hc_OhciCtrlHeadED;
    struct OhciED	 *hc_OhciCtrlTailED;
    struct OhciED	 *hc_OhciBulkHeadED;
    struct OhciED	 *hc_OhciBulkTailED;
    struct OhciED	 *hc_OhciIntED[5];
    struct OhciED	 *hc_OhciTermED;
    struct OhciTD	 *hc_OhciTermTD;
    struct OhciHCCA	 *hc_OhciHCCA;
    struct OhciED	 *hc_OhciEDPool;
    struct OhciTD	 *hc_OhciTDPool;
    struct OhciED	 *hc_OhciAsyncFreeED;
    ULONG		  hc_OhciDoneQueue;
    struct List		  hc_OhciRetireQueue;

    ULONG		  hc_FrameCounter;
    struct List		  hc_TDQueue;
    struct List		  hc_AbortQueue;
    struct List		  hc_PeriodicTDQueue;
    struct List		  hc_CtrlXFerQueue;
    struct List		  hc_IntXFerQueue;
    struct List		  hc_IsoXFerQueue;
    struct List		  hc_BulkXFerQueue;

    struct Interrupt	  hc_CompleteInt;
    struct Interrupt	  hc_ResetInt;

    UBYTE		  hc_PortNum20[MAX_ROOT_PORTS];	    /* Global Port number the local controller port corresponds with */

    UWORD		  hc_PortChangeMap[MAX_ROOT_PORTS]; /* Port Change Map */

    BOOL		  hc_complexrouting;
    ULONG		  hc_portroute;

};

/* hc_Flags */
#define HCF_ALLOCATED	0x0001	/* PCI board allocated		 */
#define HCF_ONLINE	0x0002	/* Online			 */
#define HCF_STOP_BULK	0x0004	/* Bulk transfers stopped	 */
#define HCF_STOP_CTRL	0x0008	/* Control transfers stopped	 */
#define HCF_ABORT	0x0010	/* Aborted requests available	 */

/* The device node - private
*/
struct PCIDevice
{
    struct Library	hd_Library;	  /* standard */
    UWORD		hd_Flags;	  /* various flags */

    struct UtilityBase *hd_UtilityBase;	  /* for tags etc */

    struct List		hd_TempHCIList;
    OOP_Object	       *hd_PCIHidd;
    OOP_AttrBase	hd_HiddAB;
    OOP_AttrBase	hd_HiddPCIDeviceAB;
    OOP_MethodID        hd_HiddPCIDeviceMB;

    BOOL		hd_ScanDone;	  /* PCI scan done? */
    APTR		hd_MemPool;	  /* Memory Pool */

	struct List	    hd_Units;	      /* List of units */
};

/* hd_Flags */
#define HDF_FORCEPOWER	0x01

#endif /* PCIUSB_H */
