#ifndef DEVICES_USBHARDWARE_H
#define DEVICES_USBHARDWARE_H
/*
**	$VER: usbhardware.h 3.3 (06.01.2026)
**
**	standard usb hardware device include file
**
**	(C) Copyright 2002-2007 Chris Hodges
**	(C) Copyright 2007-2026 AROS Development Team
**	    All Rights Reserved
*/

#ifndef EXEC_IO_H
#include "exec/io.h"
#endif

#ifndef EXEC_ERRORS_H
#include <exec/errors.h>
#endif

#ifndef DEVICES_USB_H
#include "devices/usb.h"
#endif

/* Common base (V1) fields */
#define IOUSBHWREQ_V1_FIELDS                                                                                                                          \
    struct IORequest    iouh_Req;               /* basic IOReq                                                                                      */\
    UWORD               iouh_Flags;             /* Transfer flags                                                                                   */\
    UWORD               iouh_State;             /* USB State Flags                                                                                  */\
    UWORD               iouh_Dir;               /* Direction of transfer                                                                            */\
    UWORD               iouh_DevAddr;           /* USB Device Address (0-127)                                                                       */\
    UWORD               iouh_Endpoint;          /* USB Device Endpoint (0-15)                                                                       */\
    UWORD               iouh_MaxPktSize;        /* Maximum packet size                                                                              */\
    ULONG               iouh_Actual;            /* Actual bytes transferred                                                                         */\
    ULONG               iouh_Length;            /* Size of buffer                                                                                   */\
    APTR                iouh_Data;              /* Pointer to in/out buffer                                                                         */\
    UWORD               iouh_Interval;          /* Interrupt Interval (ms or 125us units)                                                           */\
    ULONG               iouh_NakTimeout;        /* Timeout in ms before request retired                                                             */\
    struct UsbSetupData iouh_SetupData;         /* Setup fields for ctrl transfers                                                                  */\
    APTR                iouh_UserData;          /* private data, stack-owned                                                                        */\
    UWORD               iouh_ExtError           /* Extended error code                                                                              */

/* V2 extension fields (added after V1) */
#define IOUSBHWREQ_V2_FIELDS                                                                                                                          \
    UWORD               iouh_Frame;             /* current USB-Frame / ISO start frame                                                              */\
    UWORD               iouh_SplitHubAddr;      /* Split-Transaction HUB address                                                                    */\
    UWORD               iouh_SplitHubPort;      /* Split-Transaction HUB downstream port                                                            */\
    APTR                iouh_DriverPrivate1;    /* private data for internal driver use                                                             */\
    APTR                iouh_DriverPrivate2     /* private data for internal driver use                                                             */

/* V3 extension fields (for USB3/xHCI etc.) */
#define IOUSBHWREQ_V3_FIELDS                                                                                                                          \
    UWORD               iouh_RootPort;           /* root-hub port (1-based)                                                                          */\
    /* SuperSpeed endpoint companion info */                                                                                                          \
    UBYTE               iouh_SS_MaxBurst;       /* bMaxBurst                                                                                        */\
    UBYTE               iouh_SS_Mult;           /* Mult for isoch                                                                                   */\
    UWORD               iouh_SS_BytesPerInterval; /* wBytesPerInterval                                                                              */\
    /* Topology */                                                                                                                                    \
    ULONG               iouh_RouteString;       /* 20-bit USB3 route string                                                                         */\
    /* Optional fields */                                                                                                                             \
    UWORD               iouh_StreamID;          /* per-transfer (usually)                                                                           */\
    UWORD               iouh_PowerPolicy        /* link power / policy hints                                                                        */

/* IO Request structure */
/* Original V1 layout (kept for ABI/compat) */
struct IOUsbHWReqV1
{
    IOUSBHWREQ_V1_FIELDS;
};
#define IOUsbHWReqObsolete  IOUsbHWReqV1

/* V2 = V1 + V2 extension */
struct IOUsbHWReqV2
{
    IOUSBHWREQ_V1_FIELDS;
    IOUSBHWREQ_V2_FIELDS;
};

/* Current version = V1 + V2 + V3 */
struct IOUsbHWReq
{
    IOUSBHWREQ_V1_FIELDS;
    IOUSBHWREQ_V2_FIELDS;
    IOUSBHWREQ_V3_FIELDS;
};

