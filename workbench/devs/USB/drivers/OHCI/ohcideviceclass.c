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
#include <oop/oop.h>
#include <usb/usb.h>
#include <utility/tagitem.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>

#include <proto/oop.h>
#include <proto/utility.h>

#include <stdio.h>

#include "ohci.h"

static const usb_device_descriptor_t device_descriptor = {
    bLength:            sizeof(usb_device_descriptor_t),
    bDescriptorType:    UDESC_DEVICE,
    bcdUSB:             0x0101,
    bDeviceClass:       UDCLASS_HUB,
    bDeviceSubClass:    UDSUBCLASS_HUB,
    bDeviceProtocol:    UDPROTO_FSHUB,
    bMaxPacketSize:     64,
    iManufacturer:      2,
    iProduct:           1,
    iSerialNumber:      3,
    bNumConfigurations: 1,
};

static usb_endpoint_descriptor_t endpoint_descriptor = {
    bLength:            sizeof(usb_endpoint_descriptor_t),
    bDescriptorType:    UDESC_ENDPOINT,
    bEndpointAddress:   0x81,
    bmAttributes:       0x03,
    wMaxPacketSize:     1,
    bInterval:          255
};

static const char *string1 = "O\0H\0C\0I\0 \0R\0o\0o\0t\0 \0H\0U\0B";
static const int string1_l = sizeof("O\0H\0C\0I\0 \0R\0o\0o\0t\0 \0H\0U\0B");

static const char *string2 = "T\0h\0e\0 \0A\0R\0O\0S\0 \0D\0e\0v\0e\0l\0o\0p\0m\0e\0n\0t\0 \0T\0e\0a\0m";
static const int string2_l = sizeof("T\0h\0e\0 \0A\0R\0O\0S\0 \0D\0e\0v\0e\0l\0o\0p\0m\0e\0n\0t\0 \0T\0e\0a\0m");

BOOL METHOD(OHCI, Hidd_USBDevice, GetString)
{
    msg->string->bDescriptorType = UDESC_STRING;

    if (msg->id == USB_LANGUAGE_TABLE)
    {
        msg->string->bLength = 4;
        msg->string->bString[0] = 0x0409;
    }
    else if (msg->id == 1)
    {
        msg->string->bLength = 2 + string1_l;
        CopyMem(string1, &msg->string->bString[0], string1_l);
    }
    else if (msg->id == 2)
    {
        msg->string->bLength = 2 + string2_l;
        CopyMem(string2, &msg->string->bString[0], string2_l);
    }
    else if (msg->id == 3)
    {
        char buff[129];
        int i;
        snprintf(buff, 128, "%04d.%04d", VERSION_NUMBER, REVISION_NUMBER);
        msg->string->bLength = 2 + 2*strlen(buff);
        for (i=0; i < ((msg->string->bLength - 2) >> 1); i++)
            msg->string->bString[i] = AROS_WORD2LE(buff[i]);
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

const usb_endpoint_descriptor_t * METHOD(OHCI, Hidd_USBDevice, GetEndpoint)
{
    endpoint_descriptor.wMaxPacketSize = AROS_WORD2LE(1);

    if (msg->interface == 0 && msg->endpoint == 0)
        return &endpoint_descriptor;
    else
        return NULL;
}

BOOL METHOD(OHCI, Hidd_USBDevice, GetDeviceDescriptor)
{
    CopyMem(&device_descriptor, msg->descriptor, sizeof(device_descriptor));
    return TRUE;
}

BOOL METHOD(OHCI, Hidd_USBDevice, Configure)
{
    D(bug("[OHCI] Configure(%d)\n", msg->configNr));
    return TRUE;
}

APTR METHOD(OHCI, Hidd_USBDevice, CreatePipe)
{
    if ( (msg->type == PIPE_Interrupt) &&
         (msg->endpoint == 0x81))
        return (APTR)0xdeadbeef;
    else
        return NULL;
}

void METHOD(OHCI, Hidd_USBDevice, DeletePipe)
{
}

void METHOD(OHCI, Hidd_USBDevice, SetTimeout)
{
}
