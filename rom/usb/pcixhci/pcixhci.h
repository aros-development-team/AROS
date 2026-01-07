#ifndef PCIXHCI_H
#define PCIXHCI_H

/*
 *----------------------------------------------------------------------------
 *			   Includes for pcixhci.device
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

#if defined(__OOP_NOLIBBASE__) && defined(__OOP_NOMETHODBASES__)
#define USE_FAST_PCICFG
#endif

/* Reply the iorequest with success */
#define RC_OK                           0

/* Magic cookie, don't set error fields & don't reply the ioreq */
#define RC_DONTREPLY                    -1

#define MAX_ROOT_PORTS                  16
#define MAX_ENDPOINTS                   MAX_ROOT_PORTS
#define MAX_DEVENDPOINTS                (MAX_ROOT_PORTS << 1)

#define PCI_CLASS_SERIAL_USB            0x0c03

#define USB_DEV_MAX                     128
#define USB_DEVEP_CNT                   (USB_DEV_MAX * MAX_DEVENDPOINTS)

struct RTIsoNode;

struct PTDNode {
    struct MinNode              ptd_Node;
    APTR                        ptd_Descriptor;
    ULONG                       ptd_Phys;
    ULONG                       ptd_NextPhys;
    ULONG                       ptd_FrameIdx;
    UWORD                       ptd_Length;
    UWORD                       ptd_PktCount;
    UWORD                       ptd_PktLength[8];
    UWORD                       ptd_Flags;
    struct PTDNode             *ptd_NextPTD;
    struct IOUsbHWReq           ptd_IOReq;
    struct IOUsbHWBufferReq     ptd_BufferReq;
    APTR                        ptd_BounceBuffer;
    struct RTIsoNode           *ptd_RTIsoNode;
};

#define PTDF_ACTIVE             (1<<0)
#define PTDF_BUFFER_VALID       (1<<1)
#define PTDF_SITD               (1<<2)
#define PTDF_QUEUED             (1<<3)

#define PCIUSB_ISO_PTD_COUNT    8

struct RTIsoNode {
    struct MinNode              rtn_Node;
    struct IOUsbHWRTIso         *rtn_RTIso;
    struct IOUsbHWReq           *rtn_StdReq;
    ULONG                       rtn_NextPTD;
    ULONG                       rtn_NextFrame;
    UWORD                       rtn_PTDCount;
    struct PTDNode              **rtn_PTDs;
    struct IOUsbHWBufferReq     rtn_BufferReq;
    APTR                        rtn_BounceBuffer;
    struct IOUsbHWReq           rtn_IOReq;
    UWORD                       rtn_Flags;
};

#define RTISO_FLAG_EXPLICIT_FRAME (1 << 0)

static inline struct IOUsbHWReq *pciusbIsoGetIOReq(struct RTIsoNode *rtn)
{
    return rtn->rtn_StdReq ? rtn->rtn_StdReq : &rtn->rtn_IOReq;
}

/* The unit node - private */
struct PCIUnit {
    struct Unit                 hu_Unit;
    LONG                        hu_UnitNo;
#define PCIUSBUNIT_MASK     (1 << 31)                           /* Indicates if the unit is opened/allocated                    */

    struct PCIDevice            *hu_Device;                         /* Uplink                                                       */

    struct MsgPort              *hu_MsgPort;
    struct timerequest          *hu_TimerReq;                       /* Timer I/O Request                                            */

    struct timerequest          hu_LateIOReq;                       /* Timer I/O Request                                            */
    struct MsgPort              hu_LateMsgPort;

    struct timerequest          hu_NakTimeoutReq;
    struct MsgPort              hu_NakTimeoutMsgPort;
    struct Interrupt            hu_NakTimeoutInt;

    ULONG                       hu_DevID;                           /* Device ID (BusID+DevNo)                                      */
    ULONG                       hu_FrameCounter;                    /* Common frame counter                                         */
    struct List                 hu_Controllers;                     /* List of controllers                                          */

