#ifndef USB_H
#define USB_H


/* transfer types */
#define TT_ISOCHRONOUS 1
#define TT_INTERRUPT   2
#define TT_CONTROL     3
#define TT_BULK        4


/* Token */
#define PID_SETUP 0x2D
#define PID_IN    0x69
#define PID_OUT   0xE1


/* USB Device Request structure */

struct USBDeviceRequest {
	UBYTE bmRequestType;
	UBYTE bRequest;
	UWORD wValue;
	UWORD wIndex;
	UWORD wLength;
} __attribute__((packed));

/* bRequests */
#define UDRR_GET_STATUS        0x00
#define UDRR_CLEAR_FEATURE     0x01
#define UDRR_SET_FEATURE       0x03
#define UDRR_SET_ADDRESS       0x05
#define UDRR_GET_DESCRIPTOR    0x06
#define UDRR_SET_DESCRIPTOR    0x07
#define UDRR_GET_CONFIGURATION 0x08
#define UDRR_SET_CONFIGURATION 0x09
#define UDRR_GET_INTERFACE     0x0A
#define UDRR_SET_INTERFACE     0x0B
#define UDRR_SYNCH_FRAME       0x0C

/************ bmRequestTypes */
/* transfer direction in 2nd phase of control transfer */
#define UDRRT_DIR_HOST_TO_DEVICE (0<<7)
#define UDRRT_DIR_DEVICE_TO_HOST (1<<7)
/* request type */
#define UDRRT_RT_STANDARD        ((0<<6) | (0<<5))
#define UDRRT_RT_CLASS           ((0<<6) | (1<<5))
#define UDRRT_RT_VENDOR          ((1<<6) | (0<<5))
/* receiver */
#define UDRRT_REC_DEVICE    ((0<<4) | (0<<3) | (0<<2) | (0<<1) | (0<<0))
#define UDRRT_REC_INTERFACE ((0<<4) | (0<<3) | (0<<2) | (0<<1) | (1<<0))
#define UDRRT_REC_ENDPOINT  ((0<<4) | (0<<3) | (0<<2) | (1<<1) | (0<<0))
#define UDRRT_REC_OTHER     ((0<<4) | (0<<3) | (0<<2) | (1<<1) | (1<<0))


/* descriptor types (GET_DESCRIPTOR) */
#define GDT_DEVICE           0x01
#define GDT_CONFIGURATION    0x02
#define GDT_STRING           0x03
#define GDT_INTERFACE        0x04
#define GDT_ENDPOINT         0x05
#define GDT_DEVICE_QUALIFIER 0x06
#define GDT_OTHER_SPEED_CONFIGURATION 0x07
#define GDT_INTERFACE_POWER  0x08

/********** GET_DESCRIPTOR data definitions */
struct DescriptorHeader {
	UBYTE bLength;
	UBYTE bDescriptorType;
} __attribute__((packed));

struct DescriptorDevice {
	struct DescriptorHeader header;
	UWORD bcdUSB;
	UBYTE bDeviceClass;
	UBYTE bDeviceSubClass;
	UBYTE bDeviceProtocol;
	UBYTE bMaxPacketSize0;
	UWORD idVendor;
	UWORD idProduct;
	UWORD bcdDevice;
	UBYTE iManufacturer;
	UBYTE iProduct;
	UBYTE iSerialNumber;
	UBYTE bNumConfigurations;
} __attribute__((packed));

struct DescriptorDeviceQualifier {
	struct DescriptorHeader header;
	UWORD bcdUSB;
	UBYTE bDeviceClass;
	UBYTE bDeviceSubClass;
	UBYTE bDeviceProtocol;
	UBYTE bMaxPacketSize0;
	UBYTE bNumConfigurations;
	UBYTE bReserved;
} __attribute__((packed));

struct ConfigurationDescriptor{
	struct DescriptorHeader header;
	UWORD wTotalLength;
	UBYTE bNumInterfaces;
	UBYTE bConfigurationValue;
	UBYTE iConfiguration;
	UBYTE bmAttributes;
	UBYTE bMaxPower;
} __attribute__((packed));

struct DescriptorOtherSpeedConfiguration {
	struct DescriptorHeader header;
	UWORD wTotalLength;
	UBYTE bNumInterfaces;
	UBYTE bConfigurationValue;
	UBYTE iConfiguration;
	UBYTE bmAttributes;
	UBYTE bMaxPower;
} __attribute__((packed));

struct DescriptorInterface {
	struct DescriptorHeader header;
	UBYTE bInterfaceNumber;
	UBYTE bAlternateSetting;
	UBYTE bNumEndpoints;
	UBYTE bInterfaceClass;
	UBYTE bInterfaceSubClass;
	UBYTE bInterfaceProtocol;
	UBYTE iInterface;
} __attribute__((packed));

struct DescriptorEndpoint {
	struct DescriptorHeader header;
	UBYTE bEndpointAddress;
	UBYTE bmAttributes;
	UWORD wMaxPacketSize;
	UBYTE bInterval;
} __attribute__((packed));

struct DescriptorString {
	struct DescriptorHeader header;
	UWORD wLANGID[1];
} __attribute__((packed));

#endif

