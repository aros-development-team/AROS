/*
    Copyright (C) 2006 by Michal Schulz
    $Id$

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#define DEBUG 0

#include <inttypes.h>

#include <exec/types.h>
#include <exec/errors.h>
#include <oop/oop.h>
#include <usb/usb.h>
#include <utility/tagitem.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <devices/timer.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>

#include <proto/oop.h>
#include <proto/utility.h>

#include <stdio.h>

#include "ohci.h"

BOOL METHOD(OHCI, Hidd_USBHub, GetHubDescriptor)
{
    ohci_data_t *ohci = OOP_INST_DATA(cl, o);

    if (ohci->hubDescr.bDescriptorType) {
        CopyMem(&ohci->hubDescr, msg->descriptor, sizeof(ohci->hubDescr));
        return TRUE;
    }
    else
        return FALSE;
}

BOOL METHOD(OHCI, Hidd_USBHub, OnOff)
{
    ohci_data_t *ohci = OOP_INST_DATA(cl, o);
    BOOL retval = FALSE;

    D(bug("[OHCI] USBHub::OnOff(%d)\n", msg->on));

    if (msg->on)
    	retval = OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    uint32_t ctl = AROS_OHCI2LONG(mmio(ohci->regs->HcControl));
    ctl &= ~HC_CTRL_HCFS_MASK;
    ctl |= msg->on ? HC_CTRL_HCFS_OPERATIONAL : HC_CTRL_HCFS_SUSPENDED;
    mmio(ohci->regs->HcControl) = AROS_LONG2OHCI(ctl);

    ohci->running = msg->on;

    /*
     * Some OHCI Root Hub interrupts pending and the HUB has been just enabled?
     * Go, fire up the interrupts!
     */
    if (msg->on && ohci->pendingRHSC)
    {
        ohci->pendingRHSC = 0;
        struct Interrupt *intr;

        Disable();
        ForeachNode(&ohci->intList, intr)
        {
            Cause(intr);
        }
        Enable();
    }

    if (!msg->on)
    	retval = OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return retval;
}

BOOL METHOD(OHCI, Hidd_USBHub, PortReset)
{
    ohci_data_t *ohci = OOP_INST_DATA(cl, o);
    int i;

    D(bug("[OHCI] Port %d reset\n", msg->portNummer));

    mmio(ohci->regs->HcRhPortStatus[msg->portNummer-1]) = AROS_LONG2OHCI(UPS_RESET);

    for (i=0; i < 5; i++)
    {
        D(bug("[OHCI] Reset: Waiting for completion\n"));

        ohci_Delay(ohci->tr, USB_PORT_ROOT_RESET_DELAY);
        if ((mmio(ohci->regs->HcRhPortStatus[msg->portNummer-1]) & AROS_LONG2OHCI(UPS_RESET)) == 0)
            break;
    }

    if (i == 5)
        return FALSE;

    mmio(ohci->regs->HcRhPortStatus[msg->portNummer-1]) = AROS_LONG2OHCI(UPS_C_PORT_RESET << 16);

    return TRUE;
}

BOOL METHOD(OHCI, Hidd_USBHub, GetHubStatus)
{
    return TRUE;
}

BOOL METHOD(OHCI, Hidd_USBHub, ClearHubFeature)
{

    return TRUE;
}

BOOL METHOD(OHCI, Hidd_USBHub, SetHubFeature)
{
    return TRUE;
}

BOOL METHOD(OHCI, Hidd_USBHub, GetPortStatus)
{
    ohci_data_t *ohci = OOP_INST_DATA(cl, o);
    BOOL retval = FALSE;

    if (msg->port >= 1 && msg->port <= ohci->hubDescr.bNbrPorts)
    {
        uint32_t val = AROS_OHCI2LONG(mmio(ohci->regs->HcRhPortStatus[msg->port-1]));
        msg->status->wPortStatus = AROS_WORD2LE(val);
        msg->status->wPortChange = AROS_WORD2LE(val >> 16);
        retval = TRUE;
    }

    D(bug("[OHCI] GetPortStatus: sts=%04x chg=%04x\n", AROS_LE2WORD(msg->status->wPortStatus), AROS_LE2WORD(msg->status->wPortChange)));

    return retval;
}

