#ifndef PCIUSB_H
#define PCIUSB_H

/*
 *----------------------------------------------------------------------------
 *			   Includes for pciusb.device
 *----------------------------------------------------------------------------
 *		     By Chris Hodges <chrisly@platon42.de>
 */

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
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hidd/usb.h>
#include <hidd/system.h>

#include "debug.h"

//#define TMPXHCICODE
#if defined(__OOP_NOLIBBASE__) && defined(__OOP_NOMETHODBASES__)
#define USE_FAST_PCICFG
#endif

/* Reply the iorequest with success */
#define RC_OK	                        0

/* Magic cookie, don't set error fields & don't reply the ioreq */
#define RC_DONTREPLY                    -1

#define MAX_ROOT_PORTS	                16

#define PCI_CLASS_SERIAL_USB            0x0c03

#define USB_DEV_MAX                     128
#define USB_PORTDEV_CNT                 (USB_DEV_MAX * MAX_ROOT_PORTS * 2)

struct RTIsoNode
{
    struct MinNode          rtn_Node;
    struct IOUsbHWRTIso     *rtn_RTIso;
    ULONG                   rtn_NextPTD;
    struct PTDNode          *rtn_PTDs[2];
    struct IOUsbHWBufferReq rtn_BufferReq;
    struct IOUsbHWReq       rtn_IOReq;
    UWORD                   rtn_Dummy;
};

/* The unit node - private */
struct PCIUnit
{
    struct Unit		        hu_Unit;
    LONG		            hu_UnitNo;

    struct PCIDevice	    *hu_Device;	                        /* Uplink                                                       */

    struct MsgPort	        *hu_MsgPort;
    struct timerequest	    *hu_TimerReq;	                    /* Timer I/O Request                                            */

    struct timerequest	    hu_LateIOReq;	                    /* Timer I/O Request                                            */
    struct MsgPort	        hu_LateMsgPort;

    struct timerequest	    hu_NakTimeoutReq;
    struct MsgPort	        hu_NakTimeoutMsgPort;
    struct Interrupt	    hu_NakTimeoutInt;
    struct Interrupt	    hu_PeriodicInt;

    BOOL		            hu_UnitAllocated;                   /* Unit opened                                                  */

    ULONG		            hu_DevID;	                        /* Device ID (BusID+DevNo)                                      */
    struct List		        hu_Controllers;                     /* List of controllers                                          */
    UWORD		            hu_RootHub11Ports;
    UWORD		            hu_RootHub20Ports;
    UWORD		            hu_RootHubPorts;
    UWORD		            hu_RootHubAddr;                     /* Root Hub Address                                             */
    UWORD		            hu_RootPortChanges;                 /* Merged root hub changes                                      */
    ULONG		            hu_FrameCounter;                    /* Common frame counter                                         */
    struct List		        hu_RHIOQueue;	                    /* Root Hub Pending IO Requests                                 */

    struct MemEntry         hu_PFLRaw;
    struct MemEntry         hu_PFL;                             /* Page aligned periodic frame list                             */

    struct MemEntry         hu_TDRaw;
    struct MemEntry         hu_TDs;                             /* 32byte aligned transfer descriptors                          */

    struct MinList          hu_FreeRTIsoNodes;

    struct PCIController    *hu_PortMap11[MAX_ROOT_PORTS];      /* Maps from Global Port to USB 1.1 controller                  */
    struct PCIController    *hu_PortMap20[MAX_ROOT_PORTS];      /* Maps from Global Port to USB 2.0 controller                  */
#if defined(TMPXHCICODE)
    struct PCIController    *hu_PortMapX[MAX_ROOT_PORTS];       /* Maps from Global Port to XHCI controller                     */
#endif
    UBYTE		            hu_PortNum11[MAX_ROOT_PORTS];       /* Maps from Global Port to USB 1.1 companion controller port   */
#if defined(TMPXHCICODE)
    UBYTE		            hu_PortNum20[MAX_ROOT_PORTS];       /* Maps from Global Port to USB 2.0 companion controller port   */
#endif
    UBYTE		            hu_PortOwner[MAX_ROOT_PORTS];       /* contains the HCITYPE of the ports current owner              */
    UBYTE		            hu_ProductName[80];                 /* for Query device                                             */
    struct PCIController    *hu_DevControllers[USB_DEV_MAX];    /* maps from Device address to controller                       */
    struct IOUsbHWReq       *hu_DevBusyReq[USB_PORTDEV_CNT];    /* pointer to io assigned to the Endpoint                       */
    ULONG		            hu_NakTimeoutFrame[USB_PORTDEV_CNT];/* Nak Timeout framenumber                                      */
    UBYTE		            hu_DevDataToggle[USB_PORTDEV_CNT];  /* Data toggle bit for endpoints                                */
    struct RTIsoNode        hu_RTIsoNodes[MAX_ROOT_PORTS];
};

