#ifndef DEVICES_USBHARDWARE_H
#define DEVICES_USBHARDWARE_H
/*
**	$VER: usbhardware.h 2.3 (22.12.2011)
**
**	standard usb hardware device include file
**
**	(C) Copyright 2002-2007 Chris Hodges
**	(C) Copyright 2011 AROS Development Team
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

/* IO Request structure */

struct IOUsbHWReq
{
    struct IORequest    iouh_Req;
    UWORD               iouh_Flags;          /* Transfer flags */
    UWORD               iouh_State;          /* USB State Flags */
    UWORD               iouh_Dir;            /* Direction of transfer */
    UWORD               iouh_DevAddr;        /* USB Device Address (0-127) */
    UWORD               iouh_Endpoint;       /* USB Device Endpoint (0-15) */
    UWORD               iouh_MaxPktSize;     /* Maximum packet size for multiple packet transfers */
    ULONG               iouh_Actual;         /* Actual bytes transferred */
    ULONG               iouh_Length;         /* Size of buffer */
    APTR                iouh_Data;           /* Pointer to in/out buffer */
    UWORD               iouh_Interval;       /* Interrupt Interval (in ms or 125 µSec units) */
    ULONG               iouh_NakTimeout;     /* Timeout in ms before request will be retired */
    struct UsbSetupData iouh_SetupData;      /* Setup fields for ctrl transfers */
    APTR                iouh_UserData;       /* private data, may not be touched by hardware driver, do not make assumptions about its contents */
    UWORD               iouh_ExtError;       /* Extended error code */
    /* V2 structure extension */
    UWORD               iouh_Frame;          /* current USB-Frame value and ISO start frame*/
    UWORD               iouh_SplitHubAddr;   /* For Split-Transaction HUB address */
    UWORD               iouh_SplitHubPort;   /* For Split-Transaction HUB downstream port */
    APTR                iouh_DriverPrivate1; /* private data for internal driver use */
    APTR                iouh_DriverPrivate2; /* private data for internal driver use */
};

/* Realtime ISO transfer structure as given in iouh_Data */
struct IOUsbHWRTIso
{
    struct Node *urti_Node;           /* Driver's linkage (private) */
    struct Hook *urti_InReqHook;      /* Called with struct IOUsbHWBufferReq whenever input data has arrived and is ready to be copied */
    struct Hook *urti_OutReqHook;     /* Called with struct IOUsbHWBufferReq to prepare output buffer copying */
    struct Hook *urti_InDoneHook;     /* Called with struct IOUsbHWBufferReq when input buffer has been copied */
    struct Hook *urti_OutDoneHook;    /* Called with struct IOUsbHWBufferReq when output buffer has been sent */
    ULONG        urti_OutPrefetch;    /* Maximum prefetch in bytes allowed for output */
    APTR         urti_DriverPrivate1; /* private data for internal driver use */
    APTR         urti_DriverPrivate2; /* private data for internal driver use */
};

struct IOUsbHWBufferReq
{
    UBYTE *ubr_Buffer;        /* Pointer to buffer, filled by called function */
    ULONG  ubr_Length;        /* Length of input received or output to be sent (may be adjusted by hook to force a partial copy) */
    UWORD  ubr_Frame;         /* Frame number, filled by caller (may be adjusted by output hook) */
    UWORD  ubr_Flags;         /* Flags, may be inspected and changed by hooks */
};

/* Definitions for ubr_Flags */

#define UBFB_CONTBUFFER    0  /* Set by InReqHook or OutReqHook to indicate that more buffer needs to be copied (scatter/gather) */

#define UBFF_CONTBUFFER    (1<<UBFB_CONTBUFFER)


/* non-standard commands */

#define UHCMD_QUERYDEVICE   (CMD_NONSTD+0)
#define UHCMD_USBRESET      (CMD_NONSTD+1)
#define UHCMD_USBRESUME     (CMD_NONSTD+2)
#define UHCMD_USBSUSPEND    CMD_STOP
#define UHCMD_USBOPER       CMD_START
#define UHCMD_CONTROLXFER   (CMD_NONSTD+3)
#define UHCMD_ISOXFER       (CMD_NONSTD+4)
#define UHCMD_INTXFER       (CMD_NONSTD+5)
#define UHCMD_BULKXFER      (CMD_NONSTD+6)
#define UHCMD_ADDISOHANDLER (CMD_NONSTD+7)
#define UHCMD_REMISOHANDLER (CMD_NONSTD+8)
#define UHCMD_STARTRTISO    (CMD_NONSTD+9)
#define UHCMD_STOPRTISO     (CMD_NONSTD+10)

/* Error codes for io_Error field */

