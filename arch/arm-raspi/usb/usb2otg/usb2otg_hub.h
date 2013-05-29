/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <devices/usb_hub.h>

/* Root hub data */
const struct UsbStdDevDesc OTGRootHubDevDesc = {
    sizeof(struct UsbStdDevDesc),
    UDT_DEVICE,
    AROS_WORD2LE(0x0200),
    HUB_CLASSCODE,
    0,
    1,
    64,
    AROS_WORD2LE(0x0000),
    AROS_WORD2LE(0x0000),
    AROS_WORD2LE(0x0100),
    1,
    2,
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
        AROS_WORD2LE(sizeof(struct UsbStdCfgDesc) + sizeof(struct UsbStdIfDesc) + sizeof(struct UsbStdEPDesc)),
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
        AROS_WORD2LE(8),
        255
    },
};

const struct UsbHubDesc    OTGRootHubDesc = {
    9,                                                  // 0 Number of bytes in this descriptor, including this byte
    UDT_HUB,                                            // 1 Descriptor Type, value: 29H for hub descriptor
    1,                                                  // 2 Number of downstream facing ports that this hub supports
    AROS_WORD2LE(UHCF_INDIVID_POWER|UHCF_INDIVID_OVP),  // 3 wHubCharacteristics
    50,                                                 // 5 bPwrOn2PwrGood
    1,                                                  // 6 bHubContrCurrent
    1,                                                  // 7 DeviceRemovable (size is variable)
    0                                                   // x PortPwrCtrlMask (size is variable)
};

static const CONST_STRPTR OTGRootHubStrings[] =
{
    "The AROS Dev Team",
    "OTG2USB Root Hub",
    "Standard Config",
    "Hub interface"
};