/* HCITYPE_xxx, is the pci device interface */
#define HCITYPE_UHCI	                0x00
#define HCITYPE_OHCI	                0x10
#define HCITYPE_EHCI	                0x20
#define HCITYPE_XHCI	                0x30

struct PCIController
{
    struct Node		        hc_Node;
    struct PCIDevice	    *hc_Device;	                        /* Uplink */
    struct PCIUnit	        *hc_Unit;	                        /* Uplink */

    OOP_Object		        *hc_PCIDeviceObject;
    OOP_Object		        *hc_PCIDriverObject;
    
#if defined(USE_FAST_PCICFG)
    OOP_MethodFunc         hc_ReadConfigByte;
    OOP_Class 	          *hc_ReadConfigByte_Class;
    OOP_MethodFunc         hc_ReadConfigWord;
    OOP_Class 	          *hc_ReadConfigWord_Class;
    OOP_MethodFunc         hc_ReadConfigLong;
    OOP_Class 	          *hc_ReadConfigLong_Class;
    OOP_MethodFunc         hc_WriteConfigByte;
    OOP_Class 	          *hc_WriteConfigByte_Class;
    OOP_MethodFunc         hc_WriteConfigWord;
    OOP_Class 	          *hc_WriteConfigWord_Class;
    OOP_MethodFunc         hc_WriteConfigLong;
    OOP_Class 	          *hc_WriteConfigLong_Class;
#endif

    ULONG		            hc_DevID;
    UWORD		            hc_FunctionNum;
    UWORD		            hc_HCIType;
    UWORD		            hc_NumPorts;
    UWORD		            hc_Flags;	                        /* See below */
    ULONG		            hc_Quirks;	                        /* See below */

    volatile APTR	        hc_RegBase;

    APTR		            hc_PCIMem;
    ULONG		            hc_PCIMemSize;
    IPTR		            hc_PCIVirtualAdjust;
    IPTR		            hc_PCIIntLine;
    struct Interrupt	    hc_PCIIntHandler;
    ULONG		            hc_PCIIntEnMask;

    ULONG		            *hc_UhciFrameList;
    struct UhciQH	        *hc_UhciQHPool;
    struct UhciTD	        *hc_UhciTDPool;

    struct UhciQH	        *hc_UhciCtrlQH;
    struct UhciQH	        *hc_UhciBulkQH;
    struct UhciQH	        *hc_UhciIntQH[9];
    struct UhciTD	        *hc_UhciIsoTD;
    struct UhciQH	        *hc_UhciTermQH;

    ULONG		            hc_EhciUsbCmd;
    ULONG		            *hc_EhciFrameList;
    struct EhciQH	        *hc_EhciQHPool;
    struct EhciTD	        *hc_EhciTDPool;

    struct EhciQH	        *hc_EhciAsyncQH;
    struct EhciQH	        *hc_EhciIntQH[11];
    struct EhciQH	        *hc_EhciTermQH;
    volatile BOOL	        hc_AsyncAdvanced;
    struct EhciQH	        *hc_EhciAsyncFreeQH;
    struct EhciTD	        *hc_ShortPktEndTD;
    UWORD		            hc_EhciTimeoutShift;


