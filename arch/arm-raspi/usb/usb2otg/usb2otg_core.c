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

#include "usb2otg_intern.h"

static const UWORD __suported_cmds[] =
{
    CMD_FLUSH,
    CMD_RESET,
    UHCMD_QUERYDEVICE,
    UHCMD_USBRESET,
    UHCMD_USBRESUME,
    UHCMD_USBSUSPEND,
    UHCMD_USBOPER,
    UHCMD_CONTROLXFER,
    UHCMD_ISOXFER,
    UHCMD_INTXFER,
    UHCMD_BULKXFER,
    NSCMD_DEVICEQUERY,
    0
};

struct Unit * FNAME_DEV(OpenUnit)(struct IOUsbHWReq *ioreq,
                        LONG unitnr,
                        LIBBASETYPEPTR USB2OTGBase)
{
    struct USB2OTGUnit *otg_Unit = NULL;
    volatile unsigned int        otg_RegVal;
    unsigned int chan, ns;

    D(bug("[USB2OTG] %s(ioreq:0x%p, unit#%d)\n",
                __PRETTY_FUNCTION__, ioreq, unitnr));

    // We only support a single unit presently
    if ((unitnr == 0) && (USB2OTGBase->hd_Unit))
    {
        otg_Unit = USB2OTGBase->hd_Unit;
        if (!(otg_Unit->hu_UnitAllocated))
        {
            otg_Unit->hu_UnitAllocated = TRUE;

            D(bug("[USB2OTG] %s: Enabling Power ..\n", __PRETTY_FUNCTION__));
            *((volatile unsigned int *)USB2OTG_POWER) = 0;
#if (0)
            D(bug("[USB2OTG] %s: Preparing Controller (non HSIC mode) ..\n", __PRETTY_FUNCTION__));
            *((volatile unsigned int *)USB2OTG_USB) = USB2OTG_USB_MODESELECT|USB2OTG_USB_USBTRDTIM(5)|(USB2OTGBase->hd_Unit->hu_OperatingMode << 29);
            *((volatile unsigned int *)USB2OTG_OTGCTRL) = 0;

            otg_RegVal = *((volatile unsigned int *)USB2OTG_LPMCONFIG);
            otg_RegVal &= ~USB2OTG_LPMCONFIG_HSICCONNECT;
            *((volatile unsigned int *)USB2OTG_LPMCONFIG) = otg_RegVal;

            D(bug("[USB2OTG] %s: Clearing Global NAK ..\n", __PRETTY_FUNCTION__));
            *((volatile unsigned int *)USB2OTG_DEVCTRL) = (1 << 10) | (1 << 8);
#endif
            otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE2);
            if (((otg_RegVal & (3 << 6) >> 6) == 2) && ((otg_RegVal & (3 << 8) >> 8) == 1))
            {
                otg_RegVal = *((volatile unsigned int *)USB2OTG_USB);
                if (otg_RegVal & USB2OTG_USB_ULPIFSLS)
                {
                    D(bug("[USB2OTG] %s: Host clock: 48Mhz\n", __PRETTY_FUNCTION__));
                    otg_RegVal = *((volatile unsigned int *)USB2OTG_HOSTCFG);
                    otg_RegVal &= ~3;
                    otg_RegVal |= 1;
                    *((volatile unsigned int *)USB2OTG_HOSTCFG) = otg_RegVal;
                }
                else
                {
                    D(bug("[USB2OTG] %s: Host clock: 30-60Mhz\n", __PRETTY_FUNCTION__));
                    otg_RegVal = *((volatile unsigned int *)USB2OTG_HOSTCFG);
                    otg_RegVal &= ~3;
                    *((volatile unsigned int *)USB2OTG_HOSTCFG) = otg_RegVal;
                }
            } else {
                D(bug("[USB2OTG] %s: Host clock: 30-60Mhz\n", __PRETTY_FUNCTION__));
                otg_RegVal = *((volatile unsigned int *)USB2OTG_HOSTCFG);
                otg_RegVal &= ~3;
                *((volatile unsigned int *)USB2OTG_HOSTCFG) = otg_RegVal;
            }
            otg_RegVal = *((volatile unsigned int *)USB2OTG_HOSTCFG);
            otg_RegVal |= (1 << 2);
            *((volatile unsigned int *)USB2OTG_HOSTCFG) = otg_RegVal;

            D(bug("[USB2OTG] %s: Enabling HNP...\n", __PRETTY_FUNCTION__));
            otg_RegVal = *((volatile unsigned int *)USB2OTG_OTGCTRL);
            otg_RegVal |= USB2OTG_OTGCTRL_HOSTSETHNPENABLE;
            *((volatile unsigned int *)USB2OTG_OTGCTRL) = otg_RegVal;

            D(bug("[USB2OTG] %s: Flushing Tx Fifo's...\n", __PRETTY_FUNCTION__));
            *((volatile unsigned int *)USB2OTG_RESET) = USB2OTG_RESET_TXFIFOFLUSH|(16 << 6);
            for (ns = 0; ns < 10000; ns++) { asm volatile("mov r0, r0\n"); } // Wait 10ms
            if ((*((volatile unsigned int *)USB2OTG_RESET) & USB2OTG_RESET_TXFIFOFLUSH) != 0)
                bug("[USB2OTG] %s: Tx Flush Timed-Out!\n", __PRETTY_FUNCTION__);

            D(bug("[USB2OTG] %s: Flushing Rx Fifo's...\n", __PRETTY_FUNCTION__));
            *((volatile unsigned int *)USB2OTG_RESET) = USB2OTG_RESET_RXFIFOFLUSH;
            for (ns = 0; ns < 10000; ns++) { asm volatile("mov r0, r0\n"); } // Wait 10ms
            if ((*((volatile unsigned int *)USB2OTG_RESET) & USB2OTG_RESET_RXFIFOFLUSH) != 0)
                bug("[USB2OTG] %s: Rx Flush Timed-Out!\n", __PRETTY_FUNCTION__);

            otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE3);
            D(bug("[USB2OTG] %s: Queue Depths:\n",
                        __PRETTY_FUNCTION__));
            D(bug("[USB2OTG] %s:      Periodic Transmit: 0x%0x\n",
                        __PRETTY_FUNCTION__, ((otg_RegVal & (0x3 << 24)) >> 24)));
            D(bug("[USB2OTG] %s:      Non-Periodic Transmit: 0x%0x\n",
                        __PRETTY_FUNCTION__, ((otg_RegVal & (0x3 << 22)) >> 22)));
#if (0)
            D(bug("[USB2OTG] %s:      Device Tokens: 0x%0x\n",
                        __PRETTY_FUNCTION__, ((otg_RegVal & (0x1F << 26)) >> 26)));
#endif
            D(bug("[USB2OTG] %s:      FIFO: %ld bytes\n",
                        __PRETTY_FUNCTION__, ((otg_RegVal & (0xFFFF << 16)) >> 16) << 2));
#if (0)
            D(bug("[USB2OTG] %s: Xfer Size: %ld\n",
                        __PRETTY_FUNCTION__, (otg_RegVal & 0xF)));
#endif

            otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE2);
            if ((otg_Unit->hu_HostChans = ((otg_RegVal & (0xF << 14)) >> 14) + 1) > EPSCHANS_MAX)
                otg_Unit->hu_HostChans = EPSCHANS_MAX;

            otg_RegVal = *((volatile unsigned int *)USB2OTG_HOSTCFG);
            if ((otg_RegVal & (1 << 23)) == 0)
            {
                D(bug("[USB2OTG] %s: Host Channels: %d\n",
                            __PRETTY_FUNCTION__, otg_Unit->hu_HostChans));

                for (chan = 0; chan < otg_Unit->hu_HostChans; chan++) {
#if (0)
                    *((volatile unsigned int *)(USB2OTG_HOST_CHANBASE + (chan * USB2OTG_HOST_CHANREGSIZE) + USB2OTG_HOSTCHAN_INTRMASK)) = 
                        (USB2OTG_INTRCHAN_STALL|USB2OTG_INTRCHAN_BABBLEERROR|USB2OTG_INTRCHAN_TRANSACTIONERROR) |
                        (USB2OTG_INTRCHAN_NEGATIVEACKNOWLEDGE|USB2OTG_INTRCHAN_ACKNOWLEDGE|USB2OTG_INTRCHAN_NOTREADY) |
                        (USB2OTG_INTRCHAN_HALT|USB2OTG_INTRCHAN_FRAMEOVERRUN|USB2OTG_INTRCHAN_DATATOGGLEERROR);
#endif
                    otg_RegVal = *((volatile unsigned int *)(USB2OTG_HOST_CHANBASE + (chan * USB2OTG_HOST_CHANREGSIZE) + USB2OTG_HOSTCHAN_CHARBASE)); 
                    D(bug("[USB2OTG] %s:      Chan #%d FIFO @ 0x%p, Characteristics: %08x -> %08x\n",
                                __PRETTY_FUNCTION__,
                                chan, USB2OTG_FIFOBASE + (chan * USB2OTG_FIFOSIZE), otg_RegVal, (otg_RegVal & ~USB2OTG_HOSTCHAR_ENABLE) | (USB2OTG_HOSTCHAR_DISABLE | (1 << USB2OTG_HOSTCHAR_EPDIR))
                                ));
                    otg_RegVal &= ~USB2OTG_HOSTCHAR_ENABLE;
                    otg_RegVal |= USB2OTG_HOSTCHAR_DISABLE | (1 << USB2OTG_HOSTCHAR_EPDIR);
                    *((volatile unsigned int *)(USB2OTG_HOST_CHANBASE + (chan * USB2OTG_HOST_CHANREGSIZE) + USB2OTG_HOSTCHAN_CHARBASE)) = otg_RegVal;
                }

                for (chan = 0; chan < otg_Unit->hu_HostChans; chan++) {
                    otg_RegVal = *((volatile unsigned int *)(USB2OTG_HOST_CHANBASE + (chan * USB2OTG_HOST_CHANREGSIZE) + USB2OTG_HOSTCHAN_CHARBASE)); 
                    otg_RegVal |= USB2OTG_HOSTCHAR_ENABLE|USB2OTG_HOSTCHAR_DISABLE|(1 << USB2OTG_HOSTCHAR_EPDIR);
                    *((volatile unsigned int *)(USB2OTG_HOST_CHANBASE + (chan * USB2OTG_HOST_CHANREGSIZE) + USB2OTG_HOSTCHAN_CHARBASE)) = otg_RegVal;
                    for (ns = 0; ns < 100000; ns++) { asm volatile("mov r0, r0\n"); } // Wait 100ms
                    if ((*((volatile unsigned int *)(USB2OTG_HOST_CHANBASE + (chan * USB2OTG_HOST_CHANREGSIZE) + USB2OTG_HOSTCHAN_CHARBASE)) & USB2OTG_HOSTCHAR_ENABLE) != 0)
                        bug("[USB2OTG] %s: Unable to clear Halt on channel #%d\n", __PRETTY_FUNCTION__, chan);
                }
                
#if (0)
                D(bug("[USB2OTG] %s: Enabling HOST Channel Interrupts ...\n",
                            __PRETTY_FUNCTION__));
                *((volatile unsigned int *)USB2OTG_HOSTINTRMASK) = (1U << otg_Unit->hu_HostChans) - 1U;
#endif
            }
