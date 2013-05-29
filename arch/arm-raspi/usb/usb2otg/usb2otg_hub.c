/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <asm/bcm2835.h>
#include <hardware/usb2otg.h>
#include <devices/usb_hub.h>

#include "usb2otg_intern.h"
#include "usb2otg_hub.h"

WORD FNAME_HUB(cmdControlXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    UWORD rt = ioreq->iouh_SetupData.bmRequestType;
    UWORD req = ioreq->iouh_SetupData.bRequest;
    UWORD idx = AROS_WORD2LE(ioreq->iouh_SetupData.wIndex);
    UWORD val = AROS_WORD2LE(ioreq->iouh_SetupData.wValue);
    UWORD len = AROS_WORD2LE(ioreq->iouh_SetupData.wLength);
    BOOL cmdgood;
    ULONG cnt;

#if defined(OTG_FORCEHOSTMODE)
    if (ioreq->iouh_Endpoint)
    {
        return (UHIOERR_STALL);
    }
#endif

    if (len != ioreq->iouh_Length)
    {
        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: IOReq Len mismatch! %ld != %ld\n", len, ioreq->iouh_Length));
        return (UHIOERR_STALL);
    }
    switch (rt)
    {
        case (URTF_STANDARD|URTF_DEVICE):
            switch (req)
            {
                case USR_SET_ADDRESS:
                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Set Device Address to #%ld\n", val));
                    otg_Unit->hu_HubAddr = val;
                    ioreq->iouh_Actual = len;
                    return (0);

                case USR_SET_CONFIGURATION:
                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Set Device Configuration to #%ld\n", val));
                    ioreq->iouh_Actual = len;
                    return (0);
            }
            break;

        case (URTF_IN|URTF_STANDARD|URTF_DEVICE):
            switch (req)
            {
                case USR_GET_STATUS:
                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetStatus (%ld)\n", len));
                    if (len == 2);
                    {
                        UWORD *mptr = ioreq->iouh_Data;
                        *mptr++ = AROS_WORD2LE(U_GSF_SELF_POWERED);
                        return (0);
                    }
                    break;

                case USR_GET_DESCRIPTOR:
                    switch (val >> 8)
                    {
                        case UDT_DEVICE:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetDeviceDescriptor (%ld)\n", len));
                            ioreq->iouh_Actual = (len > sizeof(struct UsbStdDevDesc)) ? sizeof(struct UsbStdDevDesc) : len;
                            CopyMem((APTR)&OTGRootHubDevDesc, ioreq->iouh_Data, ioreq->iouh_Actual);

                            return (0);

                        case UDT_CONFIGURATION:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetConfigDescriptor (%ld)\n", len));
                            ioreq->iouh_Actual = (len > sizeof(struct OTGHubCfg)) ? sizeof(struct OTGHubCfg) : len;
                            CopyMem((APTR)&OTGRootHubCfg, ioreq->iouh_Data, ioreq->iouh_Actual);

                            return (0);

                        case UDT_STRING:
                            if (val & 0xFF) /* get lang array */
                            {
                                CONST_STRPTR source = NULL;
                                UWORD *mptr = ioreq->iouh_Data;
                                UWORD slen;
                                D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetString %04lx (%ld)\n", val, len));
                                if ((val & 0xFF) > 4) /* index too high? */
                                {
                                    return (UHIOERR_STALL);
                                }

                                source = OTGRootHubStrings[(val & 0xFF) - 1];
                                slen = strlen(source);

                                if (len > 1)
                                {
                                    ioreq->iouh_Actual = 2;
                                    *mptr++ = AROS_WORD2BE((slen << 9)|UDT_STRING);
                                    /* "expand" string to utf16 */
                                    while ((ioreq->iouh_Actual + 1) < len)
                                    {
                                        *mptr++ = AROS_WORD2LE(*source++);
                                        ioreq->iouh_Actual += 2;
                                        if (!(*source))
                                        {
                                            break;
                                        }
                                    }
                                }
                            } else {
                                UWORD *mptr = ioreq->iouh_Data;
                                D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetLangArray %04lx (%ld)\n", val, len));
                                if (len > 1)
                                {
                                   ioreq->iouh_Actual = 2;
                                   mptr[0] = AROS_WORD2BE((4 << 8)|UDT_STRING);
                                   if (len > 3)
                                   {
                                      ioreq->iouh_Actual += 2;
                                      mptr[1] = AROS_WORD2LE(0x0409);
                                   }
                                }
                            }
                            return (0);

                        default:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Unsupported Descriptor %04lx\n", idx));
                            break;
                    }
                    break;

                case USR_GET_CONFIGURATION:
                    if (len == 1)
                    {
                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetConfiguration\n"));
                        ((UBYTE *) ioreq->iouh_Data)[0] = 1;
                        ioreq->iouh_Actual = len;
                        return (0);
                    }
                    break;
            }
            break;

        case (URTF_CLASS|URTF_OTHER):
            switch (req)
            {
                case USR_SET_FEATURE:
                    if ((!idx) && (idx > 1))
                    {
                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port %ld out of range\n", idx));
                        return (UHIOERR_STALL);
                    }

//                    hciport = idx - 1;

//                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Set Feature %ld maps from glob. Port %ld to local Port %ld\n", val, idx, hciport));
                    cmdgood = FALSE;
                    
//                    UWORD portreg = OHCI_PORTSTATUS + (hciport<<2);
//                    ULONG oldval = READREG32_LE(hc->hc_RegBase, portreg);

                    switch (val)
                    {
                        case UFS_PORT_ENABLE:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Enabling Port\n"));
//                            WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTENABLE);
                            cmdgood = TRUE;
                            break;

                        case UFS_PORT_SUSPEND:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Suspending Port\n"));
                            //hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND; // manually fake suspend change
//                            WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTSUSPEND);
                            cmdgood = TRUE;
                            break;

                        case UFS_PORT_RESET:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Resetting Port\n"));
                            // make sure we have at least 50ms of reset time here, as required for a root hub port
#if (0)
                            WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
                            uhwDelayMS(10, otg_Unit);
                            WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
                            uhwDelayMS(10, otg_Unit);
                            WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
                            uhwDelayMS(10, otg_Unit);
                            WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
                            uhwDelayMS(10, otg_Unit);
                            WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTRESET);
                            uhwDelayMS(15, otg_Unit);
                            oldval = READREG32_LE(hc->hc_RegBase, portreg);
#endif
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Reset release\n"));
#if (0)
                            if(oldval & OHPF_PORTRESET)
                            {
                                 uhwDelayMS(40, otg_Unit);
                                 oldval = READREG32_LE(hc->hc_RegBase, portreg);
                                 D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Reset 2nd release (%s %s)\n", oldval & OHPF_PORTRESET ? "didn't turn off" : "okay",
                                                                                  oldval & OHPF_PORTENABLE ? "enabled" : "still not enabled"));
                            }
                            // make enumeration possible
                            otg_Unit->hu_DevControllers[0] = hc;
#endif
                            cmdgood = TRUE;
                            break;

                        case UFS_PORT_POWER:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Powering Port\n"));
//                            WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_PORTPOWER);
                            cmdgood = TRUE;
                            break;

                        /*
                            case UFS_PORT_LOW_SPEED:
                            case UFS_C_PORT_CONNECTION:
                            case UFS_C_PORT_OVER_CURRENT:
                        */
                        default:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_SET_FEATURE - Unhandled feature #%d\n", val));
                            break;
                    }
                    if (cmdgood)
                    {
                        return (0);
                    }

                    break;

                case USR_CLEAR_FEATURE:
                    if ((!idx) && (idx > 1))
                    {
                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port %ld out of range\n", idx));
                        return (UHIOERR_STALL);
                    }
