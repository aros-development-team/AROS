#ifndef DEVICES_USB_DFU_H
#define DEVICES_USB_DFU_H
/*
**	$VER: usb_dfu.h 2.0 (15.12.07)
**
**	usb definitions include file
**
**	(C) Copyright 2002-2007 Chris Hodges
**	    All Rights Reserved
*/

#include <exec/types.h>

#if defined(__GNUC__)
# pragma pack(1)
#endif

/* Usb DFU Requests */
#define UDFUR_DETACH              0x00
#define UDFUR_DNLOAD              0x01
#define UDFUR_UPLOAD              0x02
#define UDFUR_GETSTATUS           0x03
#define UDFUR_CLRSTATUS           0x04
#define UDFUR_GETSTATE            0x05
#define UDFUR_ABORT               0x06

/* DFU class specific descriptors */
#define UDT_DFU               0x21

/* Firmware Upgrade subclasses */
#define FWUPGRADE_STD_SUBCLASS 0x01

/* Firmware Upgrade protocols */
#define FWUPGRADE_PROTO_STD   0x01
#define FWUPGRADE_PROTO_DFU   0x02

/* DFU specific descriptor */

struct UsbDFUDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x21) */
    UBYTE bmAttributes;        /* DFU attributes */
    UBYTE wDetachTimeOut0;     /* Lowbyte of Detach TimeOut */
    UBYTE wDetachTimeOut1;     /* Highbyte of above */
    UBYTE wTransferSize0;      /* Maximum bytes per control-write (Lowbyte) */
    UBYTE wTransferSize1;      /* Highbyte of above */
    UBYTE bcdDFUVersion;       /* DFU Spec Release */
};

/* bmAttributes from above */
#define UDDAF_DOWNLOADABLE    0x0001
#define UDDAF_UPLOADABLE      0x0002
#define UDDAF_NO_MANIFEST_RST 0x0004
#define UDDAF_WILL_DETACH     0x0008

/* Returned by UDFUR_GETSTATUS */
struct UsbDFUStatus
{
    UBYTE bStatus;
    UBYTE bwPollTimeout0;
    UBYTE bwPollTimeout1;
    UBYTE bwPollTimeout2;
    UBYTE bState;
    UBYTE iString;
};


#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* DEVICES_USB_DFU_H */