#if (0)
            otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE2);

            if (!(otg_Unit->hu_OperatingMode) || (otg_Unit->hu_OperatingMode == USB2OTG_USBDEVICEMODE))
            {
                D(bug("[USB2OTG] %s: Configuring USB Core DEVICE mode -:\n",
                            __PRETTY_FUNCTION__));

                if ((otg_Unit->hu_DevEPs = ((otg_RegVal & (0xF << 10)) >> 10) + 1) > EPSCHANS_MAX)
                    otg_Unit->hu_DevEPs = EPSCHANS_MAX;

                otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE4);
                otg_Unit->hu_DevInEPs = ((otg_RegVal & (0xF << 26)) >> 26) + 1;

                D(bug("[USB2OTG] %s:      Endpoints: %d/%d\n",
                            __PRETTY_FUNCTION__, otg_Unit->hu_DevInEPs, otg_Unit->hu_DevEPs));

                for (chan = 0; chan < otg_Unit->hu_DevEPs; chan++) {
                    D(bug("[USB2OTG] %s:      Endpoint #%d ", __PRETTY_FUNCTION__, chan));
                    if (chan < otg_Unit->hu_DevInEPs)
                    {
                        D(bug("IN_CTL: %08x, ", *((volatile unsigned int *)(USB2OTG_DEV_INEP_BASE + (chan * USB2OTG_DEV_EPSIZE) + USB2OTG_DEV_INEP_DIEPCTL))));
                    }
                    D(bug("OUT_CTL: %08x\n", *((volatile unsigned int *)(USB2OTG_DEV_OUTEP_BASE + (chan * USB2OTG_DEV_EPSIZE) + USB2OTG_DEV_OUTEP_DOEPCTL))));
                }
            }
