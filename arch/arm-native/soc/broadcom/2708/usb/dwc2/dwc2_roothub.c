/*
    Copyright (C) 2013-2020, The AROS Development Team. All rights reserved.
*/

/* Driver for the virtual root hub on the Synopsis DesignWare USB Host Controller
 * This USB controller has a single port, which we manage and present to the OS
 * a virtual 1-port root hub.
 *
 * Note: This is NOT the onboard 4-port hub of the Raspberry Pi 2 and 3. That
 * is a standard USB hub that is connected to this port and is managed by the
 * generic hub driver.
 */

#define DEBUG 1

#include <string.h>
#include <devices/usb_hub.h>

#include "dwc2_intern.h"
#include "dwc2_regs.h"

#if AROS_BIG_ENDIAN
#define LE16(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define BE16(x) (x)
#else
#define LE16(x) (x)
#define BE16(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#endif

static const char *const stringTable[] =
{
    "Synopsys",                   // 1 - Manufacturer
    "DWC2 USB 2.0 OTG Root Hub",  // 2 - Product
    "Standard Config",            // 3 - Configuration
    "Hub Interface",              // 4 - Interface
};

// Device descriptor
static const struct UsbStdDevDesc rootHubDevDesc =
{
    .bLength = sizeof(struct UsbStdDevDesc),
    .bDescriptorType    = UDT_DEVICE,
    .bcdUSB             = LE16(0x0200),
    .bDeviceClass       = HUB_CLASSCODE,
    .bDeviceSubClass    = 0,
//#if ENABLE_HIGH_SPEED
#if 0  // can't use high speed for enumeration?
    .bDeviceProtocol    = 1,
#else
    .bDeviceProtocol    = 0,
#endif
    .bMaxPacketSize0    = 8,
    .idVendor           = LE16(0x1D6B),  // stolen from Linux's root hub ID
    .idProduct          = LE16(0x0002),  // stolen from Linux's root hub ID
    .bcdDevice          = LE16(0x0100),
    .iManufacturer      = 1,  // 1-based index in string table (0 if none)
    .iProduct           = 2,  // 1-based index in string table (0 if none)
    .iSerialNumber      = 0,  // 1-based index in string table (0 if none)
    .bNumConfigurations = 1
};

struct HubConfig
{
    struct UsbStdCfgDesc HubCfgDesc;
    struct UsbStdIfDesc  HubIfDesc;
    struct UsbStdEPDesc  HubEPDesc;
} __attribute__((packed));