    struct OhciED	        *hc_OhciCtrlHeadED;
    struct OhciED	        *hc_OhciCtrlTailED;
    struct OhciED	        *hc_OhciBulkHeadED;
    struct OhciED	        *hc_OhciBulkTailED;
    struct OhciED	        *hc_OhciIntED[5];
    struct OhciED	        *hc_OhciTermED;
    struct OhciTD	        *hc_OhciTermTD;
    struct OhciHCCA	        *hc_OhciHCCA;
    struct OhciED	        *hc_OhciEDPool;
    struct OhciTD	        *hc_OhciTDPool;
    struct OhciED	        *hc_OhciAsyncFreeED;
    ULONG		            hc_OhciDoneQueue;
    struct List		        hc_OhciRetireQueue;

    ULONG		            hc_FrameCounter;
    struct List		        hc_TDQueue;
    struct List		        hc_AbortQueue;
    
    struct List		        hc_PeriodicTDQueue;
    struct List		        hc_CtrlXFerQueue;
    struct List		        hc_IntXFerQueue;
    struct List		        hc_IsoXFerQueue;
    struct List		        hc_BulkXFerQueue;
    struct MinList          hc_RTIsoHandlers;
    
    struct Interrupt        hc_CompleteInt;
    struct Interrupt	    hc_ResetInt;

    UBYTE		            hc_PortNum[MAX_ROOT_PORTS];	    /* Global Port number the local controller port corresponds with */

    UWORD		            hc_PortChangeMap[MAX_ROOT_PORTS];   /* Port Change Map */

    BOOL		            hc_complexrouting;
    ULONG		            hc_portroute;
};

/* hc_Flags */
#define HCB_ALLOCATED	                0	                    /* PCI board allocated		 */
#define HCF_ALLOCATED	                (1 << HCB_ALLOCATED)
#define HCB_ONLINE	                    1	                    /* Online			 */
#define HCF_ONLINE	                    (1 << HCB_ONLINE)
#define HCB_STOP_BULK	                2	                    /* Bulk transfers stopped	 */
#define HCF_STOP_BULK	                (1 << HCB_STOP_BULK)
#define HCB_STOP_CTRL	                3	                    /* Control transfers stopped	 */
#define HCF_STOP_CTRL	                (1 << HCB_STOP_CTRL)
#define HCB_ABORT	                    4	                    /* Aborted requests available	 */
#define HCF_ABORT	                    (1 << HCB_ABORT)

/* hc_Quirks */
#define HCQB_EHCI_OVERLAY_CTRL_FILL     0
#define HCQ_EHCI_OVERLAY_CTRL_FILL      (1 << HCQB_EHCI_OVERLAY_CTRL_FILL)
#define HCQB_EHCI_OVERLAY_INT_FILL      1
#define HCQ_EHCI_OVERLAY_INT_FILL       (1 << HCQB_EHCI_OVERLAY_INT_FILL)
#define HCQB_EHCI_OVERLAY_BULK_FILL     2
#define HCQ_EHCI_OVERLAY_BULK_FILL      (1 << HCQB_EHCI_OVERLAY_BULK_FILL)
#define HCQB_EHCI_VBOX_FRAMEROOLOVER    3
#define HCQ_EHCI_VBOX_FRAMEROOLOVER     (1 << HCQB_EHCI_VBOX_FRAMEROOLOVER)
#define HCQB_EHCI_MOSC_FRAMECOUNTBUG    4
#define HCQ_EHCI_MOSC_FRAMECOUNTBUG     (1 << HCQB_EHCI_MOSC_FRAMECOUNTBUG)

/*
 * The device node - private
 */
struct PCIDevice
{
    struct Library	        hd_Library;	                        /* standard */
    UWORD		            hd_Flags;	                        /* various flags */

    struct UtilityBase      *hd_UtilityBase;	                /* for tags etc */
#if defined(__OOP_NOLIBBASE__)
    struct Library          *hd_OOPBase;
#endif
    OOP_Object              *e1kb_PCI;
#if defined(__OOP_NOATTRBASES__)
    OOP_AttrBase	        hd_HiddAB;
    OOP_AttrBase	        hd_HiddPCIDeviceAB;
#endif
#if defined(__OOP_NOMETHODBASES__)
    OOP_MethodID            hd_HiddPCIMB;
    OOP_MethodID            hd_HiddPCIDeviceMB;
    OOP_MethodID            hd_HiddPCIDriverMB;
    OOP_MethodID            hd_HWBase;
#endif

    struct List		        hd_TempHCIList;