#endif

#if (0)
            D(bug("[USB2OTG] %s: Pulling-Up D+ ..\n", __PRETTY_FUNCTION__));
            otg_RegVal = *((volatile unsigned int *)USB2OTG_DEVCTRL);
            otg_RegVal &= ~(1 << 1);
            *((volatile unsigned int *)USB2OTG_DEVCTRL) = otg_RegVal;
#endif

            otg_RegVal = *((volatile unsigned int *)USB2OTG_HOSTPORT);
            if (!(otg_RegVal & USB2OTG_HOSTPORT_PRTPWR))
            {
                D(bug("[USB2OTG] %s: Powering On Host Port ..\n", __PRETTY_FUNCTION__));
                otg_RegVal |= USB2OTG_HOSTPORT_PRTPWR;
                *((volatile unsigned int *)USB2OTG_HOSTPORT) = otg_RegVal;    
            }

            D(bug("[USB2OTG] %s: Reseting Host Port ..\n", __PRETTY_FUNCTION__));            
            otg_RegVal |= USB2OTG_HOSTPORT_PRTRST;
            *((volatile unsigned int *)USB2OTG_HOSTPORT) = otg_RegVal;
            for (ns = 0; ns < 10000; ns++) { asm volatile("mov r0, r0\n"); } // Wait 10ms
            otg_RegVal &= ~USB2OTG_HOSTPORT_PRTRST;
            *((volatile unsigned int *)USB2OTG_HOSTPORT) = otg_RegVal;