BOOL METHOD(OHCI, Hidd_USBHub, ClearPortFeature)
{
    ohci_data_t *ohci = OOP_INST_DATA(cl, o);

    D(bug("[OHCI] ClearPortFeature(). Port=%d, Feature=%08x\n", msg->port, msg->feature));

    if (msg->port > ohci->hubDescr.bNbrPorts)
        return FALSE;

    switch (msg->feature)
    {
        case UHF_PORT_ENABLE:
            mmio(ohci->regs->HcRhPortStatus[msg->port-1]) = AROS_LONG2OHCI(UPS_CURRENT_CONNECT_STATUS);
            break;

        case UHF_PORT_SUSPEND:
            mmio(ohci->regs->HcRhPortStatus[msg->port-1]) = AROS_LONG2OHCI(UPS_OVERCURRENT_INDICATOR);
            break;

        case UHF_PORT_POWER:
            mmio(ohci->regs->HcRhPortStatus[msg->port-1]) = AROS_LONG2OHCI(UPS_LOW_SPEED);
            break;

        case UHF_C_PORT_CONNECTION:
            mmio(ohci->regs->HcRhPortStatus[msg->port-1]) = AROS_LONG2OHCI(UPS_C_CONNECT_STATUS << 16);
            break;

        case UHF_C_PORT_ENABLE:
            mmio(ohci->regs->HcRhPortStatus[msg->port-1]) = AROS_LONG2OHCI(UPS_C_PORT_ENABLED << 16);
            break;

        case UHF_C_PORT_SUSPEND:
            mmio(ohci->regs->HcRhPortStatus[msg->port-1]) = AROS_LONG2OHCI(UPS_C_SUSPEND << 16);
            break;

        case UHF_C_PORT_OVER_CURRENT:
            mmio(ohci->regs->HcRhPortStatus[msg->port-1]) = AROS_LONG2OHCI(UPS_C_OVERCURRENT_INDICATOR << 16);
            break;

        case UHF_C_PORT_RESET:
            mmio(ohci->regs->HcRhPortStatus[msg->port-1]) = AROS_LONG2OHCI(UPS_C_PORT_RESET << 16);
            break;
    }

    switch (msg->feature)
    {
        case UHF_C_PORT_CONNECTION:
        case UHF_C_PORT_ENABLE:
        case UHF_C_PORT_SUSPEND:
        case UHF_C_PORT_OVER_CURRENT:
        case UHF_C_PORT_RESET:
            if (!CheckIO((struct IORequest *)ohci->timerReq))
                AbortIO((struct IORequest *)ohci->timerReq);
            GetMsg(&ohci->timerPort);
            SetSignal(0, 1 << ohci->timerPort.mp_SigBit);

            D(bug("[OHCI] Reenabling the RHSC interrupt\n"));
            mmio(ohci->regs->HcInterruptEnable) = mmio(ohci->regs->HcInterruptEnable) | AROS_LONG2OHCI(HC_INTR_RHSC);
            break;

        default:
            break;
    }

    return TRUE;
}

BOOL METHOD(OHCI, Hidd_USBHub, SetPortFeature)
{
    ohci_data_t *ohci = OOP_INST_DATA(cl, o);
    int i;

    D(bug("[OHCI] SetPortFeature(). Port=%d, Feature=%08x\n", msg->port, msg->feature));

    if (msg->port > ohci->hubDescr.bNbrPorts)
        return FALSE;

    switch (msg->feature)
    {
        case UHF_PORT_ENABLE:
            mmio(ohci->regs->HcRhPortStatus[msg->port-1]) = AROS_LONG2OHCI(UPS_PORT_ENABLED);
            break;

        case UHF_PORT_SUSPEND:
            mmio(ohci->regs->HcRhPortStatus[msg->port-1]) = AROS_LONG2OHCI(UPS_SUSPEND);
            break;

        case UHF_PORT_POWER:
            mmio(ohci->regs->HcRhPortStatus[msg->port-1]) = AROS_LONG2OHCI(UPS_PORT_POWER);
            break;

        case UHF_PORT_RESET:
            mmio(ohci->regs->HcRhPortStatus[msg->port-1]) = AROS_LONG2OHCI(UPS_RESET);
            for (i=0; i < 5; i++)
            {
                ohci_Delay(ohci->tr, USB_PORT_ROOT_RESET_DELAY);
                if ((mmio(ohci->regs->HcRhPortStatus[msg->port-1]) & AROS_LONG2OHCI(UPS_RESET)) == 0)
                    break;
            }
            if (i == 5)
                return FALSE;

            break;
    }

    return TRUE;
}

AROS_INTH1(OHCI_HubInterrupt, ohci_data_t *,ohci)
{
    AROS_INTFUNC_INIT

    /* Remove self from the msg queue */
    GetMsg(&ohci->timerPort);

    if (ohci->timerReq->tr_node.io_Error == IOERR_ABORTED)
    {
        D(bug("[OHCI] INTR: Abrted\n"));
        return FALSE;
    }

    D(bug("[OHCI] INTR: Reenabling the RHSC interrupt\n"));

    /* Reenable the RHSC interrupt */
    mmio(ohci->regs->HcInterruptStatus) = AROS_LONG2OHCI(HC_INTR_RHSC);
    mmio(ohci->regs->HcInterruptEnable) = AROS_LONG2OHCI(HC_INTR_RHSC);

    return FALSE;

    AROS_INTFUNC_EXIT
}
