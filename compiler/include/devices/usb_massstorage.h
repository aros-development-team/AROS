#ifndef DEVICES_USB_MASSSTORAGE_H
#define DEVICES_USB_MASSSTORAGE_H
/*
**	$VER: usb_massstorage.h 2.1 (21.12.2025)
**
**	usb definitions include file
**
**	(C) Copyright 2002-2007 Chris Hodges
**	    All Rights Reserved
*/

#include <exec/types.h>

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
#define MS_PROTO_UAS          0x62 /* USB Attached SCSI (UASP) */

#if defined(__GNUC__)
# pragma pack(1)
#endif

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

#if defined(__GNUC__)
# pragma pack()
#endif

#define UMSCBW_SIZEOF         31 /* sizeof(UsbMSCmdBlkWrapper) will yield 32 instead of 31! */
#define UMSCSW_SIZEOF         13 /* sizeof(UsbMSCmdStatusWrapper) will yield 14 instead of 13! */

#define USMF_CSW_PASS         0x00 /* command passed */
#define USMF_CSW_FAIL         0x01 /* command failed */
#define USMF_CSW_PHASEERR     0x02 /* phase error */
#define USMF_CSW_PERSIST      0x03 /* persistant error (CBI only) */

/* UAS/UASP descriptor and pipe usage definitions */
#define UAS_DESC_PIPE_USAGE  0x24
#define UAS_DESC_CS_ENDPOINT 0x25

#define UAS_PIPE_ID_COMMAND  0x01
#define UAS_PIPE_ID_STATUS   0x02
#define UAS_PIPE_ID_DATA_IN  0x03
#define UAS_PIPE_ID_DATA_OUT 0x04

/* UAS IU identifiers */
#define UAS_IU_ID_COMMAND     0x01
#define UAS_IU_ID_STATUS      0x03
#define UAS_IU_ID_RESPONSE    0x04
#define UAS_IU_ID_READ_READY  0x05
#define UAS_IU_ID_WRITE_READY 0x06
#define UAS_IU_ID_SENSE       0x07

#if defined(__GNUC__)
# pragma pack(1)
#endif

struct UasCommandIU
{
    UBYTE  iu_Id;
    UBYTE  iu_Reserved1;
    UBYTE  iu_Reserved2;
    UBYTE  iu_TaskAttr;
    ULONG  iu_Tag;
    UBYTE  iu_Lun[8];
    UBYTE  iu_Cdb[16];
};

struct UasStatusIU
{
    UBYTE  iu_Id;
    UBYTE  iu_Reserved1;
    UBYTE  iu_Status;
    UBYTE  iu_Reserved2;
    ULONG  iu_Tag;
    ULONG  iu_Residue;
    UBYTE  iu_Reserved3[4];
};

struct UasSenseIU
{
    UBYTE  iu_Id;
    UBYTE  iu_Reserved1;
    UBYTE  iu_Status;
    UBYTE  iu_Reserved2;
    ULONG  iu_Tag;
    UWORD  iu_SenseLength;
    UBYTE  iu_Reserved3[2];
    UBYTE  iu_Sense[18];
};

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* DEVICES_USB_MASSSTORAGE_H */