typedef LONG (*PsdPrepareEndpointFunc)(struct IOUsbHWReq *ioreq);
typedef void (*PsdDestroyEndpointFunc)(struct IOUsbHWReq *ioreq);

/* Realtime ISO transfer structure as given in iouh_Data */
struct IOUsbHWRTIso
{
    struct Node         *urti_Node;             /* Driver's linkage (private)                                                                       */
    struct Hook         *urti_InReqHook;        /* Called with struct IOUsbHWBufferReq whenever input data has arrived and is ready to be copied    */
    struct Hook         *urti_OutReqHook;       /* Called with struct IOUsbHWBufferReq to prepare output buffer copying                             */
    struct Hook         *urti_InDoneHook;       /* Called with struct IOUsbHWBufferReq when input buffer has been copied                            */
    struct Hook         *urti_OutDoneHook;      /* Called with struct IOUsbHWBufferReq when output buffer has been sent                             */
    ULONG               urti_OutPrefetch;       /* Maximum prefetch in bytes allowed for output                                                     */
    APTR                urti_DriverPrivate1;    /* private data for internal driver use                                                             */
    APTR                urti_DriverPrivate2;    /* private data for internal driver use                                                             */
};

struct IOUsbHWBufferReq
{
    UBYTE               *ubr_Buffer;            /* Pointer to buffer, filled by called function                                                     */
    ULONG               ubr_Length;             /* Length of input received or output to be sent (may be adjusted by hook to force a partial copy)  */
    UWORD               ubr_Frame;              /* Frame number, filled by caller (may be adjusted by output hook)                                  */
    UWORD               ubr_Flags;              /* Flags, may be inspected and changed by hooks                                                     */
};

/* Definitions for ubr_Flags */
#define UBFB_CONTBUFFER         0               /* Set by InReqHook or OutReqHook to indicate that more buffer needs to be copied (scatter/gather)  */
#define UBFF_CONTBUFFER         (1 << UBFB_CONTBUFFER)

/* non-standard commands */
#define UHCMD_QUERYDEVICE       (CMD_NONSTD + 0)
#define UHCMD_USBRESET          (CMD_NONSTD + 1)
#define UHCMD_USBRESUME         (CMD_NONSTD + 2)
#define UHCMD_USBSUSPEND        CMD_STOP
#define UHCMD_USBOPER           CMD_START
#define UHCMD_CONTROLXFER       (CMD_NONSTD + 3)
#define UHCMD_ISOXFER           (CMD_NONSTD + 4)
#define UHCMD_INTXFER           (CMD_NONSTD + 5)
#define UHCMD_BULKXFER          (CMD_NONSTD + 6)
#define UHCMD_ADDISOHANDLER     (CMD_NONSTD + 7)
#define UHCMD_REMISOHANDLER     (CMD_NONSTD + 8)
#define UHCMD_STARTRTISO        (CMD_NONSTD + 9)
#define UHCMD_STOPRTISO         (CMD_NONSTD + 10)

/* Error codes for io_Error field */
#define UHIOERR_NO_ERROR        0               /* No error occured                                                                                 */
#define UHIOERR_USBOFFLINE      1               /* USB non-operational                                                                              */
#define UHIOERR_NAK             2               /* NAK received                                                                                     */
#define UHIOERR_HOSTERROR       3               /* Unspecific host error                                                                            */
#define UHIOERR_STALL           4               /* Endpoint stalled                                                                                 */
#define UHIOERR_PKTTOOLARGE     5               /* Packet is too large to be transferred                                                            */
#define UHIOERR_TIMEOUT         6               /* No acknoledge on packet                                                                          */
#define UHIOERR_OVERFLOW        7               /* More data received than expected (babble condition)                                              */
#define UHIOERR_CRCERROR        8               /* Incoming Packet corrupted                                                                        */
#define UHIOERR_RUNTPACKET      9               /* Less data received than requested                                                                */
#define UHIOERR_NAKTIMEOUT      10              /* Timeout due to NAKs                                                                              */
#define UHIOERR_BADPARAMS       11              /* Illegal parameters in request                                                                    */
#define UHIOERR_OUTOFMEMORY     12              /* Out of auxiliary memory for the driver                                                           */
#define UHIOERR_BABBLE          13              /* Babble condition                                                                                 */