    UWORD                       hu_RootHubPorts;
    UWORD                       hu_RootHubAddr;                     /* Root Hub Address                                             */
    UWORD                       hu_RootPortChanges;                 /* Merged root hub changes                                      */
    struct List                 hu_RHIOQueue;                       /* Root Hub Pending IO Requests                                 */

    UBYTE                       hu_ProductName[80];                 /* for Query device                                             */
    struct PCIController        *hu_DevControllers[USB_DEV_MAX];    /* maps from Device address to controller                       */

    struct RTIsoNode            hu_RTIsoNodes[MAX_ROOT_PORTS];
    struct MinList              hu_FreeRTIsoNodes;

    UWORD                       hu_RootHubXPorts;

    struct PCIController        *hu_PortMapX[MAX_ROOT_PORTS];       /* Maps from Global Port to XHCI controller                     */
#if (1)
    //TODO: This needs adjusted to handle >16 EP's in XHCI
    struct IOUsbHWReq *volatile hu_DevBusyReq[USB_DEVEP_CNT];       /* pointer to io assigned to the Endpoint                       */
    volatile ULONG              hu_NakTimeoutFrame[USB_DEVEP_CNT];  /* Nak Timeout framenumber                                      */
    UBYTE                       hu_DevDataToggle[USB_DEVEP_CNT];    /* Data toggle bit for endpoints                                */
#else
    //TODO: This needs adjusted to handle >16 EP's in XHCI
    struct IOUsbHWReq           **hu_DevBusyReq;                    /* pointer to io assigned to the Endpoint                       */
    ULONG                       *hu_NakTimeoutFrame;                /* Nak Timeout framenumber                                      */
    UBYTE                       *hu_DevDataToggle;                  /* Data toggle bit for endpoints                                */
#endif
};

struct XHCIController {
    ULONG                       xc_Reserved;
};

/* HCITYPE_xxx, is the pci device interface */
#define HCITYPE_XHCI                    0x30

struct PCIController {
    struct Node                 hc_Node;
    struct PCIDevice            *hc_Device;                         /* Uplink */
    struct PCIUnit              *hc_Unit;                           /* Uplink */
#if defined(AROS_USE_LOGRES)
    APTR                        hc_LogRHandle;
#endif

    OOP_Object                  *hc_PCIDeviceObject;
    OOP_Object                  *hc_PCIDriverObject;

    UWORD                       hc_VendID;
    UWORD                       hc_ProdID;

#if defined(USE_FAST_PCICFG)
    OOP_MethodFunc              hc_ReadConfigByte;
    OOP_Class                   *hc_ReadConfigByte_Class;
    OOP_MethodFunc              hc_ReadConfigWord;
    OOP_Class                   *hc_ReadConfigWord_Class;
    OOP_MethodFunc              hc_ReadConfigLong;
    OOP_Class                   *hc_ReadConfigLong_Class;
    OOP_MethodFunc              hc_WriteConfigByte;
    OOP_Class                   *hc_WriteConfigByte_Class;
    OOP_MethodFunc              hc_WriteConfigWord;
    OOP_Class                   *hc_WriteConfigWord_Class;
    OOP_MethodFunc              hc_WriteConfigLong;
    OOP_Class                   *hc_WriteConfigLong_Class;
    OOP_MethodFunc              hc_AllocPCIMem;
    OOP_Class                   *hc_AllocPCIMem_Class;
    OOP_MethodFunc              hc_FreePCIMem;
    OOP_Class                   *hc_FreePCIMem_Class;
    OOP_MethodFunc              hc_MapPCI;
    OOP_Class                   *hc_MapPCI_Class;
    OOP_MethodFunc              hc_CPUtoPCI;
    OOP_Class                   *hc_CPUtoPCI_Class;
    OOP_MethodFunc              hc_PCItoCPU;
    OOP_Class                   *hc_PCItoCPU_Class;
#endif

    ULONG                       hc_DevID;
    UWORD                       hc_FunctionNum;
    UWORD                       hc_NumPorts;
    UWORD                       hc_Flags;                           /* See below */
    ULONG                       hc_Quirks;                          /* See below */

    UWORD                       hc_IsoPTDCount;

