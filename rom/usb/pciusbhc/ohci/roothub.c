/*
   Copyright © 2002-2009, Chris Hodges. All rights reserved.
   Copyright © 2009-2012, The AROS Development Team. All rights reserved.
   $Id$
*/

#include <devices/usb_hub.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include <strings.h>

#include "debug.h"
#include "chip.h"
#include "pci.h"

#include "cmd_protos.h"
#include "chip_protos.h"

/* we cannot use AROS_WORD2LE in struct initializer */
#if AROS_BIG_ENDIAN
#define WORD2LE(w) (UWORD)(((w) >> 8) & 0x00FF) | (((w) << 8) & 0xFF00)
#else
#define WORD2LE(w) (w)
#endif

/* Root hub data */
static const struct UsbStdDevDesc RHDevDesc =
{
    sizeof(struct UsbStdDevDesc), UDT_DEVICE, WORD2LE(0x0110),
    HUB_CLASSCODE, 0, 0, 8, WORD2LE(0x0000), WORD2LE(0x0000),
    WORD2LE(0x0100), 1, 2, 0, 1
};

static const struct UsbStdCfgDesc RHCfgDesc =
{
    9, UDT_CONFIGURATION, WORD2LE(9 + 9 + 7), 1, 1, 3,
    USCAF_ONE | USCAF_SELF_POWERED, 0
};
static const struct UsbStdIfDesc RHIfDesc =
    {9, UDT_INTERFACE, 0, 0, 1, HUB_CLASSCODE, 0, 0, 4};
static const struct UsbStdEPDesc RHEPDesc =
    {7, UDT_ENDPOINT, URTF_IN | 1, USEAF_INTERRUPT, WORD2LE(8), 255};
static const struct UsbHubDesc RHHubDesc =
{
    9,
    UDT_HUB,
    0,
    WORD2LE(UHCF_INDIVID_POWER | UHCF_INDIVID_OVP),
    0,
    1,
    1,
    0
};

static const CONST_STRPTR RHStrings[] =
{
    "Chris Hodges", "PCI Root Hub Unit x", "Standard Config",
    "Hub interface"
};

