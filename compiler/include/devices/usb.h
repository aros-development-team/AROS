#ifndef DEVICES_USB_H
#define DEVICES_USB_H
/*
**	$VER: usb.h 2.0 (15.12.07)
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

/* Flags for bmRequestType */
#define URTF_OUT              0x00      /* direction: host to device */
#define URTF_IN               0x80      /* direction: device to host */
#define URTF_STANDARD         0x00      /* type: usb standard request */
#define URTF_CLASS            0x20      /* type: class request */
#define URTF_VENDOR           0x40      /* type: vendor specific request */
#define URTF_DEVICE           0x00      /* target: device */
#define URTF_INTERFACE        0x01      /* target: interface */
#define URTF_ENDPOINT         0x02      /* target: endpoint */
#define URTF_OTHER            0x03      /* target: other */

/* Usb Standard Requests (only for URTF_STANDARD) */
#define USR_GET_STATUS        0x00
#define USR_CLEAR_FEATURE     0x01
#define USR_SET_FEATURE       0x03
#define USR_SET_ADDRESS       0x05
#define USR_GET_DESCRIPTOR    0x06
#define USR_SET_DESCRIPTOR    0x07
#define USR_GET_CONFIGURATION 0x08
#define USR_SET_CONFIGURATION 0x09
#define USR_GET_INTERFACE     0x0a
#define USR_SET_INTERFACE     0x0b
#define USR_SYNCH_FRAME       0x0c

/* Usb Standard Feature Selectors */
#define UFS_DEVICE_REMOTE_WAKEUP  0x01 /* Recipient: Device */
#define UFS_ENDPOINT_HALT         0x00 /* Recipient: Endpoint */
#define UFS_TEST_MODE             0x02 /* Recipient: Device */

/* Usb GetStatus() data bits (LE-UWORD) */
#define U_GSB_SELF_POWERED        8
#define U_GSB_REMOTE_WAKEUP       9

#define U_GSF_SELF_POWERED        (1<<U_GSB_SELF_POWERED)
#define U_GSF_REMOTE_WAKEUP       (1<<U_GSB_REMOTE_WAKEUP)

/* Usb Descriptor Types */
#define UDT_DEVICE                0x01
#define UDT_CONFIGURATION         0x02
#define UDT_STRING                0x03
#define UDT_INTERFACE             0x04
#define UDT_ENDPOINT              0x05

#define UDT_DEVICE_QUALIFIER      0x06
#define UDT_OTHERSPEED_QUALIFIER  0x07
#define UDT_INTERFACE_POWER       0x08
#define UDT_OTG                   0x09
#define UDT_DEBUG                 0x0a
#define UDT_INTERFACE_ASSOCIATION 0x0b
#define UDT_SECURITY              0x0c
#define UDT_ENCRYPTION_TYPE       0x0e
#define UDT_BOS                   0x0f
#define UDT_DEVICE_CAPABILITY     0x10
#define UDT_WIRELESS_EP_COMP      0x11 /* Wireless endpoint companion descriptor */

/* common class specific descriptors */
#define UDT_CS_UNDEFINED      0x20
#define UDT_CS_DEVICE         0x21
#define UDT_CS_CONFIGURATION  0x22
#define UDT_CS_STRING         0x23
#define UDT_CS_INTERFACE      0x24
#define UDT_CS_ENDPOINT       0x25

/* SMARTCARD class specific descriptors */
#define UDT_CCID              0x21 /* Smart card CCID functional descriptor */

/* Device Wire Adapter class specific descriptors */
#define UDT_WIREADAPTER_CLASS 0x21 /* Wire Adapter class descriptor */
#define UDT_WIREADAPTER_RPIPE 0x22 /* Wire Adapter RPipe descriptor */
#define UDT_RADIO_CONTROL_IF  0x23 /* Radio Control Interface Class descriptor */

/* Format of 8-bytes setup packet */
struct UsbSetupData
{
    UBYTE bmRequestType;       /* Request type and direction */
    UBYTE bRequest;            /* Request identifier */
    UWORD wValue;              /* request specific value, little endian! */
    UWORD wIndex;              /* request specific index, little endian! */
    UWORD wLength;             /* length of data to transfer, little endian! */
};

/* Usb Standard Device Descriptor */
struct UsbStdDevDesc
{
    UBYTE bLength;             /* Size of this descriptor in bytes */
    UBYTE bDescriptorType;     /* UDT_DEVICE Descriptor Type */
    UWORD bcdUSB;              /* USB Specification Release Number */
    UBYTE bDeviceClass;        /* Class code (assigned by the USB). */
    UBYTE bDeviceSubClass;     /* Subclass code (assigned by the USB). */
    UBYTE bDeviceProtocol;     /* Protocol code (assigned by the USB). */
    UBYTE bMaxPacketSize0;     /* Maximum packet size for endpoint zero (only 8, 16, 32, or 64 are valid) */
    UWORD idVendor;            /* Vendor ID (assigned by the USB) */
    UWORD idProduct;           /* Product ID (assigned by the manufacturer) */
    UWORD bcdDevice;           /* Device release number in binary-coded decimal */
    UBYTE iManufacturer;       /* Index of string descriptor describing manufacturer */
    UBYTE iProduct;            /* Index of string descriptor describing product */
    UBYTE iSerialNumber;       /* Index of string descriptor describing the device's serial number */
    UBYTE bNumConfigurations;  /* Number of possible configurations */
};

