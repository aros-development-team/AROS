#ifndef DEVICES_USB_MASSSTORAGE_H
#define DEVICES_USB_MASSSTORAGE_H
/*
**	$VER: usb_massstorage.h 2.0 (15.12.07)
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

/* Usb Mass Storage CBI Requests */
#define UMSR_ADSC             0x00

/* Usb Mass Storage Bulk only Requests */
#define UMSR_BULK_ONLY_RESET  0xff
#define UMSR_GET_MAX_LUN      0xfe

/* Mass Storage subclasses */
#define MS_RBC_SUBCLASS       0x01  /* Flash devices */
#define MS_ATAPI_SUBCLASS     0x02  /* CD roms etc */
#define MS_QIC157_SUBCLASS    0x03  /* Tape devices */
#define MS_UFI_SUBCLASS       0x04  /* Floppy */
#define MS_FDDATAPI_SUBCLASS  0x05  /* ATAPI Floppy */
#define MS_SCSI_SUBCLASS      0x06  /* SCSI devices */

/* Mass Storage protocols */
#define MS_PROTO_CBI          0x00 /* Control/Bulk/Interrupt transport with command completion interrupt */
#define MS_PROTO_CB           0x01 /* Control/Bulk/Interrupt transport without command completion interrupt */
#define MS_PROTO_BULK         0x50 /* Bulk-only transport */

/* Usb Mass Storage Class specific stuff */

struct UsbMSCmdBlkWrapper
{
    ULONG dCBWSignature;       /* 0x43425355 (little endian) indicating a CBW */
    ULONG dCBWTag;             /* Command Block Tag */
    ULONG dCBWDataTransferLength; /* Number of bytes to transfer */
    UBYTE bmCBWFlags;          /* Direction flag (Bit 7) */
    UBYTE bCBWLUN;             /* target logical unit number */
    UBYTE bCBWCBLength;        /* length of the command block in bytes (1-16) */
    UBYTE CBWCB[16];           /* the command block itself */
};

struct UsbMSCmdStatusWrapper
{
    ULONG dCSWSignature;       /* 0x53425355 (little endian) indicating a CSW */
    ULONG dCSWTag;             /* Command Status Tag, associated with dCBWTag */
    ULONG dCSWDataResidue;     /* Actual number of bytes to transfer */
    UBYTE bCSWStatus;          /* Status (see below) */
};

struct UsbMSCBIStatusWrapper
{
    UBYTE bType;               /* always 0x00 for valid status block (for UFI/Floppy, this is ASC) */
    UBYTE bValue;              /* mask out bit 0,1 for status (see below) (for UFI/Floppy, this is ASCQ) */
};

#define UMSCBW_SIZEOF         31 /* sizeof(UsbMSCmdBlkWrapper) will yield 32 instead of 31! */
#define UMSCSW_SIZEOF         13 /* sizeof(UsbMSCmdStatusWrapper) will yield 14 instead of 13! */

#define USMF_CSW_PASS         0x00 /* command passed */
#define USMF_CSW_FAIL         0x01 /* command failed */
#define USMF_CSW_PHASEERR     0x02 /* phase error */
#define USMF_CSW_PERSIST      0x03 /* persistant error (CBI only) */

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* DEVICES_USB_MASSSTORAGE_H */