// Config descriptor
const struct HubConfig rootHubConfig =
{
    .HubCfgDesc =
    {
        .bLength             = sizeof(struct UsbStdCfgDesc),
        .bDescriptorType     = UDT_CONFIGURATION,
        .wTotalLength        = LE16(sizeof(rootHubConfig)),
        .bNumInterfaces      = 1,
        .bConfigurationValue = 1,
        .iConfiguration      = 3,  // 1-based index in string table (0 if none)
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
        .iInterface         = 4  // 1-based index in string table (0 if none)
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

static const struct UsbHubDesc rootHubClassDesc =
{
    .bLength             = sizeof(rootHubClassDesc),
    .bDescriptorType     = UDT_HUB,
    .bNbrPorts           = 1,
    .wHubCharacteristics = 0,
    .bPwrOn2PwrGood      = 0,
    .bHubContrCurrent    = 0,
    .DeviceRemovable     = 1,
    .PortPwrCtrlMask     = 0xFF
};

static struct UsbHubStatus rootHubStatus = {0};

static void dwc2_roothub_suspend_port(void);
static void dwc2_roothub_resume_port(void);

// Handles a Control Transfer to the virtual root hub
WORD dwc2_roothub_cmd_controlxfer(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base)
{
    UWORD requestType = ioreq->iouh_SetupData.bmRequestType;
    UWORD request     = ioreq->iouh_SetupData.bRequest;
    UWORD index       = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    UWORD value       = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD length      = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);
    UBYTE descType;
    UBYTE descIndex;
    uint32_t hostport;

    //DWC2_BUG("Control Transfer");

    if (length != ioreq->iouh_Length)
    {
        DWC2_BUG("IOReq Len mismatch! %li vs %li", length, ioreq->iouh_Length);
        return UHIOERR_STALL;
    }

    switch (requestType)
    {
    case (URTF_OUT|URTF_STANDARD|URTF_DEVICE):  // 0x00
        switch (request)
        {
        case USR_SET_ADDRESS:
            DWC2_BUG("Set address %i", value);
            unit->hu_RootHubAddr = value;
            ioreq->iouh_Actual = length;
            return 0;
        case USR_SET_CONFIGURATION:
            DWC2_BUG("Set Configuration %i", value);
            ioreq->iouh_Actual = length;
            return 0;
        }
        break;
    case (URTF_OUT|URTF_CLASS|URTF_OTHER):  // 0x23
        switch (request)
        {
        case USR_SET_FEATURE:
            if (index != 1)  // Only port 1 is valid
            {
                DWC2_BUG("Port %i doesn't exist", index);
                return UHIOERR_STALL;
            }
            switch (value)
            {
            case UFS_PORT_ENABLE:
                // TODO: implement
                DWC2_BUG("Enable Port");
                return 0;
            case UFS_PORT_SUSPEND:
                DWC2_BUG("Suspend Port");
                dwc2_roothub_suspend_port();
                return 0;
            case UFS_PORT_RESET:
                DWC2_BUG("Reset Port");
                if (!dwc2_reset_root_port())
                    return UHIOERR_STALL;
                return 0;
            case UFS_PORT_POWER:
                DWC2_BUG("Powering On Port");
                dwc2_power_on_port();
                return 0;
            case UFS_PORT_CONNECTION:
                // This request is a no-op (per USB spec)
                DWC2_BUG("Setting Port Connection");
                return 0;
            default:
                DWC2_BUG("Set Port Feature: unknown feature %i", value);
                return UHIOERR_STALL;
            }
        case USR_CLEAR_FEATURE:
            if (index != 1)  // Only port 1 is valid
            {
                DWC2_BUG("Port %i doesn't exist", index);
                return UHIOERR_STALL;
            }
            hostport = rd32le(HPRT0) & ~HPRT0_WRITE_CLEAR_BITS;
            switch (value)
            {
            case UFS_PORT_ENABLE:
                // TODO: implement
                DWC2_BUG("Disable Port");
                return 0;
            case UFS_PORT_SUSPEND:
                DWC2_BUG("Resume Port");
                dwc2_roothub_resume_port();
                return 0;
            case UFS_PORT_RESET:
                DWC2_BUG("Un-Reset Port");
                if (!dwc2_reset_root_port())
                    return UHIOERR_STALL;
                return 0;
            case UFS_PORT_POWER:
                DWC2_BUG("Powering Off Port");
                dwc2_power_off_port();
                return 0;
            case UFS_PORT_CONNECTION:
                // This request is a no-op (per USB spec)
                DWC2_BUG("Clearing Port Connection");
                return 0;
            case UFS_C_PORT_CONNECTION:
                DWC2_BUG("Clearing Port Connection Change");
                wr32le(HPRT0, hostport | HPRT0_CONNDET);  // clear bit by writing it
                return 0;
            case UFS_C_PORT_ENABLE:
                DWC2_BUG("Clearing Port Enable Change");
                wr32le(HPRT0, hostport | HPRT0_ENACHG);  // clear bit by writing it
                return 0;
            case UFS_C_PORT_SUSPEND:
                DWC2_BUG("Clearing Port Suspend Change");
                // TODO: implement
                return 0;
            case UFS_C_PORT_OVER_CURRENT:
                DWC2_BUG("Clearing Port Overcurrent Change");
                wr32le(HPRT0, hostport | HPRT0_OVRCURRCHG);  // clear bit by writing it
                return 0;
            case UFS_C_PORT_RESET:
                DWC2_BUG("Clearing Port Reset Change");
                // TODO: implement
                return 0;
            default:
                DWC2_BUG("Clear Port Feature: unknown feature %i", value);
                return UHIOERR_STALL;
            }
        }
        break;
    case (URTF_IN|URTF_STANDARD|URTF_DEVICE):  // 0x80
        switch (request)
        {
        case USR_GET_DESCRIPTOR:
            descType = value >> 8;
            descIndex = value & 0xFF;
            switch (descType)  // descriptor type
            {
            case UDT_DEVICE:
                DWC2_BUG("Get Device Descriptor");
                ioreq->iouh_Actual = (length > sizeof(rootHubDevDesc)) ? sizeof(rootHubDevDesc) : length;
                CopyMem(&rootHubDevDesc, ioreq->iouh_Data, ioreq->iouh_Actual);
                return 0;
            case UDT_CONFIGURATION:
                DWC2_BUG("Get Configuration Descriptor");
                ioreq->iouh_Actual = (length > sizeof(rootHubConfig)) ? sizeof(rootHubConfig) : length;
                CopyMem(&rootHubConfig, ioreq->iouh_Data, ioreq->iouh_Actual);
                return 0;
            case UDT_STRING:
                if (descIndex == 0)
                {
                    WORD *mptr = ioreq->iouh_Data;
                    DWC2_BUG("Get Lang Array");
                    if (length > 1)
                    {
                        ioreq->iouh_Actual = 2;
                        mptr[0] = AROS_WORD2BE((4 << 8) | UDT_STRING);
                        if (length > 3)
                        {
                            ioreq->iouh_Actual += 2;
                            mptr[1] = AROS_WORD2LE(0x0409);
                        }
                    }
                }
                else
                {
                    WORD *mptr = ioreq->iouh_Data;
                    const char *source;
                    DWC2_BUG("Get String");
                    if (descIndex > ARRAY_LEN(stringTable))
                        return UHIOERR_STALL;  // index too high
                    source = stringTable[descIndex - 1];
                    if (length > 1)
                    {
                        ioreq->iouh_Actual = 1;
                        *mptr++ = AROS_WORD2BE(((strlen(source) + 1) << 9)|UDT_STRING);
                        /* "expand" string to utf16 */
                        while (ioreq->iouh_Actual + 1 < length)
                        {
                            *mptr++ = AROS_WORD2LE(*source++);
                            ioreq->iouh_Actual += 2;
                            if (*source == 0)
                                break;
                        }
                    }
                }
                return 0;
            default:
                DWC2_BUG("Unsupported device descriptor type 0x%X", descType);
                return UHIOERR_STALL;
            }
            break;
        case USR_GET_STATUS:
            DWC2_BUG("Get Status");
            if (length == 2)
            {
                UWORD *mptr = ioreq->iouh_Data;
                *mptr++ = AROS_WORD2LE(U_GSF_SELF_POWERED);
                return 0;
            }
        }
        break;
    case (URTF_IN|URTF_CLASS|URTF_DEVICE):  // 0xA0
        switch (request)
        {
        case USR_GET_STATUS:
            DWC2_BUG("Get Hub Status");
            if (length < sizeof(rootHubStatus))
                return UHIOERR_STALL;
            ioreq->iouh_Actual = 4;
            CopyMem(&rootHubStatus, ioreq->iouh_Data, ioreq->iouh_Actual);
            return 0;
        case USR_GET_DESCRIPTOR:
            descType = value >> 8;
            switch (descType)
            {
            case UDT_HUB:
                DWC2_BUG("Get Hub Descriptor");
                if (length != sizeof(rootHubClassDesc))
                    DWC2_BUG("hub descriptor size mismatch %i vs %i", length, sizeof(rootHubClassDesc));
                ioreq->iouh_Actual = (length > sizeof(rootHubClassDesc)) ? sizeof(rootHubClassDesc) : length;
                CopyMem(&rootHubClassDesc, ioreq->iouh_Data, ioreq->iouh_Actual);
                return 0;
            default:
                DWC2_BUG("Unsupported class descriptor type 0x%X", descType);
                return UHIOERR_STALL;
            }
        }
        break;
    case (URTF_IN|URTF_CLASS|URTF_OTHER):  // 0xA3
        switch (request)
        {
        case USR_GET_STATUS:
            {
                struct UsbPortStatus *status = (struct UsbPortStatus *)ioreq->iouh_Data;

                DWC2_BUG("Get Port Status");
                if (length != sizeof(struct UsbPortStatus))
                    return UHIOERR_STALL;
                if (index != 1)  // Only port 1 is valid
                    return UHIOERR_STALL;
                status->wPortStatus = 0;
                uint32_t hostport = rd32le(HPRT0);
                DWC2_BUG("HPRT0 reg: %08lX", hostport);

                if (hostport & HPRT0_PWR)     status->wPortStatus |= UPSF_PORT_POWER;
                if (hostport & HPRT0_ENA)     status->wPortStatus |= UPSF_PORT_ENABLE;
                if (hostport & HPRT0_CONNSTS) status->wPortStatus |= UPSF_PORT_CONNECTION;
                if (hostport & HPRT0_RST)     status->wPortStatus |= UPSF_PORT_RESET;
                if (hostport & HPRT0_SUSP)    status->wPortStatus |= UPSF_PORT_SUSPEND;
                switch ((hostport & HPRT0_SPD_MASK) >> HPRT0_SPD_SHIFT)
                {
                case HPRT0_SPD_HIGH_SPEED:
                    status->wPortStatus |= UPSF_PORT_HIGH_SPEED;
                    break;
                case HPRT0_SPD_LOW_SPEED:
                    status->wPortStatus |= UPSF_PORT_LOW_SPEED;
                    break;
                }
                status->wPortStatus = AROS_WORD2LE(status->wPortStatus);

                status->wPortChange = 0;
                if (hostport & HPRT0_ENACHG)  status->wPortChange |= UPSF_PORT_ENABLE;
                if (hostport & HPRT0_CONNDET) status->wPortChange |= UPSF_PORT_CONNECTION;
                if (hostport & HPRT0_RES)     status->wPortChange |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
                status->wPortChange = AROS_WORD2LE(status->wPortChange);
                DWC2_BUG("port status: %08lX, change: %08lX", status->wPortStatus, status->wPortChange);

                return 0;
            }
        }
    }

    DWC2_BUG("Unsupported request (type 0x%X, request 0x%X)", requestType, request);
    return UHIOERR_STALL;
}

static void dwc2_roothub_suspend_port(void)
{
    // TODO: implement
}

static void dwc2_roothub_resume_port(void)
{
    // TODO: implement
}
