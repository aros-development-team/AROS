#ifndef DEVICES_USB_CDC_H
#define DEVICES_USB_CDC_H
/*
**	$VER: usb_cdc.h 2.0 (15.12.07)
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

/* Usb CDC Requests (all of these target to an interface) */
#define UCDCR_SEND_ENCAPSULATED_COMMAND  0x00
#define UCDCR_GET_ENCAPSULATED_RESPONSE  0x01
#define UCDCR_SET_COMM_FEATURE           0x02
#define UCDCR_GET_COMM_FEATURE           0x03
#define UCDCR_CLEAR_COMM_FEATURE         0x04
#define UCDCR_SET_AUX_LINE_STATE         0x10
#define UCDCR_SET_HOOK_STATE             0x11
#define UCDCR_PULSE_SETUP                0x12
#define UCDCR_SEND_PULSE                 0x13
#define UCDCR_SET_PULSE_TIME             0x14
#define UCDCR_RING_AUX_JACK              0x15
#define UCDCR_SET_LINE_CODING            0x20
#define UCDCR_GET_LINE_CODING            0x21
#define UCDCR_SET_CONTROL_LINE_STATE     0x22
#define UCDCR_SEND_BREAK                 0x23
#define UCDCR_SET_RINGER_PARMS           0x30
#define UCDCR_GET_RINGER_PARMS           0x31
#define UCDCR_SET_OPERATION_PARMS        0x32
#define UCDCR_GET_OPERATION_PARMS        0x33
#define UCDCR_SET_LINE_PARMS             0x34
#define UCDCR_GET_LINE_PARMS             0x35
#define UCDCR_DIAL_DIGITS                0x36
#define UCDCR_SET_UNIT_PARAMETER         0x37
#define UCDCR_GET_UNIT_PARAMETER         0x38
#define UCDCR_CLEAR_UNIT_PARAMETER       0x39
#define UCDCR_GET_PROFILE                0x3A
#define UCDCR_SET_ETHERNET_MULTICAST_FILTERS 0x40
#define UCDCR_SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER 0x41
#define UCDCR_GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER 0x42
#define UCDCR_SET_ETHERNET_PACKET_FILTER 0x43
#define UCDCR_GET_ETHERNET_STATISTIC     0x44
#define UCDCR_SET_ATM_DATA_FORMAT        0x50
#define UCDCR_GET_ATM_DEVICE_STATISTICS  0x51
#define UCDCR_SET_ATM_DEFAULT_VC         0x52
#define UCDCR_GET_ATM_VC_STATISTICS      0x53

/* Usb CDC Notify Requests */
#define UCDCR_NETWORK_CONNECTION         0x00
#define UCDCR_RESPONSE_AVAILABLE         0x01
#define UCDCR_AUX_JACK_HOOK_STATE        0x08
#define UCDCR_RING_DETECT                0x09
#define UCDCR_SERIAL_STATE               0x20
#define UCDCR_CALL_STATE_CHANGE          0x28
#define UCDCR_LINE_STATE_CHANGE          0x29
#define UCDCR_CONNECTION_SPEED_CHANGE    0x2A