//                    hciport = idx - 1;

//                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Clear Feature %ld maps from glob. Port %ld to local Port %ld\n", val, idx, hciport));
                    cmdgood = FALSE;

#if (0)
                    UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                    ULONG oldval = READIO16_LE(hc->hc_RegBase, portreg) & ~(UHPF_ENABLECHANGE|UHPF_CONNECTCHANGE); // these are clear-on-write!
                    ULONG newval = oldval;
#endif
                    switch (val)
                    {
                        case UFS_PORT_ENABLE:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port Enable\n"));
#if (0)
                            newval &= ~UHPF_PORTENABLE;
                            // disable enumeration
                            otg_Unit->hu_DevControllers[0] = NULL;
#endif
                            cmdgood = TRUE;
                            break;

                        case UFS_PORT_SUSPEND:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port Suspend\n"));
#if (0)
                            newval &= ~UHPF_PORTSUSPEND;
#endif
                            cmdgood = TRUE;
                            break;

                        case UFS_PORT_POWER:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port Power\n"));
#if (0)
                            newval &= ~UHPF_PORTENABLE;
#endif
                            cmdgood = TRUE;
                            break;

                        case UFS_C_PORT_CONNECTION:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port Connect Change\n"));
#if (0)
                            newval |= UHPF_CONNECTCHANGE; // clear-on-write!
                            hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_CONNECTION;
#endif
                            cmdgood = TRUE;
                            break;

                        case UFS_C_PORT_ENABLE:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port Enable Change\n"));
#if (0)
                            newval |= UHPF_ENABLECHANGE; // clear-on-write!
                            hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_ENABLE;
#endif
                            cmdgood = TRUE;
                            break;

                        case UFS_C_PORT_SUSPEND:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port Suspend Change\n"));
#if (0)
                            hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_SUSPEND; // manually fake suspend change clearing
#endif
                            cmdgood = TRUE;
                            break;

                        case UFS_C_PORT_OVER_CURRENT:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port Over-Current Change\n"));
#if (0)
                            hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_OVER_CURRENT; // manually fake over current clearing
#endif
                            cmdgood = TRUE;
                            break;

                        case UFS_C_PORT_RESET:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port Reset Change\n"));
#if (0)
                            hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_RESET; // manually fake reset change clearing
#endif
                            cmdgood = TRUE;
                            break;
                        default:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE - Unhandled feature #%d\n", val));
                            break;
                    }
                    if (cmdgood)
                    {
//                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port %ld CLEAR_FEATURE %04lx->%04lx\n", idx, oldval, newval));
#if (0)
                        WRITEIO16_LE(hc->hc_RegBase, portreg, newval);
                        if(hc->hc_PortChangeMap[hciport])
                        {
                            otg_Unit->hu_RootPortChanges |= 1UL<<idx;
                        } else {
                            otg_Unit->hu_RootPortChanges &= ~(1UL<<idx);
                        }
#endif
                        return (0);
                    }
                    break;

            }
            break;

        case (URTF_IN|URTF_CLASS|URTF_OTHER):
            switch (req)
            {
                case USR_GET_STATUS:
                {
                    UWORD *mptr = ioreq->iouh_Data;
                    if (len != sizeof(struct UsbPortStatus))
                    {
                        return (UHIOERR_STALL);
                    }
                    if ((!idx) && (idx > 1))
                    {
                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port %ld out of range\n", idx));
                        return (UHIOERR_STALL);
                    }
//                    hciport = idx - 1;

#if (0)
                    UWORD portreg = hciport ? UHCI_PORT2STSCTRL : UHCI_PORT1STSCTRL;
                    UWORD oldval = READIO16_LE(hc->hc_RegBase, portreg);
                    *mptr = AROS_WORD2LE(UPSF_PORT_POWER);
                    if(oldval & UHPF_PORTCONNECTED) *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
                    if(oldval & UHPF_PORTENABLE) *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
                    if(oldval & UHPF_LOWSPEED) *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
                    if(oldval & UHPF_PORTRESET) *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
                    if(oldval & UHPF_PORTSUSPEND) *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);

                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port %ld is %s\n", idx, oldval & UHPF_LOWSPEED ? "LOWSPEED" : "FULLSPEED"));
                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port %ld Status %08lx\n", idx, *mptr));

                    mptr++;
                    if(oldval & UHPF_ENABLECHANGE)
                    {
                        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
                    }
                    if(oldval & UHPF_CONNECTCHANGE)
                    {
                        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                    }
                    if(oldval & UHPF_RESUMEDTX)
                    {
                        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE;
                    }
                    *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);
                    WRITEIO16_LE(hc->hc_RegBase, portreg, oldval);
                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port %ld Change %08lx\n", idx, *mptr));