#define UHIOERR_NO_ERROR      0   /* No error occured */
#define UHIOERR_USBOFFLINE    1   /* USB non-operational */
#define UHIOERR_NAK           2   /* NAK received */
#define UHIOERR_HOSTERROR     3   /* Unspecific host error */
#define UHIOERR_STALL         4   /* Endpoint stalled */
#define UHIOERR_PKTTOOLARGE   5   /* Packet is too large to be transferred */
#define UHIOERR_TIMEOUT       6   /* No acknoledge on packet */
#define UHIOERR_OVERFLOW      7   /* More data received than expected (babble condition) */
#define UHIOERR_CRCERROR      8   /* Incoming Packet corrupted */
#define UHIOERR_RUNTPACKET    9   /* Less data received than requested */
#define UHIOERR_NAKTIMEOUT   10   /* Timeout due to NAKs */
#define UHIOERR_BADPARAMS    11   /* Illegal parameters in request */
#define UHIOERR_OUTOFMEMORY  12   /* Out of auxiliary memory for the driver */
#define UHIOERR_BABBLE       13   /* Babble condition */

/* Values for iouh_Dir */

#define UHDIR_SETUP      0  /* This is a setup transfer (UHCMD_CTRLXFER) */
#define UHDIR_OUT        1  /* This is a host to device transfer */
#define UHDIR_IN         2  /* This is a device to host transfer */

/* Definitions for iouh_Flags */
#ifdef AROS_USB30_CODE
#   define UHFB_LOWSPEED      0  /* Device operates at low speed */
#   define UHFB_HIGHSPEED     1  /* Device operates at high speed (USB 2.0) */
#   define UHFB_NOSHORTPKT    2  /* Inhibit sending of a short packet at the end of a transfer (if possible) */
#   define UHFB_NAKTIMEOUT    3  /* Allow the request to time-out after the given timeout value */
#   define UHFB_ALLOWRUNTPKTS 4  /* Receiving less data than expected will not cause an UHIOERR_RUNTPACKET */
#   define UHFB_SPLITTRANS    5  /* new for V2.0: Split transaction for Lowspeed/Fullspeed devices at USB2.0 hubs */
#   define UHFB_MULTI_1       6  /* new for V2.1: Number of transactions per microframe bit 0 */
#   define UHFB_MULTI_2       7  /* new for V2.1: Number of transactions per microframe bit 1 */
#   define UHFS_THINKTIME     8  /* new for V2.2: Bit times required at most for intertransaction gap on LS/FS */
#   define UHFB_SUPERSPEED    9  /* Device operates at super speed (USB 3.0) */

#   define UHFF_LOWSPEED      (1<<UHFB_LOWSPEED)
#   define UHFF_HIGHSPEED     (1<<UHFB_HIGHSPEED)
#   define UHFF_NOSHORTPKT    (1<<UHFB_NOSHORTPKT)
#   define UHFF_NAKTIMEOUT    (1<<UHFB_NAKTIMEOUT)
#   define UHFF_ALLOWRUNTPKTS (1<<UHFB_ALLOWRUNTPKTS)
#   define UHFF_SPLITTRANS    (1<<UHFB_SPLITTRANS)
#   define UHFF_MULTI_1       (1<<UHFB_MULTI_1)
#   define UHFF_MULTI_2       (1<<UHFB_MULTI_2)
#   define UHFF_MULTI_3       ((1<<UHFB_MULTI_1)|(1<<UHFB_MULTI_2))
#   define UHFF_THINKTIME_8   (0<<UHFS_THINKTIME)
#   define UHFF_THINKTIME_16  (1<<UHFS_THINKTIME)
#   define UHFF_THINKTIME_24  (2<<UHFS_THINKTIME)
#   define UHFF_THINKTIME_32  (3<<UHFS_THINKTIME)
#   define UHFF_SUPERSPEED    (1<<UHFB_SUPERSPEED)
#else
#   define UHFB_LOWSPEED      0  /* Device operates at low speed */
#   define UHFB_HIGHSPEED     1  /* Device operates at high speed (USB 2.0) */
#   define UHFB_NOSHORTPKT    2  /* Inhibit sending of a short packet at the end of a transfer (if possible) */
#   define UHFB_NAKTIMEOUT    3  /* Allow the request to time-out after the given timeout value */
#   define UHFB_ALLOWRUNTPKTS 4  /* Receiving less data than expected will not cause an UHIOERR_RUNTPACKET */
#   define UHFB_SPLITTRANS    5  /* new for V2.0: Split transaction for Lowspeed/Fullspeed devices at USB2.0 hubs */
#   define UHFB_MULTI_1       6  /* new for V2.1: Number of transactions per microframe bit 0 */
#   define UHFB_MULTI_2       7  /* new for V2.1: Number of transactions per microframe bit 1 */
#   define UHFS_THINKTIME     8  /* new for V2.2: Bit times required at most for intertransaction gap on LS/FS */