    UBYTE                       hc_HCIVersionMajor;
    UBYTE                       hc_HCIVersionMinor;
    UBYTE                       hc_USBVersionMajor;
    UBYTE                       hc_USBVersionMinor;

    volatile APTR               hc_RegBase;

    struct MemEntry             hc_PCIMem;
    BOOL                        hc_PCIMemIsExec;
    IPTR                        hc_PCIVirtualAdjust;
    IPTR                        hc_PCIIntLine;
    struct Interrupt            hc_PCIIntHandler;
    ULONG                       hc_PCIIntEnMask;

    ULONG                       hc_FrameCounter;
    struct List                 hc_TDQueue;
    struct List                 hc_AbortQueue;

    struct List                 hc_PeriodicTDQueue;
    struct List                 hc_CtrlXFerQueue;
    struct List                 hc_IntXFerQueue;
    struct List                 hc_IsoXFerQueue;
    struct List                 hc_BulkXFerQueue;
    struct MinList              hc_RTIsoHandlers;

    struct Interrupt            hc_CompleteInt;
    struct Interrupt            hc_ResetInt;

    UBYTE                       hc_PortNum[MAX_ROOT_PORTS];	        /* Global Port number the local controller port corresponds with    */

    UWORD                       hc_PortChangeMap[MAX_ROOT_PORTS];   /* Port Change Map                                                  */
    UWORD                       hc_PortChangeMask;                  /* mask of enabled change notifications                             */
    ULONG                       hc_portroute;

    APTR                        hc_CPrivate;
    BOOL                        hc_complexrouting;
};

/* hc_Flags */
#define HCB_ALLOCATED	                0	                    /* PCI board allocated		    */
#define HCF_ALLOCATED	                (1 << HCB_ALLOCATED)
#define HCB_ONLINE	                    1	                    /* Online			            */
#define HCF_ONLINE	                    (1 << HCB_ONLINE)
#define HCB_STOP_BULK	                2	                    /* Bulk transfers stopped	    */
#define HCF_STOP_BULK	                (1 << HCB_STOP_BULK)
#define HCB_STOP_CTRL	                3	                    /* Control transfers stopped	*/
#define HCF_STOP_CTRL	                (1 << HCB_STOP_CTRL)
#define HCB_ABORT	                    4	                    /* Aborted requests available	*/
#define HCF_ABORT	                    (1 << HCB_ABORT)
#define HCB_MSI	                        12	                    /* MSI interrupt in use         */
#define HCF_MSI	                        (1 << HCB_MSI)
#define HCB_PPC	                        13	                    /* Per-Port Power	            */
#define HCF_PPC	                        (1 << HCB_PPC)
#define HCB_ADDR64	                    14	                    /* 64Bit addressing	            */
#define HCF_ADDR64	                    (1 << HCB_ADDR64)
#define HCB_CTX64	                    15	                    /* 64Byte context               */
#define HCF_CTX64	                    (1 << HCB_CTX64)

/*
 * The device node - private
 */
struct PCIDevice {
    struct Device               hd_Device;                          /* standard */
    UWORD                       hd_Flags;                           /* various flags */
#if defined(AROS_USE_LOGRES)
    APTR                        hd_LogResBase;
    APTR                        hd_LogRHandle;
#endif
    struct UtilityBase          *hd_UtilityBase;                    /* for tags etc */
#if defined(__OOP_NOLIBBASE__)
    struct Library              *hd_OOPBase;
#endif
    OOP_Class                   *hd_USBXHCIControllerClass;
#if defined(__OOP_NOATTRBASES__)
    OOP_AttrBase                hd_HiddAB;
    OOP_AttrBase                hd_HiddPCIDeviceAB;
#endif
#if defined(__OOP_NOMETHODBASES__)
    OOP_MethodID                hd_HiddPCIMB;
    OOP_MethodID                hd_HiddPCIDeviceMB;
    OOP_MethodID                hd_HiddPCIDriverMB;
    OOP_MethodID                hd_HWBase;
#endif

    struct List                 hd_TempHCIList;

    OOP_Object                  *hd_PCIHidd;

