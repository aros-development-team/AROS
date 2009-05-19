#ifndef DEVICES_USB_HID_H
#define DEVICES_USB_HID_H
/*
**	$VER: usb_hid.h 2.0 (15.12.07)
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

/* Usb Hid Requests */
#define UHR_GET_REPORT        0x01
#define UHR_GET_IDLE          0x02
#define UHR_GET_PROTOCOL      0x03
#define UHR_SET_REPORT        0x09
#define UHR_SET_IDLE          0x0a
#define UHR_SET_PROTOCOL      0x0b

/* HID class specific descriptors */
#define UDT_HID               0x21
#define UDT_REPORT            0x22
#define UDT_PHYSICAL          0x23

/* Hid Subclasses */
#define HID_NO_SUBCLASS       0x00
#define HID_BOOT_SUBCLASS     0x01

/* Hid Proto if HID_BOOT_SUBCLASS */
#define HID_PROTO_KEYBOARD    0x01
#define HID_PROTO_MOUSE       0x02

/* Hid Proto values for UHR_SET_PROTOCOL */
#define HID_PROTO_BOOT        0x00
#define HID_PROTO_REPORT      0x01

/* Usb Class Specific Descriptor: HID Descriptor */

struct UsbHidDesc
{
    UBYTE bLength;             /* total size of the HID descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type, value UDT_HID for HID */
    UWORD bcdHID;              /* the HID Class Spec release */
    UBYTE bCountryCode;        /* country code of the localized hardware. */
    UBYTE bNumDescriptors;     /* number of class descriptors (>=1) */
    UBYTE bDescType;           /* type of class descriptor */
    UBYTE wDescLength;         /* (WORD!) total size of the Report descriptor */
    UBYTE wPad0;
};

/* HID Report Item stuff */

#define REPORT_LONGITEM       0xfe
#define REPORT_ISIZE_0        0x00
#define REPORT_ISIZE_1        0x01
#define REPORT_ISIZE_2        0x02
#define REPORT_ISIZE_4        0x03
#define REPORT_ISIZE_MASK     0x03

#define REPORT_ITYPE_MAIN     0x00
#define REPORT_ITYPE_GLOBAL   0x04
#define REPORT_ITYPE_LOCAL    0x08
#define REPORT_ITYPE_MASK     0x0c

#define REPORT_ITAG_MASK      0xf0

/* main items */
#define REPORT_MAIN_INPUT     0x80
#define REPORT_MAIN_OUTPUT    0x90
#define REPORT_MAIN_COLLECT   0xa0
#define REPORT_MAIN_FEATURE   0xb0
#define REPORT_MAIN_ENDCOLL   0xc0

/* global items */
#define REPORT_GLOB_USAGE     0x00
#define REPORT_GLOB_LOGMIN    0x10
#define REPORT_GLOB_LOGMAX    0x20
#define REPORT_GLOB_PHYMIN    0x30
#define REPORT_GLOB_PHYMAX    0x40
#define REPORT_GLOB_UNITEXP   0x50
#define REPORT_GLOB_UNIT      0x60
#define REPORT_GLOB_RPSIZE    0x70
#define REPORT_GLOB_RPID      0x80
#define REPORT_GLOB_RPCOUNT   0x90
#define REPORT_GLOB_PUSH      0xa0
#define REPORT_GLOB_POP       0xb0

/* local items */
#define REPORT_LOCL_USAGE     0x00
#define REPORT_LOCL_USEMIN    0x10
#define REPORT_LOCL_USEMAX    0x20
#define REPORT_LOCL_DESIDX    0x30
#define REPORT_LOCL_DESMIN    0x40
#define REPORT_LOCL_DESMAX    0x50
#define REPORT_LOCL_STRIDX    0x70
#define REPORT_LOCL_STRMIN    0x80
#define REPORT_LOCL_STRMAX    0x90
#define REPORT_LOCL_DELIM     0xa0

/* Unit data definitons */

#define RP_UNIT_NONE          0x0

/* System units (nibble 0) */
#define RP_UNIT_SILINEAR      0x1
#define RP_UNIT_SIROTATION    0x2
#define RP_UNIT_ENGLINEAR     0x3
#define RP_UNIT ENGROTATION   0x4

