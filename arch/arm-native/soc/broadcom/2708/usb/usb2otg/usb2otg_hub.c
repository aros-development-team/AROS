/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <devices/usb_hub.h>

#include "usb2otg_intern.h"
#include "usb2otg_hub.h"

WORD FNAME_ROOTHUB(cmdControlXFer)(struct IOUsbHWReq *ioreq,
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

    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER(%ld:%ld)\n", rt, req));

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
                
                    unsigned int otg_RegVal = *((volatile unsigned int *)USB2OTG_DEVCFG);
                    otg_RegVal &= ~(0x7F << 4);
                    otg_RegVal |= ((val & 0x7F) << 4);
                    *((volatile unsigned int *)USB2OTG_DEVCFG) = otg_RegVal;

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
                        ((UBYTE *) ioreq->iouh_Data)[0] = 1; // TODO: Expose 3 configurations? 1 = Host + Device, 2 = Host Only, 3 = Device Only?
                        ioreq->iouh_Actual = len;
                        return (0);
                    }
                    break;
            }
            break;

        case (URTF_CLASS|URTF_OTHER):
        {
            switch (req)
            {
                case USR_SET_FEATURE:
                {
                    if ((!idx) || (idx > 1))
                    {
                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port #%ld out of range\n", idx));
                        return (UHIOERR_STALL);
                    }

                    cmdgood = FALSE;

                    ULONG oldval = *((volatile unsigned int *)USB2OTG_HOSTPORT) & ~(USB2OTG_HOSTPORT_PRTENCHNG|USB2OTG_HOSTPORT_PRTCONNSTS);
                    ULONG newval = oldval;

                    switch (val)
                    {
                        case UFS_PORT_ENABLE:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Enabling Port #%ld\n", idx));

                            newval |= USB2OTG_HOSTPORT_PRTENA;
                            cmdgood = TRUE;

                            break;

                        case UFS_PORT_SUSPEND:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Suspending Port #%ld\n", idx));

                            newval |= USB2OTG_HOSTPORT_PRTSUSP;
                            cmdgood = TRUE;

                            break;

                        case UFS_PORT_RESET:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Resetting Port #%ld\n", idx));

                            newval |= USB2OTG_HOSTPORT_PRTRST;
                            cmdgood = TRUE;

                            break;

                        case UFS_PORT_POWER:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Powering Port #%ld\n", idx));

                            newval |= USB2OTG_HOSTPORT_PRTPWR;
                            cmdgood = TRUE;

                            break;

                        /*
                            case UFS_PORT_LOW_SPEED:
                            case UFS_C_PORT_CONNECTION:
                            case UFS_C_PORT_OVER_CURRENT:
                        */
                        default:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_SET_FEATURE - Unhandled feature %ld for Port #%ld\n", val, idx));
                            break;
                    }
                    if (cmdgood)
                    {
                        *((volatile unsigned int *)USB2OTG_HOSTPORT) = newval; 

                        return (0);
                    }

                    break;
                }

                case USR_CLEAR_FEATURE:
                {
                    if ((!idx) || (idx > 1))
                    {
                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port #%ld out of range\n", idx));
                        return (UHIOERR_STALL);
                    }

                    cmdgood = FALSE;

                    ULONG oldval = *((volatile unsigned int *)USB2OTG_HOSTPORT) & ~(USB2OTG_HOSTPORT_PRTENCHNG|USB2OTG_HOSTPORT_PRTCONNSTS);
                    ULONG newval = oldval;

                    switch (val)
                    {
                        case UFS_PORT_ENABLE:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Enable\n", idx));

                            newval &= ~USB2OTG_HOSTPORT_PRTENA;
                            cmdgood = TRUE;

                            break;

                        case UFS_PORT_SUSPEND:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Suspend\n", idx));

                            newval &= ~USB2OTG_HOSTPORT_PRTSUSP;
                            cmdgood = TRUE;

                            break;

                        case UFS_PORT_POWER:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Power\n", idx));

                            newval &= ~USB2OTG_HOSTPORT_PRTPWR;
                            cmdgood = TRUE;

                            break;

                        case UFS_C_PORT_CONNECTION:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Connect Change\n", idx));

                            newval &= ~USB2OTG_HOSTPORT_PRTCONNDET;
                            otg_Unit->hu_HubPortChanged = TRUE;
                            cmdgood = TRUE;

                            break;

                        case UFS_C_PORT_ENABLE:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Enable Change\n", idx));

                            newval &= ~USB2OTG_HOSTPORT_PRTENCHNG;
                            otg_Unit->hu_HubPortChanged = TRUE;
                            cmdgood = TRUE;

                            break;

                        case UFS_C_PORT_SUSPEND:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Suspend Change\n", idx));

                            newval &= ~USB2OTG_HOSTPORT_PRTRES;
                            otg_Unit->hu_HubPortChanged = TRUE;
                            cmdgood = TRUE;

                            break;

                        case UFS_C_PORT_OVER_CURRENT:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Over-Current Change\n", idx));

                            newval &= ~USB2OTG_HOSTPORT_PRTOVRCURRCHNG;
                            otg_Unit->hu_HubPortChanged = TRUE;
                            cmdgood = TRUE;

                            break;

                        case UFS_C_PORT_RESET:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE Port #%ld Reset Change\n", idx));

                            newval &= ~USB2OTG_HOSTPORT_PRTRST;
                            otg_Unit->hu_HubPortChanged = TRUE;
                            cmdgood = TRUE;

                            break;

                        default:
                            D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: USR_CLEAR_FEATURE - Unhandled feature %ld for Port #%ld\n", val, idx));
                            break;
                    }
                    if (cmdgood)
                    {
                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port #%ld CLEAR_FEATURE %04lx->%04lx\n", idx, oldval, newval));
                        *((volatile unsigned int *)USB2OTG_HOSTPORT) = newval; 

                        return (0);
                    }
                    break;
                }
            }
            break;
        }

        case (URTF_IN|URTF_CLASS|URTF_OTHER):
        {
            switch (req)
            {
                case USR_GET_STATUS:
                {
                    UWORD *mptr = ioreq->iouh_Data;

                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Get Port #%ld Status..\n", idx));

                    if (len != sizeof(struct UsbPortStatus))
                    {
                        return (UHIOERR_STALL);
                    }
                    if ((!idx) || (idx > 1))
                    {
                        D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port #%ld out of range\n", idx));
                        return (UHIOERR_STALL);
                    }

                    ULONG oldval = *((volatile unsigned int *)USB2OTG_HOSTPORT);

                    *mptr = 0;
                    if (oldval & USB2OTG_HOSTPORT_PRTPWR)        *mptr |= AROS_WORD2LE(UPSF_PORT_POWER);
                    if (oldval & USB2OTG_HOSTPORT_PRTENA)        *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
                    if (oldval & USB2OTG_HOSTPORT_PRTCONNSTS)    *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
                    if (oldval & USB2OTG_HOSTPORT_PRTSPD_LOW)    *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
                    if (oldval & USB2OTG_HOSTPORT_PRTRST)        *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
                    if (oldval & USB2OTG_HOSTPORT_PRTSUSP)       *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);

                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port #%ld is %s\n", idx, (oldval & USB2OTG_HOSTPORT_PRTSPD_LOW) ? "LOWSPEED" : "FULLSPEED"));
                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port #%ld Status %08lx\n", idx, *mptr));

                    mptr++;
                    *mptr = 0;
                    if (oldval & USB2OTG_HOSTPORT_PRTENCHNG)    *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
                    if (oldval & USB2OTG_HOSTPORT_PRTCONNDET)   *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
                    if (oldval & USB2OTG_HOSTPORT_PRTRES)       *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND|UPSF_PORT_ENABLE);

                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Port #%ld Change %08lx\n", idx, *mptr));

                    return (0);
                }
            }
            break;
        }

        case (URTF_IN|URTF_CLASS|URTF_DEVICE):
        {
            switch (req)
            {
                case USR_GET_STATUS:
                {
                    UWORD *mptr = ioreq->iouh_Data;

                    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: GetHubStatus (%ld)\n", len));

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
                {
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
        }
    }

    D(bug("[USB2OTG:Hub] UHCMD_CONTROLXFER: Unsupported command %02lx %02lx %04lx %04lx %04lx!\n", rt, req, idx, val, len));

    return (UHIOERR_STALL);
}

WORD FNAME_ROOTHUB(cmdIntXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG:Hub] UHCMD_INTXFER()\n"));

    if ((ioreq->iouh_Endpoint != 1) || (!ioreq->iouh_Length))
    {
        return UHIOERR_STALL;
    }

    if (otg_Unit->hu_HubPortChanged)
    {
        D(bug("[USB2OTG:Hub] UHCMD_INTXFER: Registering Immediate Portchange\n"));

        if (ioreq->iouh_Length == 1)
        {
            *((UBYTE *) ioreq->iouh_Data) = 1;
            ioreq->iouh_Actual = 1;
        }
        else
        {
            ((UBYTE *) ioreq->iouh_Data)[0] = 1;
            ((UBYTE *) ioreq->iouh_Data)[1] = 0;
            ioreq->iouh_Actual = 2;
        }
        otg_Unit->hu_HubPortChanged = FALSE;

        return 0;
    }

    D(bug("[USB2OTG:Hub] UHCMD_INTXFER: Queueing request\n"));

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    Disable();
    AddTail(&otg_Unit->hu_IOPendingQueue, (struct Node *)ioreq);
    Enable();

    return RC_DONTREPLY;
}

void FNAME_ROOTHUB(PendingIO)(struct USB2OTGUnit *otg_Unit)
{
    struct IOUsbHWReq *ioreq;

    if (otg_Unit->hu_HubPortChanged && otg_Unit->hu_IOPendingQueue.lh_Head->ln_Succ)
    {
        D(bug("[USB2OTG:Hub] PendingIO: PortChange detected\n"));
        Disable();
        ioreq = (struct IOUsbHWReq *) otg_Unit->hu_IOPendingQueue.lh_Head;
        while (((struct Node *) ioreq)->ln_Succ)
        {
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            if (ioreq->iouh_Length == 1)
            {
                *((UBYTE *) ioreq->iouh_Data) = 1;
                ioreq->iouh_Actual = 1;
            } else {
                ((UBYTE *) ioreq->iouh_Data)[0] = 1;
                ((UBYTE *) ioreq->iouh_Data)[1] = 0;
                ioreq->iouh_Actual = 2;
            }
            ReplyMsg(&ioreq->iouh_Req.io_Message);
            ioreq = (struct IOUsbHWReq *) otg_Unit->hu_IOPendingQueue.lh_Head;
        }
        otg_Unit->hu_HubPortChanged = FALSE;
        Enable();
    }
}