    BOOL                        hd_ScanDone;                        /* PCI scan done? */
    APTR                        hd_MemPool;                         /* Memory Pool */

    struct List                 hd_Units;                           /* List of units */
};

/** OOP Related **/

#if defined(__OOP_NOLIBBASE__)
#undef OOPBase
#define OOPBase                         (base->hd_OOPBase)
#endif

#if defined(__OOP_NOATTRBASES__)
#undef HiddAttrBase
#define HiddAttrBase                    (base->hd_HiddAB)
#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase           (base->hd_HiddPCIDeviceAB)
#endif

#if defined(__OOP_NOMETHODBASES__)
#undef HiddPCIBase
#define HiddPCIBase                     (base->hd_HiddPCIMB)
#undef HiddPCIDeviceBase
#define HiddPCIDeviceBase               (base->hd_HiddPCIDeviceMB)
#undef HiddPCIDriverBase
#define HiddPCIDriverBase               (base->hd_HiddPCIDriverMB)
#undef HWBase
#define HWBase                          (base->hd_HWBase)
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
    rcb_p.mID           = hc->hc_Device->hd_HiddPCIDeviceMB + moHidd_PCIDevice_ReadConfigByte;
#else
    rcb_p.mID           = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
#endif
    rcb_p.reg           = reg;
    return (UBYTE)hc->hc_ReadConfigByte(hc->hc_ReadConfigByte_Class, o, &rcb_p.mID);
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
    rcw_p.mID           = hc->hc_Device->hd_HiddPCIDeviceMB + moHidd_PCIDevice_ReadConfigWord;
#else
    rcw_p.mID           = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord);
#endif
    rcw_p.reg           = reg;
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
    rcl_p.mID           = hc->hc_Device->hd_HiddPCIDeviceMB + moHidd_PCIDevice_ReadConfigWord;
#else
    rcl_p.mID           = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigLong);
#endif
    rcl_p.reg           = reg;
    return (ULONG)hc->hc_ReadConfigLong(hc->hc_ReadConfigLong_Class, o, &rcl_p.mID);
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
    wcb_p.mID           = hc->hc_Device->hd_HiddPCIDeviceMB + moHidd_PCIDevice_WriteConfigByte;
#else
    wcb_p.mID           = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
#endif
    wcb_p.reg           = reg;
    wcb_p.val           = value;
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
    wcw_p.mID           = hc->hc_Device->hd_HiddPCIDeviceMB + moHidd_PCIDevice_WriteConfigWord;
#else
    wcw_p.mID           = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord);
#endif
    wcw_p.reg           = reg;
    wcw_p.val           = value;
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
    wcl_p.mID           = hc->hc_Device->hd_HiddPCIDeviceMB + moHidd_PCIDevice_WriteConfigLong;
#else
    wcl_p.mID           = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigLong);
#endif
    wcl_p.reg           = reg;
    wcl_p.val           = value;
    hc->hc_WriteConfigLong(hc->hc_WriteConfigLong_Class, o, &wcl_p.mID);
#if defined(__OOP_NOLIBBASE__)
# undef base
#else
# if !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
}

static inline APTR ALLOCPCIMEM(struct PCIController *hc, OOP_Object *o, ULONG Size)
{
    struct pHidd_PCIDriver_AllocPCIMem apm_p;
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
    apm_p.mID           = hc->hc_Device->hd_HiddPCIDriverMB + moHidd_PCIDriver_AllocPCIMem;
#else
    apm_p.mID           = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_AllocPCIMem);
#endif
    apm_p.Size          = Size;
    return (APTR)hc->hc_AllocPCIMem(hc->hc_AllocPCIMem_Class, o, &apm_p.mID);
#if defined(__OOP_NOLIBBASE__)
# undef base
#else
# if !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
}

static inline APTR FREEPCIMEM(struct PCIController *hc, OOP_Object *o, APTR Address)
{
    struct pHidd_PCIDriver_FreePCIMem fpm_p;
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
    fpm_p.mID           = hc->hc_Device->hd_HiddPCIDriverMB + moHidd_PCIDriver_FreePCIMem;
#else
    fpm_p.mID           = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_FreePCIMem);