/* /// "cmdControlXFerRootHub()" */
WORD cmdControlXFerRootHub(struct IOUsbHWReq *ioreq,
    struct PCIUnit *unit, struct PCIDevice *base)
{
    struct PCIController *hc;
    struct PCIController *chc;
    UWORD rt = ioreq->iouh_SetupData.bmRequestType;
    UWORD req = ioreq->iouh_SetupData.bRequest;
    UWORD idx = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    UWORD val = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD len = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);
    UWORD hciport, i;
    ULONG numports = unit->hu_RootHubPorts, reg_val, flag;
    BOOL cmdgood;

    if (ioreq->iouh_Endpoint)
    {
        return UHIOERR_STALL;
    }

    if (len != ioreq->iouh_Length)
    {
        KPRINTF(20, ("RH: Len (%ld != %ld) mismatch!\n",
                len != ioreq->iouh_Length));
        return UHIOERR_STALL;
    }
    switch (rt)
    {
    case (URTF_STANDARD | URTF_DEVICE):
        switch (req)
        {
        case USR_SET_ADDRESS:
            KPRINTF(1, ("RH: SetAddress = %ld\n", val));
            unit->hu_RootHubAddr = val;
            ioreq->iouh_Actual = len;
            return 0;

        case USR_SET_CONFIGURATION:
            KPRINTF(1, ("RH: SetConfiguration=%ld\n", val));
            ioreq->iouh_Actual = len;
            return 0;
        }
        break;

    case (URTF_IN | URTF_STANDARD | URTF_DEVICE):
        switch (req)
        {
        case USR_GET_STATUS:
            {
                UWORD *mptr = ioreq->iouh_Data;
                if (len != sizeof(struct UsbPortStatus))
                {
                    return UHIOERR_STALL;
                }
                if ((!idx) && (idx > numports))
                {
                    KPRINTF(20, ("Port %ld out of range\n", idx));
                    return UHIOERR_STALL;
                }
                hc = unit->hu_PortMap11[idx - 1];
                hciport = unit->hu_PortNum11[idx - 1];
                {
                    UWORD portreg = OHCI_PORTSTATUS + (hciport << 2);
                    ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg);

                    *mptr = AROS_WORD2LE(TranslatePortFlags(oldval,
                            OHPF_PORTPOWER | OHPF_OVERCURRENT
                            | OHPF_PORTCONNECTED | OHPF_PORTENABLE
                            | OHPF_LOWSPEED | OHPF_PORTRESET
                            | OHPF_PORTSUSPEND));

                    KPRINTF(5, ("OHCI Port %ld (glob. %ld) is %s\n",
                            hciport, idx,
                            oldval & OHPF_LOWSPEED ? "LOWSPEED" :
                            "FULLSPEED"));
                    KPRINTF(5, ("OHCI Port %ld Status %08lx (%08lx)\n", idx,
                            *mptr, oldval));

                    mptr++;
                    hc->hc_PortChangeMap[hciport] |=
                        TranslatePortFlags(oldval,
                        OHPF_OVERCURRENTCHG | OHPF_RESETCHANGE |
                        OHPF_ENABLECHANGE | OHPF_CONNECTCHANGE |
                        OHPF_RESUMEDTX);
                    *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);
                    KPRINTF(5, ("OHCI Port %ld Change %08lx\n", idx,
                            *mptr));
                    return 0;
                }

                return 0;
            }

        case USR_GET_DESCRIPTOR:
            switch (val >> 8)
            {
            case UDT_DEVICE:
                KPRINTF(1, ("RH: GetDeviceDescriptor (%ld)\n", len));
                ioreq->iouh_Actual =
                    (len >
                    sizeof(struct UsbStdDevDesc)) ? sizeof(struct
                    UsbStdDevDesc) : len;
                CopyMem((APTR) & RHDevDesc, ioreq->iouh_Data,
                    ioreq->iouh_Actual);
                if (ioreq->iouh_Length >= sizeof(struct UsbStdDevDesc))
                {
                    if (unit->hu_RootHub20Ports)
                    {
                        struct UsbStdDevDesc *usdd =
                            (struct UsbStdDevDesc *)ioreq->iouh_Data;

                        // signal a highspeed root hub
                        usdd->bcdUSB = AROS_WORD2LE(0x0200);

                        usdd->bDeviceProtocol = 1;      // single TT
                    }
                }
                return 0;

            case UDT_CONFIGURATION:
                {
                    UBYTE tmpbuf[9 + 9 + 7];
                    KPRINTF(1, ("RH: GetConfigDescriptor (%ld)\n", len));
                    CopyMem((APTR) & RHCfgDesc, tmpbuf, 9);
                    CopyMem((APTR) & RHIfDesc, &tmpbuf[9], 9);
                    CopyMem((APTR) & RHEPDesc, &tmpbuf[9 + 9], 7);
                    if (unit->hu_RootHub20Ports)
                    {
                        struct UsbStdEPDesc *usepd =
                            (struct UsbStdEPDesc *)&tmpbuf[9 + 9];
                        usepd->bInterval = 12;  // 2048 ÂµFrames
                    }
                    ioreq->iouh_Actual =
                        (len > 9 + 9 + 7) ? 9 + 9 + 7 : len;
                    CopyMem(tmpbuf, ioreq->iouh_Data, ioreq->iouh_Actual);
                    return 0;
                }

            case UDT_STRING:
                if (val & 0xff) /* get lang array */
                {
                    CONST_STRPTR source = NULL;
                    UWORD *mptr = ioreq->iouh_Data;
                    UWORD slen = 1;
                    KPRINTF(1, ("RH: GetString %04lx (%ld)\n", val, len));
                    if ((val & 0xff) > 4)       /* index too high? */
                    {
                        return UHIOERR_STALL;
                    }
                    source = RHStrings[(val & 0xff) - 1];
                    if (len > 1)
                    {
                        ioreq->iouh_Actual = 2;
                        while (*source++)
                        {
                            slen++;
                        }
                        source = RHStrings[(val & 0xff) - 1];
                        *mptr++ = AROS_WORD2BE((slen << 9) | UDT_STRING);
                        while (ioreq->iouh_Actual + 1 < len)
                        {
                            // special hack for unit number in root hub string
                            if (((val & 0xff) == 2) && (source[1] == 0))
                            {
                                *mptr++ =
                                    AROS_WORD2LE('0' + unit->hu_UnitNo);
                            }
                            else
                            {
                                *mptr++ = AROS_WORD2LE(*source);
                            }
                            source++;
                            ioreq->iouh_Actual += 2;
                            if (!(*source))
                            {
                                break;
                            }
                        }
                    }
                }
                else
                {
                    UWORD *mptr = ioreq->iouh_Data;
                    KPRINTF(1, ("RH: GetLangArray %04lx (%ld)\n", val,
                            len));
                    if (len > 1)
                    {
                        ioreq->iouh_Actual = 2;
                        mptr[0] = AROS_WORD2BE((4 << 8) | UDT_STRING);
                        if (len > 3)
                        {
                            ioreq->iouh_Actual += 2;
                            mptr[1] = AROS_WORD2LE(0x0409);
                        }
                    }
                }
                return 0;

            default:
                KPRINTF(1, ("RH: Unsupported Descriptor %04lx\n", idx));
            }
            break;

        case USR_GET_CONFIGURATION:
            if (len == 1)
            {
                KPRINTF(1, ("RH: GetConfiguration\n"));
                ((UBYTE *) ioreq->iouh_Data)[0] = 1;
                ioreq->iouh_Actual = len;
                return 0;
            }
            break;
        }
        break;

    case (URTF_CLASS | URTF_OTHER):
        switch (req)
        {
        case USR_SET_FEATURE:
            if ((!idx) && (idx > numports))
            {
                KPRINTF(20, ("Port %ld out of range\n", idx));
                return UHIOERR_STALL;
            }
            chc = unit->hu_PortMap11[idx - 1];
            hc = chc;
            hciport = unit->hu_PortNum11[idx - 1];
            KPRINTF(10,
                ("Set Feature %ld maps from global Port %ld "
                    "to local Port %ld\n",
                    val, idx, hciport));
            cmdgood = TRUE;
            UWORD portreg = OHCI_PORTSTATUS + (hciport << 2);
            if (val == UFS_PORT_RESET)
            {
                KPRINTF(10, ("Resetting Port (%s)\n",
                        READREG32_LE(hc->hc_RegBase,
                            portreg) & OHPF_PORTRESET ? "already" : "ok"));
                // make sure we have at least 50ms of reset time here,
                // as required for a root hub port
                for (i = 0; i < 5; i++)
                {
                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
                    DelayMS(10, unit);
                }
                DelayMS(5, unit);

                ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg);
                KPRINTF(10, ("OHCI Reset release (%s %s)\n",
                        oldval & OHPF_PORTRESET ? "didn't turn off" :
                        "okay",
                        oldval & OHPF_PORTENABLE ? "enabled" :
                        "not enabled"));
                if (oldval & OHPF_PORTRESET)
                {
                    DelayMS(40, unit);
                    oldval = READREG32_LE(hc->hc_RegBase, portreg);
                    KPRINTF(10, ("OHCI Reset 2nd release (%s %s)\n",
                            oldval & OHPF_PORTRESET ? "didn't turn off" :
                            "okay",
                            oldval & OHPF_PORTENABLE ? "enabled" :
                            "still not enabled"));
                }
                // make enumeration possible
                unit->hu_DevControllers[0] = hc;
                return 0;
            }
            else
            {
                switch (val)
                {
                    /* case UFS_PORT_CONNECTION: not possible */
                case UFS_PORT_ENABLE:
                    KPRINTF(10, ("Enabling Port (%s)\n",
                            oldval & OHPF_PORTENABLE ? "already" : "ok"));
                    reg_val = OHPF_PORTENABLE;
                    break;

                case UFS_PORT_SUSPEND:
                    KPRINTF(10, ("Suspending Port (%s)\n",
                            oldval & OHPF_PORTSUSPEND ? "already" : "ok"));
                    reg_val = OHPF_PORTSUSPEND;
                    break;

                    /* case UFS_PORT_OVER_CURRENT: not possible */
                case UFS_PORT_POWER:
                    KPRINTF(10, ("Powering Port (%s)\n",
                            oldval & OHPF_PORTPOWER ? "already" : "ok"));
                    reg_val = OHPF_PORTPOWER;
                    break;

                    /* case UFS_PORT_LOW_SPEED: not possible */
                    /* case UFS_C_PORT_CONNECTION:
                       case UFS_C_PORT_ENABLE:
                       case UFS_C_PORT_SUSPEND:
                       case UFS_C_PORT_OVER_CURRENT:
                       case UFS_C_PORT_RESET: */
                default:
                    cmdgood = FALSE;
                }
                if (cmdgood)
                {
                    WRITEREG32_LE(hc->hc_RegBase, portreg, reg_val);
                    return 0;
                }
                break;
            }

            break;

        case USR_CLEAR_FEATURE:
            if ((!idx) && (idx > numports))
            {
                KPRINTF(20, ("Port %ld out of range\n", idx));
                return UHIOERR_STALL;
            }
            if (unit->hu_EhciOwned[idx - 1])
            {
                hc = unit->hu_PortMap20[idx - 1];
                hciport = idx - 1;
            }
            else
            {
                hc = unit->hu_PortMap11[idx - 1];
                hciport = unit->hu_PortNum11[idx - 1];
            }
            KPRINTF(10,
                ("Clear Feature %ld maps from global Port %ld "
                    "to local Port %ld\n",
                    val, idx, hciport));
            cmdgood = TRUE;
            flag = 0;
            {
                UWORD portreg = OHCI_PORTSTATUS + (hciport << 2);
                ULONG __unused oldval =
                    READREG32_LE(hc->hc_RegBase, portreg);

                switch (val)
                {
                case UFS_PORT_ENABLE:
                    KPRINTF(10, ("Disabling Port (%s)\n",
                            oldval & OHPF_PORTENABLE ? "ok" : "already"));
                    reg_val = OHPF_PORTDISABLE;
                    break;

                case UFS_PORT_SUSPEND:
                    KPRINTF(10, ("Resuming Port (%s)\n",
                            oldval & OHPF_PORTSUSPEND ? "ok" : "already"));
                    //flag = UPSF_PORT_SUSPEND; // manually fake suspend change
                    reg_val = OHPF_RESUME;
                    break;

                case UFS_PORT_POWER:
                    KPRINTF(10, ("Unpowering Port (%s)\n",
                            oldval & OHPF_PORTPOWER ? "ok" : "already"));
                    reg_val = OHPF_PORTUNPOWER;
                    break;

                case UFS_C_PORT_CONNECTION:
                    reg_val = OHPF_CONNECTCHANGE;
                    flag = UPSF_PORT_CONNECTION;
                    break;

                case UFS_C_PORT_ENABLE:
                    reg_val = OHPF_ENABLECHANGE;
                    flag = UPSF_PORT_ENABLE;
                    break;

                case UFS_C_PORT_SUSPEND:
                    reg_val = OHPF_RESUMEDTX;
                    flag = UPSF_PORT_SUSPEND;
                    break;

                case UFS_C_PORT_OVER_CURRENT:
                    reg_val = OHPF_OVERCURRENTCHG;
                    flag = UPSF_PORT_OVER_CURRENT;
                    break;

                case UFS_C_PORT_RESET:
                    reg_val = OHPF_RESETCHANGE;
                    flag = UPSF_PORT_RESET;
                    break;
                default:
                    cmdgood = FALSE;
                }
                if (cmdgood)
                {
                    WRITEREG32_LE(hc->hc_RegBase, portreg, reg_val);
                    hc->hc_PortChangeMap[hciport] &= ~flag;
                    return 0;
                }
                break;
            }

            break;
        }
        break;

    case (URTF_IN | URTF_CLASS | URTF_OTHER):
        switch (req)
        {
        case USR_GET_STATUS:
            {
                UWORD *mptr = ioreq->iouh_Data;
                if (len != sizeof(struct UsbPortStatus))
                {
                    return UHIOERR_STALL;
                }
                if ((!idx) && (idx > numports))
                {
                    KPRINTF(20, ("Port %ld out of range\n", idx));
                    return UHIOERR_STALL;
                }
                if (unit->hu_EhciOwned[idx - 1])
                {
                    hc = unit->hu_PortMap20[idx - 1];
                    hciport = idx - 1;
                }
                else
                {
                    hc = unit->hu_PortMap11[idx - 1];
                    hciport = unit->hu_PortNum11[idx - 1];
                }
                {
                    UWORD portreg = OHCI_PORTSTATUS + (hciport << 2);
                    ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg);

                    *mptr = 0;
                    *mptr = AROS_WORD2LE(TranslatePortFlags(oldval,
                            OHPF_PORTPOWER | OHPF_OVERCURRENT
                            | OHPF_PORTCONNECTED | OHPF_PORTENABLE
                            | OHPF_LOWSPEED | OHPF_PORTRESET
                            | OHPF_PORTSUSPEND));

                    KPRINTF(5, ("OHCI Port %ld (glob. %ld) is %s\n",
                            hciport, idx,
                            oldval & OHPF_LOWSPEED ? "LOWSPEED" :
                            "FULLSPEED"));
                    KPRINTF(5, ("OHCI Port %ld Status %08lx (%08lx)\n", idx,
                            *mptr, oldval));

                    mptr++;
                    hc->hc_PortChangeMap[hciport] |=
                        TranslatePortFlags(oldval,
                        OHPF_OVERCURRENTCHG | OHPF_RESETCHANGE
                        | OHPF_ENABLECHANGE | OHPF_CONNECTCHANGE
                        | OHPF_RESUMEDTX);
                    *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);
                    KPRINTF(5, ("OHCI Port %ld Change %08lx\n", idx,
                            *mptr));
                    return 0;
                }

                return 0;
            }

        }
        break;

    case (URTF_IN | URTF_CLASS | URTF_DEVICE):
        switch (req)
        {
        case USR_GET_STATUS:
            {
                UWORD *mptr = ioreq->iouh_Data;
                if (len < sizeof(struct UsbHubStatus))
                {
                    return UHIOERR_STALL;
                }
                *mptr++ = 0;
                *mptr++ = 0;
                ioreq->iouh_Actual = 4;
                return 0;
            }

        case USR_GET_DESCRIPTOR:
            switch (val >> 8)
            {
            case UDT_HUB:
                {
                    ULONG hubdesclen = 9;
                    ULONG powergood = 1;

                    struct UsbHubDesc *uhd =
                        (struct UsbHubDesc *)ioreq->iouh_Data;
                    KPRINTF(1, ("RH: GetHubDescriptor (%ld)\n", len));

                    if (unit->hu_RootHubPorts > 7)
                    {
                        hubdesclen += 2;    // needs two bytes for port masks
                    }

                    ioreq->iouh_Actual =
                        (len > hubdesclen) ? hubdesclen : len;
                    CopyMem((APTR) & RHHubDesc, ioreq->iouh_Data,
                        ioreq->iouh_Actual);

                    if (ioreq->iouh_Length)
                    {
                        uhd->bLength = hubdesclen;
                    }

                    if (ioreq->iouh_Length >= 6)
                    {
                        hc = (struct PCIController *)unit->
                            hu_Controllers.lh_Head;
                        while (hc->hc_Node.ln_Succ)
                        {
                            {
                                ULONG localpwgood =
                                    (READREG32_LE(hc->hc_RegBase,
                                        OHCI_HUBDESCA) & OHAM_POWERGOOD) >>
                                    OHAS_POWERGOOD;
                                if (localpwgood > powergood)
                                {
                                    powergood = localpwgood;
                                    KPRINTF(10,
                                        ("Increasing power good time to %ld\n",
                                            powergood));
                                }
                            }
                            hc = (struct PCIController *)hc->
                                hc_Node.ln_Succ;
                        }

                        uhd->bPwrOn2PwrGood = powergood;
                    }
                    if (ioreq->iouh_Length >= hubdesclen)
                    {
                        uhd->bNbrPorts = unit->hu_RootHubPorts;
                        if (hubdesclen == 9)
                        {
                            uhd->DeviceRemovable = 0;
                            uhd->PortPwrCtrlMask =
                                (1 << (unit->hu_RootHubPorts + 2)) - 2;
                        }
                        else
                        {
                            // each field is now 16 bits wide
                            uhd->DeviceRemovable = 0;
                            uhd->PortPwrCtrlMask = 0;
                            ((UBYTE *) ioreq->iouh_Data)[9] =
                                (1 << (unit->hu_RootHubPorts + 2)) - 2;
                            ((UBYTE *) ioreq->iouh_Data)[10] =
                                ((1 << (unit->hu_RootHubPorts + 2)) -
                                2) >> 8;
                        }
                    }
                    return 0;
                }

            default:
                KPRINTF(20, ("RH: Unsupported Descriptor %04lx\n", idx));
            }
            break;
        }

    }
    KPRINTF(20, ("RH: Unsupported command %02x %02x %04x %04x %04x!\n",
            (UWORD) rt, (UWORD) req, idx, val, len));
    return UHIOERR_STALL;
}
/* \\\ */