/* Values for iouh_Dir */
#define UHDIR_SETUP             0               /* This is a setup transfer (UHCMD_CTRLXFER)                                                        */
#define UHDIR_OUT               1               /* This is a host to device transfer                                                                */
#define UHDIR_IN                2               /* This is a device to host transfer                                                                */

/* Definitions for iouh_Flags */
#define UHFB_LOWSPEED           0               /* Device operates at low speed                                                                     */
#define UHFB_HIGHSPEED          1               /* Device operates at high speed (USB 2.0)                                                          */
#define UHFB_NOSHORTPKT         2               /* Inhibit sending of a short packet at the end of a transfer (if possible)                         */
#define UHFB_NAKTIMEOUT         3               /* Allow the request to time-out after the given timeout value                                      */
#define UHFB_ALLOWRUNTPKTS      4               /* Receiving less data than expected will not cause an UHIOERR_RUNTPACKET                           */
#define UHFB_SPLITTRANS         5               /* new for V2.0: Split transaction for Lowspeed/Fullspeed devices at USB2.0 hubs                    */
#define UHFB_MULTI_1            6               /* new for V2.1: Number of transactions per microframe bit 0                                        */
#define UHFB_MULTI_2            7               /* new for V2.1: Number of transactions per microframe bit 1                                        */
#define UHFS_THINKTIME          8               /* new for V2.2: Bit times required at most for intertransaction gap on LS/FS                       */
#define UHFB_HUB                10              /* new for v3.0: device is a hub                                                                    */
#define UHFB_TT_MULTI           11              /* new for v3.0: hub is multi-TT                                                                    */
#define UHFB_SUPERSPEED         15              /* new for v3.0: Device operates at super speed (USB 3.0)                                           */

#define UHTT_8                  (0)
#define UHTT_16                 (1)
#define UHTT_24                 (2)
#define UHTT_32                 (3)

#define UHFF_LOWSPEED           (1 << UHFB_LOWSPEED)
#define UHFF_HIGHSPEED          (1 << UHFB_HIGHSPEED)
#define UHFF_NOSHORTPKT         (1 << UHFB_NOSHORTPKT)
#define UHFF_NAKTIMEOUT         (1 << UHFB_NAKTIMEOUT)
#define UHFF_ALLOWRUNTPKTS      (1 << UHFB_ALLOWRUNTPKTS)
#define UHFF_SPLITTRANS         (1 << UHFB_SPLITTRANS)
#define UHFF_MULTI_1            (1 << UHFB_MULTI_1)
#define UHFF_MULTI_2            (1 << UHFB_MULTI_2)
#define UHFF_MULTI_3            (UHFF_MULTI_1|UHFF_MULTI_2)
#define UHFF_THINKTIME_8        (UHTT_8  << UHFS_THINKTIME)
#define UHFF_THINKTIME_16       (UHTT_16 << UHFS_THINKTIME)
#define UHFF_THINKTIME_24       (UHTT_24 << UHFS_THINKTIME)
#define UHFF_THINKTIME_32       (UHTT_32 << UHFS_THINKTIME)
#define UHFF_HUB                (1 << UHFB_HUB)
#define UHFF_TT_MULTI           (1 << UHFB_TT_MULTI)
#define UHFF_SUPERSPEED         (1 << UHFB_SUPERSPEED)

/* Tags for UHCMD_QUERYDEVICE */

#define UHA_Dummy               (TAG_USER  + 0x4711)
#define UHA_State               (UHA_Dummy + 0x01)
#define UHA_Manufacturer        (UHA_Dummy + 0x10)
#define UHA_ProductName         (UHA_Dummy + 0x11)
#define UHA_Version             (UHA_Dummy + 0x12)
#define UHA_Revision            (UHA_Dummy + 0x13)
#define UHA_Description         (UHA_Dummy + 0x14)
#define UHA_Copyright           (UHA_Dummy + 0x15)
#define UHA_DriverVersion       (UHA_Dummy + 0x20)
#define UHA_Capabilities        (UHA_Dummy + 0x21)
#define UHA_PrepareEndpoint     (UHA_Dummy + 0x22)
#define UHA_DestroyEndpoint     (UHA_Dummy + 0x23)