#endif
    fpm_p.Address       = Address;
    return (APTR)hc->hc_FreePCIMem(hc->hc_FreePCIMem_Class, o, &fpm_p.mID);
#if defined(__OOP_NOLIBBASE__)
# undef base
#else
# if !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
}

static inline APTR MAPPCI(struct PCIController *hc, OOP_Object *o, APTR PCIAddress, ULONG Length)
{
    struct pHidd_PCIDriver_MapPCI mpci_p;
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
    mpci_p.mID          = hc->hc_Device->hd_HiddPCIDriverMB + moHidd_PCIDriver_MapPCI;
#else
    mpci_p.mID          = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI);
#endif
    mpci_p.PCIAddress   = PCIAddress;
    mpci_p.Length       = Length;
    return (APTR)hc->hc_MapPCI(hc->hc_MapPCI_Class, o, &mpci_p.mID);
#if defined(__OOP_NOLIBBASE__)
# undef base
#else
# if !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
}

static inline APTR CPUTOPCI(struct PCIController *hc, OOP_Object *o, APTR address)
{
    struct pHidd_PCIDriver_CPUtoPCI cputopci_p;
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
    cputopci_p.mID          = hc->hc_Device->hd_HiddPCIDriverMB + moHidd_PCIDriver_CPUtoPCI;
#else
    cputopci_p.mID          = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_CPUtoPCI);
#endif
    cputopci_p.address   = address;
    return (APTR)hc->hc_CPUtoPCI(hc->hc_CPUtoPCI_Class, o, &cputopci_p.mID);
#if defined(__OOP_NOLIBBASE__)
# undef base
#else
# if !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
}

static inline APTR PCITOCPU(struct PCIController *hc, OOP_Object *o, APTR address)
{
    struct pHidd_PCIDriver_PCItoCPU pcitocpu_p;
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
    pcitocpu_p.mID          = hc->hc_Device->hd_HiddPCIDriverMB + moHidd_PCIDriver_PCItoCPU;
#else
    pcitocpu_p.mID          = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_PCItoCPU);
#endif
    pcitocpu_p.address   = address;
    return (APTR)hc->hc_PCItoCPU(hc->hc_PCItoCPU_Class, o, &pcitocpu_p.mID);
#if defined(__OOP_NOLIBBASE__)
# undef base
#else
# if !defined(__OOP_NOMETHODBASES__)
#  undef __obj
# endif
#endif
}
#else
#define READCONFIGBYTE(hc, obj, reg)    HIDD_PCIDevice_ReadConfigByte(obj, reg)
#define READCONFIGWORD(hc, obj, reg)    HIDD_PCIDevice_ReadConfigWord(obj, reg)
#define READCONFIGLONG(hc, obj, reg)    HIDD_PCIDevice_ReadConfigLong(obj, reg)
#define WRITECONFIGBYTE(hc, obj, reg, val) HIDD_PCIDevice_WriteConfigByte(obj, reg, val)
#define WRITECONFIGWORD(hc, obj, reg, val) HIDD_PCIDevice_WriteConfigWord(obj, reg, val)
#define WRITECONFIGLONG(hc, obj, reg, val) HIDD_PCIDevice_WriteConfigLong(obj, reg, val)
#endif /* USE_FAST_PCICFG */
#if !defined(USE_FAST_PCICFG)
#define ALLOCPCIMEM(hc, obj, size)      HIDD_PCIDriver_AllocPCIMem(obj, size)
#define FREEPCIMEM(hc, obj, address)    HIDD_PCIDriver_FreePCIMem(obj, address)
#define MAPPCI(hc, obj, pciaddress, length) HIDD_PCIDriver_MapPCI(obj, pciaddress, length)
#define CPUTOPCI(hc, obj, pciaddress) HIDD_PCIDriver_CPUtoPCI(obj, pciaddress)
#define PCITOCPU(hc, obj, pciaddress) HIDD_PCIDriver_PCItoCPU(obj, pciaddress)
#endif
#endif /* PCIXHCI_H */
