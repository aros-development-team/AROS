#ifndef DEVICES_USB_PRINTER_H
#define DEVICES_USB_PRINTER_H
/*
**	$VER: usb_printer.h 2.0 (15.12.07)
**
**	usb definitions include file
**
**	(C) Copyright 2002-2007 Chris Hodges
**	    All Rights Reserved
*/

#if defined(__GNUC__)
# pragma pack(1)
#endif

/* Usb Printer Requests */
#define UPR_GET_DEVICE_ID     0x00
#define UPR_GET_PORT_STATUS   0x01
#define UPR_SOFT_RESET        0x02

/* Printer protocols */
#define PRT_PROTO_UNIDIR      0x01
#define PRT_PROTO_BIDIR       0x02
#define PRT_PROTO_IEEE1284    0x03

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* DEVICES_USB_PRINTER_H */