#if (0)
            D(bug("[USB2OTG] %s: Configuring Interrupts ...\n",
                        __PRETTY_FUNCTION__));

            *((volatile unsigned int *)USB2OTG_INTRMASK) = 
                (USB2OTG_INTRCORE_ENUMERATIONDONE|USB2OTG_INTRCORE_USBRESET|USB2OTG_INTRCORE_USBSUSPEND) |
                (USB2OTG_INTRCORE_INENDPOINT|USB2OTG_INTRCORE_RECEIVESTATUSLEVEL|USB2OTG_INTRCORE_SESSIONREQUEST|USB2OTG_INTRCORE_OTG|USB2OTG_INTRCORE_HOSTCHANNEL|USB2OTG_INTRCORE_PORT);

            if (!(otg_Unit->hu_OperatingMode) || (otg_Unit->hu_OperatingMode == USB2OTG_USBDEVICEMODE))
            {
                otg_RegVal = *((volatile unsigned int *)USB2OTG_HARDWARE2);
                if (otg_RegVal & (1 << 20))
                {
                    D(bug("[USB2OTG] %s: Multi-Process Interrupts ...\n",
                        __PRETTY_FUNCTION__));
                    for (chan = 0; chan < otg_Unit->hu_DevInEPs; chan++) {
//                        *((volatile unsigned int *)(USB2OTG_DEV_INEP_BASE + (chan * USB2OTG_DEV_EPSIZE) + USB2OTG_DEV_INEP_DIEPCTL)) = (1 << 0);
//                        *((volatile unsigned int *)(USB2OTG_DEV_INEP_BASE + (chan * USB2OTG_DEV_EPSIZE) + USB2OTG_DEV_INEP_DIEPCTL)) = 0;
                        *((volatile unsigned int *)USB2OTG_DEVEACHINTMSK) = 0xFFFF;
                    }
                }
                else
                {
                    *((volatile unsigned int *)USB2OTG_DEVINEPMASK) = (1 << 0);
                    *((volatile unsigned int *)USB2OTG_DEVOUTEPMASK) = 0;
                    *((volatile unsigned int *)USB2OTG_DEVINTRMASK) = 0xFFFF;
                }
            }

            D(bug("[USB2OTG] %s: Enabling Global Interrupts ...\n",
                        __PRETTY_FUNCTION__));
            *((volatile unsigned int *)USB2OTG_AHB) = USB2OTG_AHB_INTENABLE;
