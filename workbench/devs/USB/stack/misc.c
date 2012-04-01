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

#include "misc.h"

#include <proto/exec.h>

/*
 * TODO: fix up all the mess below. too many resource allocations, a bit too complex
 */

void DumpDescriptor(usb_descriptor_t *desc)
{
    bug("[USB] Descriptor dump:\n");
    bug("[USB]  bLength = %d\n", desc->bLength);
    
    if (desc->bDescriptorType == UDESC_DEVICE)
    {
        bug("[USB]  bDescriptorType = %d (Device)\n", desc->bDescriptorType);
        usb_device_descriptor_t *d = (usb_device_descriptor_t *)desc;
        bug("[USB]  bcdUSB = 0x%04x\n", AROS_LE2WORD(d->bcdUSB));
        bug("[USB]  bDeviceClass = 0x%02x\n", d->bDeviceClass);
        bug("[USB]  bDeviceSubClass = 0x%02x\n", d->bDeviceSubClass);
        bug("[USB]  bDeviceProtocol = 0x%02x\n", d->bDeviceProtocol);
        bug("[USB]  bMaxPacketSize = 0x%04x\n", d->bMaxPacketSize);
        bug("[USB]  idVendor = 0x%04x\n", d->idVendor);
        bug("[USB]  idProduct = 0x%04x\n", AROS_LE2WORD(d->idProduct));
        bug("[USB]  bcdDevice = 0x%04x\n", AROS_LE2WORD(d->bcdDevice));
        bug("[USB]  iManufacturer = 0x%02x\n", d->iManufacturer);
        bug("[USB]  iProduct = 0x%02x\n", d->iProduct);
        bug("[USB]  iSerialNumber = 0x%02x\n", d->iSerialNumber);
        bug("[USB]  bNumConfigurations = %d\n", d->bNumConfigurations);
    }
    else if (desc->bDescriptorType == UDESC_CONFIG)
    {
        bug("[USB]  bDescriptorType = %d (Config)\n", desc->bDescriptorType);
        usb_config_descriptor_t *d = (usb_config_descriptor_t *)desc;
        bug("[USB]  wTotalLength = %d\n", AROS_LE2WORD(d->wTotalLength));
        bug("[USB]  bNumInterface = %d\n", d->bNumInterface);
        bug("[USB]  bConfigurationValue = %d\n", d->bConfigurationValue);
        bug("[USB]  iConfiguration = %d\n", d->iConfiguration);
        bug("[USB]  bmAttributes = 0x%02x  ", d->bmAttributes);
        if (d->bmAttributes & UC_SELF_POWERED)
            bug("SELF_POWERED ");
        if (d->bmAttributes & UC_BUS_POWERED)
            bug("BUS_POWERED ");
        if (d->bmAttributes & UC_REMOTE_WAKEUP)
            bug("REMOTE_WAKEUP ");
        bug("\n");
        bug("[USB]  bMaxPower = %d mA\n", d->bMaxPower * UC_POWER_FACTOR);
    }
    else if (desc->bDescriptorType == UDESC_HUB)
    {
        usb_hub_descriptor_t *d = (usb_hub_descriptor_t *)desc;
        int i;
        bug("[USB]  bDescriptorType = %d (Hub)\n", desc->bDescriptorType);
        bug("[USB]  bNbrPorts = %d\n", d->bNbrPorts);
        bug("[USB]  wHubCharacteristics = 0x%04x\n", AROS_LE2WORD(d->wHubCharacteristics));
        bug("[USB]  bPwrOn2PwrGood = %d ms\n", d->bPwrOn2PwrGood * UHD_PWRON_FACTOR);
        bug("[USB]  bHubContrCurrent = %d\n", d->bHubContrCurrent);
        bug("[USB]  DeviceRemovable = ");
        for (i=0; i < 32; i++)
            bug("%02x", d->DeviceRemovable[i]);
        bug("\n");
    }
    else if (desc->bDescriptorType == UDESC_ENDPOINT)
    {
        bug("[USB]  bDescriptorType = %d (Endpoint)\n", desc->bDescriptorType);
        usb_endpoint_descriptor_t *d = (usb_endpoint_descriptor_t *)desc;
        bug("[USB]  bEndpointAddress = %02x\n", d->bEndpointAddress);
        bug("[USB]  bmAttributes = %02x\n", d->bmAttributes);
        bug("[USB]  wMaxPacketSize = %d\n", AROS_LE2WORD(d->wMaxPacketSize));
        bug("[USB]  bInterval = %d\n", d->bInterval);
    }
    else
        bug("[USB]  bDescriptorType = %d\n", desc->bDescriptorType);
}

/*
 * usb_delay() stops waits for specified amount of miliseconds. It uses the timerequest
 * of specified USB device. No pre-allocation of signals is required.
 */
void USBDelay(struct timerequest *tr, uint32_t msec)
{
    /* Allocate a signal within this task context */
    tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit = AllocSignal(-1);
    tr->tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);
    
    /* Specify the request */
    tr->tr_node.io_Command = TR_ADDREQUEST;
    tr->tr_time.tv_secs = msec / 1000;
    tr->tr_time.tv_micro = 1000 * (msec % 1000);

    /* Wait */
    DoIO((struct IORequest *)tr);
    
    /* The signal is not needed anymore */
    FreeSignal(tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit);
    tr->tr_node.io_Message.mn_ReplyPort->mp_SigTask = NULL;
}

/*
 * usbtimer() sets a timer to wake the process up after timeout expires
 * NOTE: it assumes your task and sigbit fields are filled in already
 */
uint32_t USBTimer(struct timerequest *tr, uint32_t msec)
{
    /* Allocate a signal within this task context */
    tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit = AllocSignal(-1);
    tr->tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);
    
    /* Specify the request */
    tr->tr_node.io_Command = TR_ADDREQUEST;
    tr->tr_time.tv_secs = msec / 1000;
    tr->tr_time.tv_micro = 1000 * (msec % 1000);

    /* Wait */
    SendIO((struct IORequest *)tr);
    
    return (tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit);
}

/*
 * usbtimerdone() completes the above
 */
void USBTimerDone(struct timerequest *tr)
{
    WaitPort(tr->tr_node.io_Message.mn_ReplyPort);
    /* since we created port and we issued only one request, it's only one message */
    GetMsg(tr->tr_node.io_Message.mn_ReplyPort);
    /* The signal is not needed anymore */
    FreeSignal(tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit);
    tr->tr_node.io_Message.mn_ReplyPort->mp_SigTask = NULL;
}


struct timerequest *USBCreateTimer()
{
    struct timerequest *tr = NULL;
    struct MsgPort *mp = NULL;
    
    mp = CreateMsgPort();
    if (mp)
    {        
        tr = (struct timerequest *)CreateIORequest(mp, sizeof(struct timerequest));
        if (tr)
        {
            FreeSignal(mp->mp_SigBit);
            if (!OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0))
                return tr;
            
            DeleteIORequest((struct IORequest *)tr);
            mp->mp_SigBit = AllocSignal(-1);
        }
        DeleteMsgPort(mp);
    }
    
    return NULL;
}

void USBDeleteTimer(struct timerequest *tr)
{
    if (tr)
    {
        tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit = AllocSignal(-1);
        CloseDevice((struct IORequest *)tr);
        DeleteMsgPort(tr->tr_node.io_Message.mn_ReplyPort);
        DeleteIORequest((struct IORequest *)tr);
    }
}