/* Length units (nibble 1) */
#define RP_UNIT_CM            0x1
#define RP_UNIT_RAD           0x2
#define RP_UNIT_INCH          0x3
#define RP_UNIT_DEG           0x4

/* Mass units (nibble 2) */
#define RP_UNIT_GRAM1         0x1
#define RP_UNIT_GRAM2         0x2
#define RP_UNIT_SLUG1         0x3
#define RP_UNIT_SLUG2         0x4

/* Time units (nibble 3) */
#define RP_UNIT_SECS1         0x1
#define RP_UNIT_SECS2         0x2
#define RP_UNIT_SECS3         0x3
#define RP_UNIT_SECS4         0x4

/* Temperature units (nibble 4) */
#define RP_UNIT_KELVIN1       0x1
#define RP_UNIT_KELVIN2       0x2
#define RP_UNIT_FAHRENHEIT1   0x3
#define RP_UNIT_FAHRENHEIT2   0x4

/* Current units (nibble 5)   */
#define RP_UNIT_AMPERE1       0x1
#define RP_UNIT_AMPERE2       0x2
#define RP_UNIT_AMPERE3       0x3
#define RP_UNIT_AMPERE4       0x4

/* Lumious intensity units (nibble 6) */
#define RP_UNIT_CANDELA1      0x1
#define RP_UNIT_CANDELA2      0x2
#define RP_UNIT_CANDELA3      0x3
#define RP_UNIT_CANDELA4      0x4


/* Data for input/output/feature main items */

#define RPF_MAIN_CONST        0x0001 /* !DATA */
#define RPF_MAIN_VARIABLE     0x0002 /* !ARRAY */
#define RPF_MAIN_RELATIVE     0x0004 /* !ABSOLUTE */
#define RPF_MAIN_WRAP         0x0008 /* !NOWRAP */
#define RPF_MAIN_NONLINEAR    0x0010 /* !LINEAR */
#define RPF_MAIN_NOPREF       0x0020 /* !PREFERRED */
#define RPF_MAIN_NULLSTATE    0x0040 /* !NO NULL POS */
#define RPF_MAIN_VOLATILE     0x0080 /* !NON VOLATILE (only output/feature) */
#define RPF_MAIN_BUFBYTES     0x0100 /* !BITFIELD */

/* Data for collection main items */

#define RP_COLL_PHYSICAL      0x00 /* group of axes */
#define RP_COLL_APP           0x01 /* mouse, keyboard */
#define RP_COLL_LOGICAL       0x02 /* interrelated data */
#define RP_COLL_REPORT        0x03 /* Collection around a report */
#define RP_COLL_NAMEDARRAY    0x04 /* Named Array, array of selector usages */
#define RP_COLL_USAGESWITCH   0x05 /* Usage Switch */
#define RP_COLL_USAGEMODIFIER 0x06 /* Usage Modifier */

/* Usage Page IDs */

#define RP_PAGE_GENERIC       0x01
#define RP_PAGE_SIMCTRLS      0x02
#define RP_PAGE_VRCTRLS       0x03
#define RP_PAGE_SPORTCTRLS    0x04
#define RP_PAGE_GAMECTRLS     0x05
#define RP_PAGE_KEYBOARD      0x07
#define RP_PAGE_LEDS          0x08
#define RP_PAGE_BUTTON        0x09
#define RP_PAGE_ORDINAL       0x0a
#define RP_PAGE_TELEPHONY     0x0b
#define RP_PAGE_CONSUMER      0x0c
#define RP_PAGE_DIGITIZER     0x0d
#define RP_PAGE_PID           0x0f
#define RP_PAGE_UNICODE       0x10
#define RP_PAGE_ALPHADISP     0x14
#define RP_PAGE_MONITOR       0x80
#define RP_PAGE_MONITORENUM   0x81
#define RP_PAGE_MONITORVESA   0x82
#define RP_PAGE_POWER         0x84
#define RP_PAGE_CAMERACTRL    0x90
#define RP_PAGE_ARCADE        0x91

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* DEVICES_USB_HID_H */
