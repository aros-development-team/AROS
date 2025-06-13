/*
    Copyright (C) 2013, The AROS Development Team. All rights reserved.
*/

#include <devices/usb_hub.h>

#if AROS_BIG_ENDIAN
#define LE16(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define BE16(x) (x)
#else
#define LE16(x) (x)
#define BE16(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#endif

/* Root hub data */
const struct UsbStdDevDesc OTGRootHubDevDesc = {
    .bLength = sizeof(struct UsbStdDevDesc),
    .bDescriptorType    = UDT_DEVICE,
    .bcdUSB             = LE16(0x0200),
    .bDeviceClass       = HUB_CLASSCODE,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = 8,
    .idVendor           = LE16(0x1D6B),
    .idProduct          = LE16(0x0002),
    .bcdDevice          = LE16(0x0100),
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 0,
    .bNumConfigurations = 1
};

struct OTGHubCfg
{
    struct UsbStdCfgDesc HubCfgDesc;
    struct UsbStdIfDesc  HubIfDesc;
    struct UsbStdEPDesc  HubEPDesc;
} __attribute__((packed));

const struct OTGHubCfg OTGRootHubCfg =
{
    .HubCfgDesc =
    {
        .bLength             = sizeof(struct UsbStdCfgDesc),
        .bDescriptorType     = UDT_CONFIGURATION,
        .wTotalLength        = LE16(sizeof(struct OTGHubCfg)),
        .bNumInterfaces      = 1,
        .bConfigurationValue = 1,
        .iConfiguration      = 3,
        .bmAttributes        = USCAF_ONE|USCAF_SELF_POWERED,
        .bMaxPower           = 0
    },

    .HubIfDesc =
    {
        .bLength            = sizeof(struct UsbStdIfDesc),
        .bDescriptorType    = UDT_INTERFACE,
        .bInterfaceNumber   = 0,
        .bAlternateSetting  = 0,
        .bNumEndpoints      = 1,
        .bInterfaceClass    = HUB_CLASSCODE,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface         = 4
    },

    .HubEPDesc =
    {
        .bLength          = sizeof(struct UsbStdEPDesc),
        .bDescriptorType  = UDT_ENDPOINT,
        .bEndpointAddress = URTF_IN|1,
        .bmAttributes     = USEAF_INTERRUPT,
        .wMaxPacketSize   = LE16(8),
        .bInterval        = 255
    },
};

const struct UsbHubDesc    OTGRootHubDesc = {
    9,                                                  // 0 Number of bytes in this descriptor, including this byte
    UDT_HUB,                                            // 1 Descriptor Type, value: 29H for hub descriptor
    1,                                                  // 2 Number of downstream facing ports that this hub supports
    0,                                                  // 3 wHubCharacteristics
    0,                                                  // 5 bPwrOn2PwrGood
    0,                                                  // 6 bHubContrCurrent
    1,                                                  // 7 DeviceRemovable (size is variable)
    0xFF                                                // x PortPwrCtrlMask (size is variable)
};

static const CONST_STRPTR OTGRootHubStrings[] =
{
    "The AROS Dev Team",         // 1 Manufacturer
    "BCM2708 OTG2USB Root Hub",  // 2 Product
    "Standard Config",           // 3 Configuration
    "Hub interface"              // 4 Interface
};
