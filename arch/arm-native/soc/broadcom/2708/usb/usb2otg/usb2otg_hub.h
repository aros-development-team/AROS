/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
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
    sizeof(struct UsbStdDevDesc),
    UDT_DEVICE,
    LE16(0x0200),
    HUB_CLASSCODE,
    0,
    0,
    8,
    LE16(0x0000),
    LE16(0x0000),
    LE16(0x0100),
    0,
    1,
    0,
    1
};

struct OTGHubCfg
{
    struct UsbStdCfgDesc HubCfgDesc;
    struct UsbStdIfDesc  HubIfDesc;
    struct UsbStdEPDesc  HubEPDesc;
} __attribute__((packed));

const struct OTGHubCfg OTGRootHubCfg =
{
    .HubCfgDesc = {
        sizeof(struct UsbStdCfgDesc),
        UDT_CONFIGURATION,
        LE16(sizeof(struct OTGHubCfg)),
        1,
        1,
        3,
        USCAF_ONE|USCAF_SELF_POWERED,
        0
    },

    .HubIfDesc = {
        sizeof(struct UsbStdIfDesc),
        UDT_INTERFACE,
        0,
        0,
        1,
        HUB_CLASSCODE,
        0,
        0,
        4
    },

    .HubEPDesc = {
        sizeof(struct UsbStdEPDesc),
        UDT_ENDPOINT,
        URTF_IN|1,
        USEAF_INTERRUPT,
        LE16(8),
        255
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
    "The AROS Dev Team",
    "BCM2708 OTG2USB Root Hub",
    "Standard Config",
    "Hub interface"
};