#endif

            D(bug("[USB2OTG] %s: Initialising NakTimeout (intr @ 0x%p) ...\n",
                        __PRETTY_FUNCTION__, &otg_Unit->hu_NakTimeoutInt));

            Cause(&otg_Unit->hu_NakTimeoutInt);
        }

        otg_RegVal = *((volatile unsigned int *)USB2OTG_OTGCTRL);
        D(bug("[USB2OTG] %s: OTG Control: %08x\n",
                    __PRETTY_FUNCTION__, otg_RegVal));

        return (&otg_Unit->hu_Unit);
    }

    return (NULL);
}

void FNAME_DEV(CloseUnit)(struct IOUsbHWReq *ioreq, struct USB2OTGUnit *otg_Unit, LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] %s(unit:0x%p, ioreq:0x%p)\n",
                __PRETTY_FUNCTION__, otg_Unit, ioreq));

    otg_Unit->hu_UnitAllocated = FALSE;
}

void FNAME_DEV(TermIO)(struct IOUsbHWReq *ioreq,
            LIBBASETYPEPTR USB2OTGBase)
{
    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;

    if (!(ioreq->iouh_Req.io_Flags & IOF_QUICK))
    {
        ReplyMsg(&ioreq->iouh_Req.io_Message);
    }
}