    OOP_Object	            *hd_PCIHidd;

    BOOL		            hd_ScanDone;	                    /* PCI scan done? */
    APTR		            hd_MemPool;	                        /* Memory Pool */

	struct List	            hd_Units;	                        /* List of units */
};

/* hd_Flags */
#define HDB_FORCEPOWER	                0
#define HDF_FORCEPOWER	                (1 << HDB_FORCEPOWER)
#if defined(TMPXHCICODE)
#define HDB_ENABLEXHCI	                1
#define HDF_ENABLEXHCI	                (1 << HDB_ENABLEXHCI)
#endif

/** OOP Related **/

#if defined(__OOP_NOLIBBASE__)
#undef OOPBase
#define OOPBase                 (base->hd_OOPBase)
#endif

#if defined(__OOP_NOATTRBASES__)
#undef HiddAttrBase
#define HiddAttrBase            (base->hd_HiddAB)
#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase   (base->hd_HiddPCIDeviceAB)
#endif

#if defined(__OOP_NOMETHODBASES__)
#undef HiddPCIBase
#define HiddPCIBase             (base->hd_HiddPCIMB)
#undef HiddPCIDeviceBase
#define HiddPCIDeviceBase       (base->hd_HiddPCIDeviceMB)
#undef HiddPCIDriverBase
#define HiddPCIDriverBase       (base->hd_HiddPCIDriverMB)
#undef HWBase
#define HWBase                  (base->hd_HWBase)
#endif

#if defined(USE_FAST_PCICFG)
static inline UBYTE READCONFIGBYTE(struct PCIController *hc, OOP_Object *o, ULONG reg)
{
    struct pHidd_PCIDevice_ReadConfigWord rcb_p;
#if defined(__OOP_NOLIBBASE__)
# ifdef base
#  undef base
# endif
# define base (hc->hc_Device)
#else
# if !defined(__OOP_NOMETHODBASES__)
#  define __obj o
# endif
#endif /* __OOP_NO_LIBBASE__ */
#if defined(__OOP_NOMETHODBASES__)
    rcb_p.mID   = hc->hc_Device->hd_HiddPCIDeviceMB + moHidd_PCIDevice_ReadConfigByte;
#else
    rcb_p.mID   = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
#endif
    rcb_p.reg   = reg;
    return (UWORD)hc->hc_ReadConfigByte(hc->hc_ReadConfigByte_Class, o, &rcb_p.mID);
#if defined(__OOP_NOLIBBASE__)
# undef base
#else
# if !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
}
static inline UWORD READCONFIGWORD(struct PCIController *hc, OOP_Object *o, ULONG reg)
{
    struct pHidd_PCIDevice_ReadConfigWord rcw_p;
#if defined(__OOP_NOLIBBASE__)
# ifdef base
#  undef base
# endif
# define base (hc->hc_Device)
#else
# if !defined(__OOP_NOMETHODBASES__)
#  define __obj o
# endif
#endif /* __OOP_NO_LIBBASE__ */
#if defined(__OOP_NOMETHODBASES__)
    rcw_p.mID   = hc->hc_Device->hd_HiddPCIDeviceMB + moHidd_PCIDevice_ReadConfigWord;
#else
    rcw_p.mID   = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord);
#endif
    rcw_p.reg   = reg;
    return (UWORD)hc->hc_ReadConfigWord(hc->hc_ReadConfigWord_Class, o, &rcw_p.mID);
#if defined(__OOP_NOLIBBASE__)
# undef base
#else
# if !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
}

static inline ULONG READCONFIGLONG(struct PCIController *hc, OOP_Object *o, ULONG reg)
{
    struct pHidd_PCIDevice_ReadConfigLong rcl_p;
#if defined(__OOP_NOLIBBASE__)
# ifdef base
#  undef base
# endif
# define base (hc->hc_Device)
#else
# if !defined(__OOP_NOMETHODBASES__)
#  define __obj o
# endif
#endif /* __OOP_NO_LIBBASE__ */
#if defined(__OOP_NOMETHODBASES__)
    rcl_p.mID   = hc->hc_Device->hd_HiddPCIDeviceMB + moHidd_PCIDevice_ReadConfigWord;
#else
    rcl_p.mID   = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigLong);
