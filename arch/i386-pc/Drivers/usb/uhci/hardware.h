#ifndef HARDWARE_H
#define HARDWARE_H

#include <hidd/pcibus.h>
#include "usb.h"

#define VENDOR_INTEL      0x8086
#define BASE_CLASS_SERIAL 0x0C
#define SUB_CLASS_USB     0x03
#define INTERFACE_UHCI    0x00
#define DEVICE_82371AB_2  0x7112

#define PORT_COMMAND           0x00 /* word */
#define PORT_STATUS            0x02 /* word */
#define PORT_INTERRUPT_ENABLE  0x04 /* word */
#define PORT_FRAME_NUMBER      0x06 /* word */
#define PORT_FRAME_LIST_BASE   0x08 /* long */
#define PORT_SOF_MODIFY        0x0C /* byte */
#define PORT_PORT1_STATUS_CTRL 0x10 /* word */
#define PORT_PORT2_STATUS_CTRL 0x12 /* word */

/* command port bits */
#define CMD_RUNSTOP        (1<<0)
#define CMD_CONFIGURE_FLAG (1<<6)

/* interrupt enable port bits */
#define IE_TIMEOUT_CRC  (1<<0)
#define IE_RESUME       (1<<1)
#define IE_ON_COMPLETE  (1<<2)
#define IE_SHORT_PACKET (1<<3)

/* port 1/2 status bits */
#define PSC_CONNECT_STATUS        (1<<0)
#define PSC_CONNECT_STATUS_CHANGE (1<<1)
#define PSC_ENABLE                (1<<2)
#define PSC_ENABLE_CHANGE         (1<<3)
#define PSC_LOW_SPEED             (1<<8)
#define PSC_RESET                 (1<<9)

struct UHCIData {
	UWORD baseport;       /* io base address */
	ULONG *realframebase; /* allocated framebase pointer */
	ULONG *framebase;     /* used framebase pointer (4k aligned) */
	struct TransferContext *flp; /* initial TD for all frame list pointers */
	UBYTE abitmap[16];    /* bitmap of free addresses for ports */
};

struct PortUnit {
	struct UHCIData *uhci;
	struct MsgPort *tmp;
	struct timerequest *treq;
	UBYTE portnum;
	UBYTE flags;
	UBYTE Endpoint;
	UBYTE DeviceAddress;
	struct DescriptorDevice dd;
};
#define PUF_LOW_SPEED (1<<0)


/* PORT: frame list pointer defines */
#define FLP_TERMINATE           (1<<0)
#define FLP_TYPE                (1<<1)
#define FLP_TYPE_QH             (1<<1)
#define FLP_TYPE_TD             (0<<1)

/* PORT: status register defines */
#define STATUS_HC_HALTED        (1<<5)
#define STATUS_HC_PROCESS_ERROR (1<<4)
#define STATUS_HS_ERROR         (1<<3)
#define STATUS_RESUME_DETECT    (1<<2)
#define STATUS_ERROR_INTERRUPT  (1<<1)
#define STATUS_USB_INTERRUPT    (1<<0)

/************ DATA: Transfer Descriptor defines */
/* general */
#define TD_BYTE_SIZE 0x10
#define TD_LONG_SIZE 0x04
/* Link Pointer */
#define TD_LP_TERMINATE           (1<<0)
#define TD_LP_TYPE                (1<<1)
#define TD_LP_TYPE_QH             (1<<1)
#define TD_LP_TYPE_TD             (0<<1)
#define TD_LP_DEPTH_FIRST         (1<<2)
#define TD_LP_BREADTH_FIRST       (0<<2)
/* Control */
#define TD_CTRL_LOW_SPEED         (1<<26)
/* Status */
#define TD_STATUS_ACTIVE            (1<<23)
#define TD_STATUS_STALLED           (1<<22)
#define TD_STATUS_DATA_BUFFER_ERROR (1<<21)
#define TD_STATUS_BABBLE_DETECTED   (1<<20)
#define TD_STATUS_NAK_RECEIVED      (1<<19)
#define TD_STATUS_CRC_TIMEOUT_ERROR (1<<18)
#define TD_STATUS_BITSTUFF_ERROR    (1<<17)
/* Token */
#define TD_TOKEN_DATA0  (0<<19)
#define TD_TOKEN_DATA1  (1<<19)
#define TD_TOKEN_TOGGLE (1<<19)

struct TransferContext {
	struct Node ln;
	struct List qlist; /* in case of queued head */
	ULONG rtd[TD_LONG_SIZE+4];
	ULONG *td;
};

#define TCT_TD 1
#define TCT_QH 2

struct TransferData {
	struct USBDeviceRequest *req; /* in case of CONTROL */
	struct List list;             /* list of to be transfered data (TransferContext-s) */
	APTR data;
	LONG size;
	UBYTE mode; /* CONTROL, ... */
};

BOOL initUSBUHCIHW(struct UHCIData *, HIDDT_PCI_Device *);

#endif