WORD FNAME_DEV(cmdNSDeviceQuery)(struct IOStdReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    struct USBNSDeviceQueryResult *query = (struct USBNSDeviceQueryResult *)ioreq->io_Data;

    D(bug("[USB2OTG] NSCMD_DEVICEQUERY(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if ((!query) ||
       (ioreq->io_Length < sizeof(struct USBNSDeviceQueryResult)) ||
       (query->DevQueryFormat != 0) ||
       (query->SizeAvailable != 0))
    {
        ioreq->io_Error = IOERR_NOCMD;
        FNAME_DEV(TermIO)((struct IOUsbHWReq *) ioreq, USB2OTGBase);

        return RC_DONTREPLY;
    }

    ioreq->io_Actual         = query->SizeAvailable
                             = sizeof(struct USBNSDeviceQueryResult);
    query->DeviceType        = NSDEVTYPE_USBHARDWARE;
    query->DeviceSubType     = 0;
    query->SupportedCommands = __suported_cmds;

    return RC_OK;
}

WORD FNAME_DEV(cmdQueryDevice)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    struct TagItem *taglist = (struct TagItem *) ioreq->iouh_Data;
    struct TagItem *tag;
    ULONG count = 0;

    D(bug("[USB2OTG] UHCMD_QUERYDEVICE(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if (((tag = FindTagItem(UHA_State, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = (IPTR)UHSF_OPERATIONAL;
        count++;
    }

    if (((tag = FindTagItem(UHA_Manufacturer, taglist)) != NULL) && (tag->ti_Data))
    {
        *((STRPTR *) tag->ti_Data) = "The AROS Dev Team";
        count++;
    }

    if (((tag = FindTagItem(UHA_ProductName, taglist)) != NULL) && (tag->ti_Data))
    {
        *((STRPTR *) tag->ti_Data) = "HS USB 2.0 OTG Controller";
        count++;
    }

    if (((tag = FindTagItem(UHA_Description, taglist)) != NULL) && (tag->ti_Data))
    {
        *((STRPTR *) tag->ti_Data) = "Synopsys/DesignWare USB 2.0 OTG Controller for Raspberry Pi";
        count++;
    }

    if (((tag = FindTagItem(UHA_Copyright, taglist)) != NULL) && (tag->ti_Data))
    {
        *((STRPTR *) tag->ti_Data) ="Â©2013 The AROS Dev Team";
        count++;
    }

    if (((tag = FindTagItem(UHA_Version, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = VERSION_NUMBER;
        count++;
    }

    if (((tag = FindTagItem(UHA_Revision, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = REVISION_NUMBER;
        count++;
    }

    if (((tag = FindTagItem(UHA_DriverVersion, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = 0x100;
        count++;
    }

    if (((tag = FindTagItem(UHA_Capabilities, taglist)) != NULL) && (tag->ti_Data))
    {
        *((IPTR *) tag->ti_Data) = UHCF_USB20;
        count++;
    }
    ioreq->iouh_Actual = count;

    return RC_OK;
}

WORD FNAME_DEV(cmdReset)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] CMD_RESET(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if (ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }

    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdFlush)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] CMD_FLUSH(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    return RC_OK;
}

WORD FNAME_DEV(cmdUsbReset)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_USBRESET(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if (ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdUsbResume)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_USBRESUME(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if (ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return (WORD)UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdUsbSuspend)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_USBSUSPEND(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if (ioreq->iouh_State & UHSF_SUSPENDED)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdUsbOper)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_USBOPER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

    if (ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}

WORD FNAME_DEV(cmdControlXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_CONTROLXFER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

#if (0)
    if (!(ioreq->iouh_State & UHSF_OPERATIONAL))
    {
        return (UHIOERR_USBOFFLINE);
    }
#endif

    if (ioreq->iouh_DevAddr == otg_Unit->hu_HubAddr)
    {
        return (FNAME_ROOTHUB(cmdControlXFer)(ioreq, otg_Unit, USB2OTGBase));
    }

    D(bug("[USB2OTG] UHCMD_CONTROLXFER: DevAddr #%ld\n",
                ioreq->iouh_DevAddr));

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail(&otg_Unit->hu_CtrlXFerQueue, (struct Node *) ioreq);
    Enable();
    FNAME_DEV(Cause)(USB2OTGBase, &otg_Unit->hu_PendingInt);

    D(bug("[USB2OTG] UHCMD_CONTROLXFER: handled ioreq @ 0x%p\n", ioreq));

    return (RC_DONTREPLY);
}

WORD FNAME_DEV(cmdBulkXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_BULKXFER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

#if (0)
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL))
    {
        return(UHIOERR_USBOFFLINE);
    }
#endif

    if (ioreq->iouh_Flags & UHFF_LOWSPEED)
    {
        return(UHIOERR_BADPARAMS);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail(&otg_Unit->hu_BulkXFerQueue, (struct Node *) ioreq);
    Enable();
    FNAME_DEV(Cause)(USB2OTGBase, &otg_Unit->hu_PendingInt);

    D(bug("[USB2OTG] UHCMD_BULKXFER: handled ioreq @ 0x%p\n", ioreq));

    return (RC_DONTREPLY);
}

WORD FNAME_DEV(cmdIntXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_INTXFER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

#if (0)
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL))
    {
        return (UHIOERR_USBOFFLINE);
    }
#endif

    if (ioreq->iouh_DevAddr == otg_Unit->hu_HubAddr)
    {
        return (FNAME_ROOTHUB(cmdIntXFer)(ioreq, otg_Unit, USB2OTGBase));
    }

    D(bug("[USB2OTG] UHCMD_INTXFER: DevAddr #%ld\n",
                ioreq->iouh_DevAddr));

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail(&otg_Unit->hu_IntXFerQueue, (struct Node *) ioreq);
    Enable();
    FNAME_DEV(Cause)(USB2OTGBase, &otg_Unit->hu_PendingInt);

    D(bug("[USB2OTG] UHCMD_INTXFER: handled ioreq @ 0x%p\n", ioreq));

    return (RC_DONTREPLY);
}

WORD FNAME_DEV(cmdIsoXFer)(struct IOUsbHWReq *ioreq,
                       struct USB2OTGUnit *otg_Unit,
                       LIBBASETYPEPTR USB2OTGBase)
{
    D(bug("[USB2OTG] UHCMD_ISOXFER(unit:0x%p, ioreq:0x%p)\n",
                otg_Unit, ioreq));

#if (0)
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL))
    {
        return(UHIOERR_USBOFFLINE);
    }
#endif
    if(ioreq->iouh_Flags & UHFF_LOWSPEED)
    {
        return(UHIOERR_BADPARAMS);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail(&otg_Unit->hu_IsoXFerQueue, (struct Node *) ioreq);
    Enable();
    FNAME_DEV(Cause)(USB2OTGBase, &otg_Unit->hu_PendingInt);

    D(bug("[USB2OTG] UHCMD_ISOXFER: handled ioreq @ 0x%p\n", ioreq));
    return(RC_DONTREPLY);
}