#endif
    rcl_p.reg   = reg;
    return (UWORD)hc->hc_ReadConfigLong(hc->hc_ReadConfigLong_Class, o, &rcl_p.mID);
#if defined(__OOP_NOLIBBASE__)
# undef base
#else
# if !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
}

static inline void WRITECONFIGBYTE(struct PCIController *hc, OOP_Object *o, ULONG reg, UBYTE value)
{
    struct pHidd_PCIDevice_WriteConfigByte wcb_p;
#if defined(__OOP_NOLIBBASE__)
# ifdef base
#  undef base
# endif
# define base (hc->hc_Device)
#else
# if !defined(__OOP_NOMETHODBASES__)
#  define __obj o
# endif
#endif /* __OOP_NO_LIBBASE__ */
#if defined(__OOP_NOMETHODBASES__)
    wcb_p.mID   = hc->hc_Device->hd_HiddPCIDeviceMB + moHidd_PCIDevice_WriteConfigByte;
#else
    wcb_p.mID   = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
#endif
    wcb_p.reg   = reg;
    wcb_p.val   = value;
    hc->hc_WriteConfigByte(hc->hc_WriteConfigByte_Class, o, &wcb_p.mID);
#if defined(__OOP_NOLIBBASE__)
# undef base
#else
# if !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
}

static inline void WRITECONFIGWORD(struct PCIController *hc, OOP_Object *o, ULONG reg, UWORD value)
{
    struct pHidd_PCIDevice_WriteConfigWord wcw_p;
#if defined(__OOP_NOLIBBASE__)
# ifdef base
#  undef base
# endif
# define base (hc->hc_Device)
#else
# if !defined(__OOP_NOMETHODBASES__)
#  define __obj o
# endif
#endif /* __OOP_NO_LIBBASE__ */
#if defined(__OOP_NOMETHODBASES__)
    wcw_p.mID   = hc->hc_Device->hd_HiddPCIDeviceMB + moHidd_PCIDevice_WriteConfigWord;
#else
    wcw_p.mID   = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord);
#endif
    wcw_p.reg   = reg;
    wcw_p.val   = value;
    hc->hc_WriteConfigWord(hc->hc_WriteConfigWord_Class, o, &wcw_p.mID);
#if defined(__OOP_NOLIBBASE__)
# undef base
#else
# if !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
}

static inline void WRITECONFIGLONG(struct PCIController *hc, OOP_Object *o, ULONG reg, ULONG value)
{
    struct pHidd_PCIDevice_WriteConfigLong wcl_p;
#if defined(__OOP_NOLIBBASE__)
# ifdef base
#  undef base
# endif
# define base (hc->hc_Device)
#else
# if !defined(__OOP_NOMETHODBASES__)
#  define __obj o
# endif
#endif /* __OOP_NO_LIBBASE__ */
#if defined(__OOP_NOMETHODBASES__)
    wcl_p.mID   = hc->hc_Device->hd_HiddPCIDeviceMB + moHidd_PCIDevice_WriteConfigLong;
#else
    wcl_p.mID   = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigLong);
#endif
    wcl_p.reg   = reg;
    wcl_p.val   = value;
    hc->hc_WriteConfigLong(hc->hc_WriteConfigLong_Class, o, &wcl_p.mID);
#if defined(__OOP_NOLIBBASE__)
# undef base
#else
# if !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
}
#else
#define READCONFIGBYTE(hc, obj, reg) HIDD_PCIDevice_ReadConfigByte(obj, reg)
#define READCONFIGWORD(hc, obj, reg) HIDD_PCIDevice_ReadConfigWord(obj, reg)
#define READCONFIGLONG(hc, obj, reg) HIDD_PCIDevice_ReadConfigLong(obj, reg)
#define WRITECONFIGBYTE(hc, obj, reg, val) HIDD_PCIDevice_WriteConfigByte(obj, reg, val)
#define WRITECONFIGWORD(hc, obj, reg, val) HIDD_PCIDevice_WriteConfigWord(obj, reg, val)
#define WRITECONFIGLONG(hc, obj, reg, val) HIDD_PCIDevice_WriteConfigLong(obj, reg, val)
#endif

#endif /* PCIUSB_H */