/* Usb Standard Configuration Descriptor */
struct UsbStdCfgDesc
{
    UBYTE bLength;             /* Size of this descriptor in bytes */
    UBYTE bDescriptorType;     /* UDT_CONFIGURATION Descriptor Type */
    UWORD wTotalLength;        /* Total length of data returned for this configuration. */
    UBYTE bNumInterfaces;      /* Number of interfaces supported by this configuration. */
    UBYTE bConfigurationValue; /* Value to use as an argument to the SetConfiguration() request */
    UBYTE iConfiguration;      /* Index of string descriptor describing this configuration */
    UBYTE bmAttributes;        /* Configuration characteristics */
    UBYTE bMaxPower;           /* Maximum power consumption of the USB device (2mA units) */
};

/* Flags for bmAttributes */
#define USCAF_ONE             0x80
#define USCAF_SELF_POWERED    0x40
#define USCAF_REMOTE_WAKEUP   0x20

struct UsbStdIfDesc
{
    UBYTE bLength;             /* Size of this descriptor in bytes */
    UBYTE bDescriptorType;     /* UDT_INTERFACE Descriptor Type */
    UBYTE bInterfaceNumber;    /* Number of interface. */
    UBYTE bAlternateSetting;   /* Value used to select alternate setting */
    UBYTE bNumEndpoints;       /* Number of endpoints used by this interface (excluding endpoint zero). */
    UBYTE bInterfaceClass;     /* Class code (assigned by the USB). */
    UBYTE bInterfaceSubClass;  /* Subclass code (assigned by the USB). */
    UBYTE bInterfaceProtocol;  /* Protocol code (assigned by the USB). */
    UBYTE iInterface;          /* Index of string descriptor describing this interface */
};

/* Usb Standard Endpoint Descriptor */

struct UsbStdEPDesc
{
    UBYTE bLength;             /* Size of this descriptor in bytes */
    UBYTE bDescriptorType;     /* UDT_ENDPOINT Descriptor Type */
    UBYTE bEndpointAddress;    /* The address of the endpoint on the USB, MSB holds direction */
    UBYTE bmAttributes;        /* TransferType (00=Control, 01=Iso, 10=Bulk, 11=Interrupt */
    UWORD wMaxPacketSize;      /* Maximum packet size this endpoint is capable of sending or receiving */
    UBYTE bInterval;           /* Interval for polling endpoint for data transfers in ms */
};

/* Usb Standard String Descriptors */
struct UsbStdStrDesc
{
    UBYTE bLength;             /* Size of this descriptor in bytes */
    UBYTE bDescriptorType;     /* UDT_STRING Descriptor Type */
    UWORD bString[1];          /* UNICODE encoded string */
};

/* Flags for bmAttributes */
#define USEAF_CONTROL         0x00
#define USEAF_ISOCHRONOUS     0x01
#define USEAF_BULK            0x02
#define USEAF_INTERRUPT       0x03

/* Flags for Synchronization Type (already shifted right by 2) */
#define USEAF_NOSYNC          0x00
#define USEAF_ASYNC           0x01
#define USEAF_ADAPTIVE        0x02
#define USEAF_SYNC            0x03

/* Flags for Usage Type (already shifted right by 2) */
#define USEAF_DATA            0x00
#define USEAF_FEEDBACK        0x01
#define USEAF_IMPLFEEDBACK    0x02

/* Standard classes */
#define AUDIO_CLASSCODE       0x01
#define CDCCTRL_CLASSCODE     0x02
#define CDCDATA_CLASSCODE     0x0a
#define HID_CLASSCODE         0x03
#define PHYSICAL_CLASSCODE    0x05
#define STILLIMG_CLASSCODE    0x06
#define PRINTER_CLASSCODE     0x07
#define MASSSTORE_CLASSCODE   0x08
#define HUB_CLASSCODE         0x09
#define SMARTCARD_CLASSCODE   0x0b
#define SECURITY_CLASSCODE    0x0d
#define VIDEO_CLASSCODE       0x0e
#define BLUETOOTH_CLASSCODE   0xe0
#define MISC_CLASSCODE        0xef
#define FWUPGRADE_CLASSCODE   0xfe
#define VENDOR_CLASSCODE      0xff

/* Misc Subclasses */
#define MISC_COMMON_SUBCLASS  0x02

/* Misc Subclass Common protocols */
#define MISC_PROTO_WUSB_WAMP  0x02 /* Wire Adapter Multifunction Peripheral */

/* Bluetooth subclasses */
#define BLUETOOTH_RF_SUBCLASS 0x01
#define BLUETOOTH_WUSB_SUBCLASS 0x02 /* Wireless USB Wire Adapter */

/* Bluetooth RF protocols */
#define BLUETOOTH_PROTO_PRG   0x01
#define BLUETOOTH_PROTO_UWB   0x02 /* UWB Radio Control Interface Programming Interface */

/* Wireless WUSB protocols */
#define BLUETOOTH_PROTO_HWA   0x01 /* Host Wire Adapter Control */

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* DEVICES_USB_H */