/* /// "cmdIntXFerRootHub()" */
WORD cmdIntXFerRootHub(struct IOUsbHWReq * ioreq,
    struct PCIUnit * unit, struct PCIDevice * base)
{
    if ((ioreq->iouh_Endpoint != 1) || (!ioreq->iouh_Length))
    {
        return UHIOERR_STALL;
    }

    if (unit->hu_RootPortChanges)
    {
        KPRINTF(1, ("Immediate Portchange map %04lx\n",
                unit->hu_RootPortChanges));
        if ((unit->hu_RootHubPorts < 8) || (ioreq->iouh_Length == 1))
        {
            *((UBYTE *) ioreq->iouh_Data) = unit->hu_RootPortChanges;
            ioreq->iouh_Actual = 1;
        }
        else
        {
            ((UBYTE *) ioreq->iouh_Data)[0] = unit->hu_RootPortChanges;
            ((UBYTE *) ioreq->iouh_Data)[1] = unit->hu_RootPortChanges >> 8;
            ioreq->iouh_Actual = 2;
        }
        unit->hu_RootPortChanges = 0;
        return 0;
    }
    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    Disable();
    AddTail(&unit->hu_RHIOQueue, (struct Node *)ioreq);
    Enable();
    return RC_DONTREPLY;
}
/* \\\ */

