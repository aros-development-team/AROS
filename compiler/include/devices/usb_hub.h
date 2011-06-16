#ifndef DEVICES_USB_HUB_H
#define DEVICES_USB_HUB_H
/*
**	$VER: usb_hub.h 3.0 (31.05.09)
**
**	usb definitions include file
**
**	(C) Copyright 2002-2009 Chris Hodges
**	    All Rights Reserved
*/

#include <exec/types.h>

#if defined(__GNUC__)
# pragma pack(1)
#endif

/* Usb Hub Requests */
#define UHR_GET_STATE         0x02 /* URTF_OTHER for port and URTF_DEVICE for Hub itself */
#define UHR_CLEAR_TT_BUFFER   0x08
#define UHR_RESET_TT_BUFFER   0x09
#define UHR_GET_TT_STATE      0x0a
#define UHR_STOP_TT           0x0b

/* Usb Hub Feature Selectors */
#define UFS_C_HUB_LOCAL_POWER     0x00
#define UFS_C_HUB_OVER_CURRENT    0x01
#define UFS_PORT_CONNECTION       0x00
#define UFS_PORT_ENABLE           0x01
#define UFS_PORT_SUSPEND          0x02
#define UFS_PORT_OVER_CURRENT     0x03
#define UFS_PORT_RESET            0x04
#define UFS_PORT_POWER            0x08
#define UFS_PORT_LOW_SPEED        0x09
#define UFS_C_PORT_CONNECTION     0x10
#define UFS_C_PORT_ENABLE         0x11
#define UFS_C_PORT_SUSPEND        0x12
#define UFS_C_PORT_OVER_CURRENT   0x13
#define UFS_C_PORT_RESET          0x14
#define UFS_PORT_TEST             0x15
#define UFS_PORT_INDICATOR        0x16

/* HUB class specific descriptors */
#define UDT_HUB               0x29
#define UDT_SSHUB             0x2a  /* SuperSpeed hub descriptor */

/* Usb Class Specific Descriptor: Hub Descriptor */
struct  UsbHubDesc
{
    UBYTE bLength;             /* Number of bytes in this descriptor, including this byte */
    UBYTE bDescriptorType;     /* Descriptor Type, value:  29H for hub descriptor */
    UBYTE bNbrPorts;           /* Number of downstream ports that this hub supports */
    UWORD wHubCharacteristics; /* Hub flags */
    UBYTE bPwrOn2PwrGood;      /* Time (in 2ms intervals) for power-good on port */
    UBYTE bHubContrCurrent;    /* Maximum current requirements of the Hub Controller in mA. */
    UBYTE DeviceRemovable;     /* Variable Size! Indicates if a port has a removable (0) device attached, Bit n<-> Port n */
    UBYTE PortPwrCtrlMask;     /* Variable Size! Obsolete (USB1.0) */
};

/* Usb Class Specific Descriptor: SuperSpeed Hub Descriptor */
struct  UsbSSHubDesc
{
    UBYTE bLength;             /* Number of bytes in this descriptor, including this byte */
    UBYTE bDescriptorType;     /* Descriptor Type, value:  2AH for SuperSpeed hub descriptor */
    UBYTE bNbrPorts;           /* Number of downstream ports that this hub supports */
    UWORD wHubCharacteristics; /* Hub flags */
    UBYTE bPwrOn2PwrGood;      /* Time (in 2ms intervals) for power-good on port */
    UBYTE bHubContrCurrent;    /* */
    UBYTE bHubHdrDecLat;       /* Hub Packet Header Decode Latency */
    UWORD wHubDelay;           /* */
    UWORD DeviceRemovable;     /* Indicates if a port has a removable device attached */
};

/* Flags for wHubCharacteristics */
#define UHCF_INDIVID_POWER    0x0001 /* Individual port power switching */
#define UHCF_IS_COMPOUND      0x0004 /* Hub is part of a compound device */
#define UHCF_INDIVID_OVP      0x0008 /* Individual port over-current status */
#define UHCF_NO_OVP           0x0010 /* No over-current protection */
#define UHCF_PORT_INDICATORS  0x0080 /* Port indicators are supported */

#define UHCS_THINK_TIME       13
#define UHCF_THINK_TIME_8     0x0000 /* TT Think Time 8 FS bit times */
#define UHCF_THINK_TIME_16    0x2000 /* TT Think Time 16 FS bit times */
#define UHCF_THINK_TIME_24    0x4000 /* TT Think Time 24 FS bit times */
#define UHCF_THINK_TIME_32    0x6000 /* TT Think Time 32 FS bit times */
#define UHCM_THINK_TIME       0x6000

/* Structure returned by GetHubStatus() */

struct UsbHubStatus
{
    UWORD wHubStatus;          /* Current status of hub (see below) */
    UWORD wHubChange;          /* Changes of status */
};

/* Flags for wHubStatus and wHubChange */
#define UHSF_LOCAL_POWER_LOST 0x0001
#define UHSF_OVER_CURRENT     0x0002

/* Structure returned by GetPortStatus() */
struct UsbPortStatus
{
    UWORD wPortStatus;         /* Current status of port (see below) */
    UWORD wPortChange;         /* Changes of status */
};

/* Flags for wPortStatus and wPortChange */
#define UPSF_PORT_CONNECTION  0x0001
#define UPSF_PORT_ENABLE      0x0002
#define UPSF_PORT_SUSPEND     0x0004
#define UPSF_PORT_OVER_CURRENT 0x0008
#define UPSF_PORT_RESET       0x0010
#define UPSF_PORT_POWER       0x0100
#define UPSF_PORT_LOW_SPEED   0x0200
#define UPSF_PORT_HIGH_SPEED  0x0400 /* USB 2.0 */
#define UPSF_PORT_TEST_MODE   0x0800 /* USB 2.0 */
#define UPSF_PORT_INDICATOR   0x1000 /* USB 2.0 */

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* DEVICES_USB_HUB_H */