/* CDC WMC class specific descriptor subtypes */
#define UDST_CDC_HEADER       0x00 /* Header Functional Descriptor */
#define UDST_CDC_CALLMGM      0x01 /* Call Management Functional Descriptor */
#define UDST_CDC_ACM          0x02 /* Abstract Control Management Functional Descriptor */
#define UDST_CDC_DIRECTLINE   0x03 /* Direct Line Management Functional Descriptor */
#define UDST_CDC_PHONERING    0x04 /* Telephone Ringer Functional Descriptor */
#define UDST_CDC_PHONECALL    0x05 /* Telephone Call and Line State Reporting Capabilities Functional Descriptor */
#define UDST_CDC_UNION        0x06 /* Union Functional descriptor */
#define UDST_CDC_COUNTRY      0x07 /* Country Selection Functional Descriptor */
#define UDST_CDC_PHONEOP      0x08 /* Telephone Operational Modes Functional Descriptor */
#define UDST_CDC_TERMINAL     0x09 /* USB Terminal Functional Descriptor */
#define UDST_CDC_NETWORK      0x0a /* Network Channel Terminal Descriptor */
#define UDST_CDC_PROTOCOL     0x0b /* Protocol Unit Functional Descriptor */
#define UDST_CDC_EXTENSION    0x0c /* Extension Unit Functional Descriptor */
#define UDST_CDC_MCM          0x0d /* Multi-Channel Management Functional Descriptor */
#define UDST_CDC_CAPICTRL     0x0e /* CAPI Control Management Functional Descriptor */
#define UDST_CDC_ETHERNET     0x0f /* Ethernet Networking Functional Descriptor */
#define UDST_CDC_ATM          0x10 /* ATM Networking Functional Descriptor */
#define UDST_CDC_WMC          0x11 /* Wireless Handset Control Model Functional Descriptor */
#define UDST_CDC_MDLM         0x12 /* Mobile Direct Line Model Functional Descriptor */
#define UDST_CDC_MDLMDETAIL   0x13 /* MDLM Detail Functional Descriptor */
#define UDST_CDC_DEVMANGM     0x14 /* Device Management Model Functional Descriptor */
#define UDST_CDC_OBEX         0x15 /* OBEX Functional Descriptor */
#define UDST_CDC_CMDSET       0x16 /* Command Set Functional Descriptor */
#define UDST_CDC_CMDSETDETAIL 0x17 /* Command Set Detail Functional Descriptor */
#define UDST_CDC_TCM          0x18 /* Telephone Control Model Functional Descriptor */

/* CDC Control Interface subclasses */
#define CDC_DLCM_SUBCLASS     0x01 /* Direct Line Control Model */
#define CDC_ACM_SUBCLASS      0x02 /* Abstract Control Model */
#define CDC_TCM_SUBCLASS      0x03 /* Telephone Control Model */
#define CDC_MCCM_SUBCLASS     0x04 /* Multi-Channel Control Model */
#define CDC_CAPICM_SUBCLASS   0x05 /* CAPI Control Model */
#define CDC_ETHCM_SUBCLASS    0x06 /* Ethernet Networking Control Model */
#define CDC_ATMCM_SUBCLASS    0x07 /* ATM Networking Control Model */
#define CDC_WHCM_SUBCLASS     0x08 /* Wireless Handset Control Model */
#define CDC_DEVMANGM_SUBCLASS 0x09 /* Device Management Model */
#define CDC_MDLM_SUBCLASS     0x0a /* Mobile Direct Line Model */
#define CDC_OBEX_SUBCLASS     0x0b /* OBEX Model */

/* CDC Control Interface protocols */
#define CDC_PROTO_USB         0x00 /* no class specific protocol */
#define CDC_PROTO_HAYES       0x01 /* common AT commands */
#define CDC_PROTO_PCCA101     0x02 /* AT Commands defined by [PCCA101] */
#define CDC_PROTO_PCCA101O    0x03 /* AT Commands defined by [PCCA101] + [PCCA101-O] */
#define CDC_PROTO_GSM0707     0x04 /* AT Commands defined by [GSM07.07] */
#define CDC_PROTO_3GPP27007   0x05 /* AT Commands defined by [3GPP27.007] */
#define CDC_PROTO_CS00170     0x06 /* AT Commands defined by [C-S0017-0] */
#define CDC_PROTO_EXTERNAL    0xfe /* External Protocol: Commands defined by Command Set functional descriptor */

/* CDC Data Interface protocols */
#define CDC_PROTO_ISDNBRI     0x30 /* I.430 Physical interface protocol for ISDN BRI */
#define CDC_PROTO_HDLC        0x31 /* ISO/IEC 3309-1993 HDLC */
#define CDC_PROTO_TRANSP      0x32 /* Transparent */
#define CDC_PROTO_Q921M       0x50 /* Management protocol for Q.921 data link protocol */
#define CDC_PROTO_Q921D       0x51 /* Data link protocol for Q.931 */
#define CDC_PROTO_Q921TM      0x52 /* TEI-multiplexor for Q.921 data link protocol */
#define CDC_PROTO_V42BIS      0x90 /* V.42bis Data compression procedures */
#define CDC_PROTO_EUROISDN    0x91 /* Q.931 Euro ISDN */
#define CDC_PROTO_V120        0x92 /* V.24 rate adaption to ISDN */
#define CDC_PROTO_CAPI20      0x93 /* CAPI Commands */
#define CDC_PROTO_HOST        0xFD /* host based driver */
#define CDC_PROTO_CDC         0xFE /* protocol described using protocol unit descriptors */