/* /// "CheckRootHubChanges()" */
void CheckRootHubChanges(struct PCIUnit *unit)
{
    struct IOUsbHWReq *ioreq;

    if (unit->hu_RootPortChanges && unit->hu_RHIOQueue.lh_Head->ln_Succ)
    {
        KPRINTF(1, ("Portchange map %04lx\n", unit->hu_RootPortChanges));
        Disable();
        ioreq = (struct IOUsbHWReq *)unit->hu_RHIOQueue.lh_Head;
        while (((struct Node *)ioreq)->ln_Succ)
        {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            if ((unit->hu_RootHubPorts < 8) || (ioreq->iouh_Length == 1))
            {
                *((UBYTE *) ioreq->iouh_Data) = unit->hu_RootPortChanges;
                ioreq->iouh_Actual = 1;
            }
            else
            {
                ((UBYTE *) ioreq->iouh_Data)[0] = unit->hu_RootPortChanges;
                ((UBYTE *) ioreq->iouh_Data)[1] =
                    unit->hu_RootPortChanges >> 8;
                ioreq->iouh_Actual = 2;
            }
            ReplyMsg(&ioreq->iouh_Req.io_Message);
            ioreq = (struct IOUsbHWReq *)unit->hu_RHIOQueue.lh_Head;
        }
        unit->hu_RootPortChanges = 0;
        Enable();
    }
}
/* \\\ */