#endif
                    return (0);
                }

            }
            break;

        case (URTF_IN|URTF_CLASS|URTF_DEVICE):
            switch (req)
            {
                case USR_GET_STATUS:
                {
                    UWORD *mptr = ioreq->iouh_Data;

                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetDCStatus (%ld)\n", len));

                    if (len < sizeof(struct UsbHubStatus))
                    {
                        return(UHIOERR_STALL);
                    }

                    *mptr++ = 0;
                    *mptr++ = 0;
                    ioreq->iouh_Actual = 4;
                    return (0);
                }

                case USR_GET_DESCRIPTOR:
                    switch (val >> 8)
                    {
                        case UDT_HUB:
                        {
                            ULONG hubdesclen = 9;
                            ULONG powergood = 1;

                            struct UsbHubDesc *uhd = (struct UsbHubDesc *) ioreq->iouh_Data;
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetHubDescriptor (%ld)\n", len));

                            ioreq->iouh_Actual = (len > hubdesclen) ? hubdesclen : len;
                            CopyMem((APTR)&OTGRootHubDesc, ioreq->iouh_Data, ioreq->iouh_Actual);

                            if (ioreq->iouh_Length)
                            {
                                uhd->bLength = hubdesclen;
                            }

                            if (ioreq->iouh_Length >= hubdesclen)
                            {
                                if (hubdesclen == 9)
                                {
                                    uhd->DeviceRemovable = 0;
                                    uhd->PortPwrCtrlMask = (1 << 3) - 2;
                                } else {
                                    // each field is 16 bits wide
                                    uhd->DeviceRemovable = 0;
                                    uhd->PortPwrCtrlMask = 0;
                                    ((UBYTE *)ioreq->iouh_Data)[9] = (1 << 3) - 2;
                                    ((UBYTE *)ioreq->iouh_Data)[10] = ((1 << 3) - 2) >> 8;
                                }
                            }
                            return (0);
                        }

                        default:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Unsupported Descriptor %04lx\n", idx));
                            break;
                    }
                    break;
            }

    }

    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Unsupported command %02lx %02lx %04lx %04lx %04lx!\n", rt, req, idx, val, len));

    return (UHIOERR_STALL);
}