#   define UHFF_LOWSPEED      (1<<UHFB_LOWSPEED)
#   define UHFF_HIGHSPEED     (1<<UHFB_HIGHSPEED)
#   define UHFF_NOSHORTPKT    (1<<UHFB_NOSHORTPKT)
#   define UHFF_NAKTIMEOUT    (1<<UHFB_NAKTIMEOUT)
#   define UHFF_ALLOWRUNTPKTS (1<<UHFB_ALLOWRUNTPKTS)
#   define UHFF_SPLITTRANS    (1<<UHFB_SPLITTRANS)
#   define UHFF_MULTI_1       (1<<UHFB_MULTI_1)
#   define UHFF_MULTI_2       (1<<UHFB_MULTI_2)
#   define UHFF_MULTI_3       ((1<<UHFB_MULTI_1)|(1<<UHFB_MULTI_2))
#   define UHFF_THINKTIME_8   (0<<UHFS_THINKTIME)
#   define UHFF_THINKTIME_16  (1<<UHFS_THINKTIME)
#   define UHFF_THINKTIME_24  (2<<UHFS_THINKTIME)
#   define UHFF_THINKTIME_32  (3<<UHFS_THINKTIME)
#endif

/* Tags for UHCMD_QUERYDEVICE */

#define UHA_Dummy          (TAG_USER  + 0x4711)
#define UHA_State          (UHA_Dummy + 0x01)
#define UHA_Manufacturer   (UHA_Dummy + 0x10)
#define UHA_ProductName    (UHA_Dummy + 0x11)
#define UHA_Version        (UHA_Dummy + 0x12)
#define UHA_Revision       (UHA_Dummy + 0x13)
#define UHA_Description    (UHA_Dummy + 0x14)
#define UHA_Copyright      (UHA_Dummy + 0x15)
#define UHA_DriverVersion  (UHA_Dummy + 0x20)
#define UHA_Capabilities   (UHA_Dummy + 0x21)

/* Capabilities as returned by UHA_Capabities */
#ifdef AROS_USB30_CODE
#   define UHCB_USB20         0 /* Host controller supports USB 2.0 Highspeed */
#   define UHCB_ISO           1 /* Host controller driver supports ISO transfers (UHCMD_ISOXFER) */
#   define UHCB_RT_ISO        2 /* Host controller driver supports real time ISO transfers (UHCMD_ADDISOHANDLER) */
#   define UHCB_QUICKIO       3 /* BeginIO()/AbortIO() may be called from interrupts for less overhead */
#   define UHCB_USB30         4 /* Host controller supports USB 3.0 SuperSpeed */

#   define UHCF_USB20         (1<<UHCB_USB20)
#   define UHCF_ISO           (1<<UHCB_ISO)
#   define UHCF_RT_ISO        (1<<UHCB_RT_ISO)
#   define UHCF_QUICKIO       (1<<UHCB_QUICKIO)
#   define UHCF_USB30         (1<<UHCB_USB30)
#else
#   define UHCB_USB20         0 /* Host controller supports USB 2.0 Highspeed */
#   define UHCB_ISO           1 /* Host controller driver supports ISO transfers (UHCMD_ISOXFER) */
#   define UHCB_RT_ISO        2 /* Host controller driver supports real time ISO transfers (UHCMD_ADDISOHANDLER) */
#   define UHCB_QUICKIO       3 /* BeginIO()/AbortIO() may be called from interrupts for less overhead */

#   define UHCF_USB20         (1<<UHCB_USB20)
#   define UHCF_ISO           (1<<UHCB_ISO)
#   define UHCF_RT_ISO        (1<<UHCB_RT_ISO)
#   define UHCF_QUICKIO       (1<<UHCB_QUICKIO)
#endif

#ifdef AROS_USB2OTG_CODE
#   define UHCB_USB2OTG       5 /* Host controller supports USB2OTG (Controller can be a host or a device) */

#   define UHCF_USB2OTG       (1<<UHCB_USB2OTG)
#endif

/* Definitions for UHA_State/iouh_State */

#define UHSB_OPERATIONAL 0 /* USB can be used for transfers */
#define UHSB_RESUMING    1 /* USB is currently resuming */
#define UHSB_SUSPENDED   2 /* USB is in suspended state */
#define UHSB_RESET       3 /* USB is just inside a reset phase */

#define UHSF_OPERATIONAL (1<<UHSB_OPERATIONAL)
#define UHSF_RESUMING    (1<<UHSB_RESUMING)
#define UHSF_SUSPENDED   (1<<UHSB_SUSPENDED)
#define UHSF_RESET       (1<<UHSB_RESET)

#endif	/* DEVICES_USBHARDWARE_H */