/* Usb CDC Class specific stuff */

/* actual descriptors */

struct UsbCDCHeaderDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_HEADER) */
    UBYTE bcdCDC0;             /* BCD Version lowbyte */
    UBYTE bcdCDC1;             /* BCD Version highbyte */
};

struct UsbCDCCallMgmDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_CALLMGM) */
    UBYTE bmCapabilities;      /* Capabilities (bit 0 handles cm, bit 1 data interface) */
    UBYTE bDataInterface;      /* interface number for call management */
};

struct UsbCDCACMDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_ACM) */
    UBYTE bmCapabilities;      /* Capabilities */
};

struct UsbCDCDLDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_DirectLine) */
    UBYTE bmCapabilities;      /* Capabilities */
};

struct UsbCDCRingerDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_PHONERING) */
    UBYTE bRingerVolSteps;     /* Volume scale */
    UBYTE bNumRingerPatterns;  /* Number of ring patterns supported */
};

struct UsbCDCPhoneOpDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_PHONEOP) */
    UBYTE bmCapabilities;      /* Capabilities */
};

struct UsbCDCPhoneCallDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_PHONECALL) */
    ULONG bmCapabilities;      /* Capabilities */
};

struct UsbCDCUnionDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_UNION) */
    UBYTE bMasterInterface;    /* interface number of master interface */
    UBYTE bSlaveInterface0;    /* interface number of the first slave */
};

struct UsbCDCCountryDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_COUNTRY) */
    UBYTE iCountryCodeRelDate; /* Index of a release date string */
    UWORD wCountryCode0;       /* Country code in hex */
};

struct UsbCDCTerminalDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_TERMINAL) */
    UBYTE bEntityId;           /* constant identifying the terminal */
    UBYTE iName;               /* string index */
    UBYTE bChannelIndex;       /* channel index of associated network channel */
    UBYTE bPhysicalInterface;  /* type of physical interface (0=None, 1=ISDN) */
};

struct UsbCDCProtocolDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_PROTOCOL) */
    UBYTE bEntityId;           /* constant identifying the unit */
    UBYTE bProtocol;           /* Protocol code */
    UBYTE bChildId0;           /* first ID of lower terminal or unit */
};

struct UsbCDCExtensionDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_EXTENSION) */
    UBYTE bEntityId;           /* constant identifying the unit */
    UBYTE bExtensionCode;      /* vendor specific code */
    UBYTE iName;               /* string index */
    UBYTE bChildId0;           /* first ID of lower terminal or unit */
};

struct UsbCDCMCMDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_MCM) */
    UBYTE bmCapabilities;      /* Capabilities */
};

struct UsbCDCCAPICtrlDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_CAPICTRL) */
    UBYTE bmCapabilities;      /* Capabilities */
};

struct UsbCDCEthernetDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_ETHERNET) */
    UBYTE iMACAddress;         /* index of string descriptor with MAC */
    ULONG bmEthernetStatistics; /* bitmap of statistics supported */
    UWORD wMaxSegmentSize;     /* MTU */
    UWORD wNumberMCFilters;    /* Number of multicast filters */
    UBYTE bNumberPowerFilters; /* Number of pattern filters for wake-up */
};

struct UsbCDCATMDesc
{
    UBYTE bFunctionLength;     /* total size of the descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type */
    UBYTE bDescriptorSubtype;  /* Descriptor Subtype (CDC_FD_ATM) */
    UBYTE iEndSystemIdentifier; /* string index */
    UBYTE bmDataCapabilities;  /* Capabilities */
    UBYTE bmATMDeviceStatistics; /* bitmap of statistics supported */
    UWORD wType2MaxSegmentSize; /* MTU of Type 2 segments */
    UWORD wType3MaxSegmentSize; /* MTU of Type 3 segments */
    UWORD wMaxVC;              /* max number of virtual circuits */
};

/* CDC ACM specific stuff */

struct UsbCDCLineCoding
{
    ULONG dwDTERate;           /* Baud rate */
    UBYTE bCharFormat;         /* Stop bits 0->1, 1->1.5, 2->2 */
    UBYTE bParityType;         /* Party (0=None, 1=Odd, 2=Even, 3=Mark, 4=Space) */
    UBYTE bDataBits;           /* Databits (5,6,7,8 or 16) */
};

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* DEVICES_USB_CDC_H */