/*
 *  Capabilities as returned by UHA_Capabities
 *  Only the main USB generations are defined.
 */
#define UHCB_USB20              0               /* Host controller supports USB 2.0 Highspeed                                                       */
#define UHCB_ISO                1               /* Host controller driver supports ISO transfers (UHCMD_ISOXFER)                                    */
#define UHCB_RT_ISO             2               /* Host controller driver supports real time ISO transfers (UHCMD_ADDISOHANDLER)                    */
#define UHCB_QUICKIO            3               /* BeginIO()/AbortIO() may be called from interrupts for less overhead                              */
#define UHCB_USB2OTG            4               /* Host controller supports USB2OTG device mode                                                     */

#define UHCB_USB30              31              /* Host controller supports USB 3.x SuperSpeed/+                                                    */

#define UHCF_USB20              (1 << UHCB_USB20)
#define UHCF_ISO                (1 << UHCB_ISO)
#define UHCF_RT_ISO             (1 << UHCB_RT_ISO)
#define UHCF_QUICKIO            (1 << UHCB_QUICKIO)
#define UHCF_USB2OTG            (1 << UHCB_USB2OTG)
#define UHCF_USB30              (1 << UHCB_USB30)

/* Definitions for UHA_State/iouh_State */

#define UHSB_OPERATIONAL        0               /* USB can be used for transfers                                                                    */
#define UHSB_RESUMING           1               /* USB is currently resuming                                                                        */
#define UHSB_SUSPENDED          2               /* USB is in suspended state                                                                        */
#define UHSB_RESET              3               /* USB is just inside a reset phase                                                                 */

#define UHSF_OPERATIONAL        (1 << UHSB_OPERATIONAL)
#define UHSF_RESUMING           (1 << UHSB_RESUMING)
#define UHSF_SUSPENDED          (1 << UHSB_SUSPENDED)
#define UHSF_RESET              (1 << UHSB_RESET)

/* -----------------------------------------------------------------------
 * iouh_PowerPolicy (UWORD)
 *
 * Per-transfer / per-endpoint link power management hints.
 *
 * Lower 4 bits: which low-power link states are ALLOWED for this transfer.
 * Next  3 bits: policy preference (power-save vs performance).
 * Upper bits :  misc flags / future extensions.
 * -------------------------------------------------------------------- */

/* Allowed states / capabilities (bitwise OR) */
#define USBPWR_ALLOW_L1         0x0001          /* Allow USB 2.0 LPM (L1) while active                                                              */
#define USBPWR_ALLOW_U1         0x0002          /* Allow USB 3.x U1 while active                                                                    */
#define USBPWR_ALLOW_U2         0x0004          /* Allow USB 3.x U2 while active                                                                    */
#define USBPWR_ALLOW_U3         0x0008          /* Allow USB 3.x U3 (device suspend)                                                                */

#define USBPWR_ALLOW_MASK       0x000F

/* Policy preference (mutually exclusive "mode") */
#define USBPWR_POLICY_MASK      0x0070
#define USBPWR_POLICY_DEFAULT   0x0000          /* Use controller's / OS global default                                                             */
#define USBPWR_POLICY_PERF      0x0010          /* Prefer performance (less power save)                                                             */
#define USBPWR_POLICY_BALANCED  0x0020          /* Balanced power/performance (default)                                                             */
#define USBPWR_POLICY_POWERSAVE 0x0030          /* Prefer power saving (more L1/U1/U2)                                                              */

/* Behaviour flags */
#define USBPWR_FLAG_NO_REMOTE_WAKE 0x0100       /* Don't arm remote wake/signal resume                                                              */
#define USBPWR_FLAG_NO_AUTOSUSPEND 0x0200       /* Don't autosuspend this endpoint                                                                  */

/* Predefined "profiles" (for convenience) */
#define USBPWR_PROFILE_LEGACY    0x0000         /* No low power states, policy default                                                              */
#define USBPWR_PROFILE_USB2_LPM  (USBPWR_ALLOW_L1 | USBPWR_POLICY_BALANCED)
#define USBPWR_PROFILE_USB3_LPM  (USBPWR_ALLOW_U1 | USBPWR_ALLOW_U2 | USBPWR_POLICY_BALANCED)

#endif	/* DEVICES_USBHARDWARE_H */
