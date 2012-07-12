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

#define DEBUG 1

#include <inttypes.h>

#include <aros/debug.h>

#include <exec/types.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <usb/usb.h>

#include <proto/oop.h>
#include <proto/utility.h>

#include "usb.h"
#include "misc.h"

/******************** Local utility functions area ********************/

static const char *unknown_name = "? unknown name ?";
static const char *unknown_manufacturer = "? unknown manufacturer ?";
static const char *unknown_serial = "? unknown serial ?";

static BOOL usb_SetAddress(OOP_Class *cl, OOP_Object *o, uint8_t address)
{
    DeviceData *dev = OOP_INST_DATA(cl, o);
    void *pipe = NULL;

    USBDevice_Request request = {
            bmRequestType:  UT_WRITE_DEVICE,
            bRequest:       UR_SET_ADDRESS,
            wValue:         AROS_WORD2LE(address),
            wIndex:         AROS_WORD2LE(0),
            wLength:        AROS_WORD2LE(0)
    };

    if (dev->default_pipe)
        pipe = dev->default_pipe;
    else
        pipe = HIDD_USBDrv_CreatePipe(dev->bus, PIPE_Control, dev->fast, dev->address, 0, 0, dev->maxpacket, 100);

    BOOL ret = HIDD_USBDrv_ControlTransfer(dev->bus, pipe, &request, NULL, 0);
    HIDD_USBDrv_DeletePipe(dev->bus, pipe);

    dev->default_pipe = NULL;

    if (ret)
    {
        D(bug("[USBDevice::New] testing address %d...\n", address));
        usb_device_descriptor_t descriptor;

        pipe = HIDD_USBDrv_CreatePipe(dev->bus, PIPE_Control, dev->fast, address, 0, 0, dev->maxpacket, 100);

        USBDevice_Request request = {
                bmRequestType:  UT_READ_DEVICE,
                bRequest:       UR_GET_DESCRIPTOR,
                wValue:         AROS_WORD2LE(UDESC_DEVICE << 8),
                wIndex:         AROS_WORD2LE(0),
                wLength:        AROS_WORD2LE(USB_DEVICE_DESCRIPTOR_SIZE)
        };
        ret = HIDD_USBDrv_ControlTransfer(dev->bus, pipe, &request, &descriptor, sizeof(descriptor));

        if (ret)
        {
            D(bug("[USBDevice::New] New address set correctly\n"));
            dev->address = address;
            dev->default_pipe = pipe;
            return TRUE;
        }
        else
        {
            HIDD_USBDrv_DeletePipe(dev->bus, pipe);
            return FALSE;
        }
    }
    else return FALSE;
}



/******************** Implementation of interfaces ********************/

BOOL METHOD(USBDevice, Hidd_USBDevice, GetString)
{
    DeviceData *dev = OOP_INST_DATA(cl, o);

    if (msg->string)
    {
        USBDevice_Request request = {
                bmRequestType:  UT_READ_DEVICE,
                bRequest:       UR_GET_DESCRIPTOR,
                wValue:         AROS_WORD2LE(UDESC_STRING << 8 | (msg->id & 0xff)),
                wIndex:         AROS_WORD2LE(msg->language),
                wLength:        AROS_WORD2LE(1)
        };

        if (HIDD_USBDevice_ControlMessage(o, NULL, &request, msg->string, 1))
        {
            request.wLength = AROS_WORD2LE(msg->string->bLength);
            if (HIDD_USBDrv_ControlTransfer(dev->bus, dev->default_pipe, &request, msg->string, msg->string->bLength))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

OOP_Object *METHOD(USBDevice, Root, New)
{
    D(bug("[USB] USBDevice::New()\n"));

    BASE(cl->UserData)->LibNode.lib_OpenCnt++;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        DeviceData *dev = OOP_INST_DATA(cl, o);
        usb_string_descriptor_t string;
        uint16_t langid;
        int i;

        dev->tr = USBCreateTimer();

        dev->address = GetTagData(aHidd_USBDevice_Address, 0, msg->attrList);
        dev->hub = (OOP_Object *)GetTagData(aHidd_USBDevice_Hub, 0, msg->attrList);
        dev->bus = (OOP_Object *)GetTagData(aHidd_USBDevice_Bus, 0, msg->attrList);
        dev->fast = GetTagData(aHidd_USBDevice_Fast, TRUE, msg->attrList);
        dev->maxpacket = GetTagData(aHidd_USBDevice_MaxPacketSize, 8, msg->attrList);
        dev->iface = GetTagData(aHidd_USBDevice_Interface, 0, msg->attrList);
        dev->default_pipe = NULL;
        dev->config = USB_UNCONFIG_NO;
        dev->next = (OOP_Object *)GetTagData(aHidd_USBDevice_Next, 0, msg->attrList);

        /*
         * The USB bus object not in attrList. Try to get it from itself, with help of GetAttr call.
         * It sounds ridiculous, but it might happen that the GetAttr is overrided already (it is
         * the case of HUB embedded in the driver
         */

        if (!dev->bus && dev->hub)
            OOP_GetAttr(dev->hub, aHidd_USBDevice_Bus, (IPTR*)&dev->bus);

        D(bug("[USBDevice::New] Address=%02x, Interface=%02x, Bus=%p, Hub=%p\n", dev->address, dev->iface, dev->bus, dev->hub));

        if (dev->bus)
        {
            if (!dev->default_pipe)
            {
                dev->default_pipe = HIDD_USBDevice_CreatePipe(o, PIPE_Control, 0, 0, 0, 100);
            }

            /* Address was either unknown or equals zero. In such case the right address has
             * to be set */
            if (dev->address == 0)
            {
                D(bug("[USBDevice::New] fetching new device address\n"));
                uint8_t addr = HIDD_USB_AllocAddress(SD(cl)->usb, dev->bus);
                D(bug("[USBDevice::New] trying address %d...\n", addr));
                if (!usb_SetAddress(cl, o, addr))
                    HIDD_USB_FreeAddress(SD(cl)->usb, dev->bus, addr);
            }

            /* Check whether the address is set now */
            if (!dev->address)
            {
                OOP_MethodID disp_mid = OOP_GetMethodID((STRPTR)IID_Root, moRoot_Dispose);
                OOP_CoerceMethod(cl, o, &disp_mid);
                o = NULL;
            }
        }


        HIDD_USBDevice_GetDeviceDescriptor(o, &dev->descriptor);

        D(bug("[USBDevice::New] Device %04x:%04x %02x/%02x/%02x at address %08x:%02x\n",
                AROS_LE2WORD(dev->descriptor.idProduct), AROS_LE2WORD(dev->descriptor.idVendor),
                dev->descriptor.bDeviceClass, dev->descriptor.bDeviceSubClass, dev->descriptor.bDeviceProtocol,
                dev->bus, dev->address));

        HIDD_USBDevice_GetString(o, USB_LANGUAGE_TABLE, 0, &string);
        D(bug("[USBDevice::New] Default LangID=%04x\n", string.bString[0]));
        langid = string.bString[0];

        if (dev->descriptor.iProduct && HIDD_USBDevice_GetString(o, dev->descriptor.iProduct, langid, &string))
        {
            dev->product_name = AllocVecPooled(SD(cl)->MemPool, 1 + ((string.bLength - 2) >> 1));

            for (i=0; i < (string.bLength - 2) >> 1; i++) {
                dev->product_name[i] = AROS_LE2WORD(string.bString[i]);
            }
            dev->product_name[(string.bLength - 2) >> 1] = 0;

            D(bug("[USBDevice::New] iProduct = \"%s\"\n", dev->product_name));
        }
        else {
            dev->product_name = AllocVecPooled(SD(cl)->MemPool, 1 + strlen(unknown_name));
            CopyMem(unknown_name, dev->product_name, strlen(unknown_name) + 1);
        }

        if (dev->descriptor.iManufacturer && HIDD_USBDevice_GetString(o, dev->descriptor.iManufacturer, langid, &string))
        {
            dev->manufacturer_name = AllocVecPooled(SD(cl)->MemPool, 1 + ((string.bLength - 2) >> 1));

            for (i=0; i < (string.bLength - 2) >> 1; i++) {
                dev->manufacturer_name[i] = AROS_LE2WORD(string.bString[i]);
            }
            dev->manufacturer_name[(string.bLength - 2) >> 1] = 0;

            D(bug("[USBDevice::New] iManufacturer = \"%s\"\n", dev->manufacturer_name));
        }
        else {
            dev->manufacturer_name = AllocVecPooled(SD(cl)->MemPool, 1 + strlen(unknown_manufacturer));
            CopyMem(unknown_manufacturer, dev->manufacturer_name, strlen(unknown_manufacturer) + 1);
        }


        if (dev->descriptor.iSerialNumber && HIDD_USBDevice_GetString(o, dev->descriptor.iSerialNumber, langid, &string))
        {
            dev->serialnumber_name = AllocVecPooled(SD(cl)->MemPool, 1 + ((string.bLength - 2) >> 1));

            for (i=0; i < (string.bLength - 2) >> 1; i++) {
                dev->serialnumber_name[i] = AROS_LE2WORD(string.bString[i]);
            }
            dev->serialnumber_name[(string.bLength - 2) >> 1] = 0;

            D(bug("[USBDevice::New] iSerial = \"%s\"\n", dev->serialnumber_name));
        }
        else {
            dev->serialnumber_name = AllocVecPooled(SD(cl)->MemPool, 1 + strlen(unknown_serial));
            CopyMem(unknown_serial, dev->serialnumber_name, strlen(unknown_serial) + 1);
        }
    }

    D(bug("[USB] USBDevice::New() = %p\n",o));

    if (!o)
        BASE(cl->UserData)->LibNode.lib_OpenCnt--;

    return o;
}

BOOL METHOD(USBDevice, Hidd_USBDevice, GetDescriptor)
{
    USBDevice_Request req;
    BOOL ret;

    req.bmRequestType = UT_READ_DEVICE;
    req.bRequest = UR_GET_DESCRIPTOR;
    req.wValue = AROS_WORD2LE(msg->type << 8 | msg->index);
    req.wIndex = AROS_WORD2LE(0);
    req.wLength = AROS_WORD2LE(msg->length);

    ret = HIDD_USBDevice_ControlMessage(o, NULL, &req, msg->descriptor, msg->length);

    return ret;
}

BOOL METHOD(USBDevice, Hidd_USBDevice, GetConfigDescriptor)
{
    return HIDD_USBDevice_GetDescriptor(o, UDESC_CONFIG, msg->index, USB_CONFIG_DESCRIPTOR_SIZE, msg->descriptor);
}

BOOL METHOD(USBDevice, Hidd_USBDevice, GetDeviceDescriptor)
{
    return HIDD_USBDevice_GetDescriptor(o, UDESC_DEVICE, 0, USB_DEVICE_DESCRIPTOR_SIZE, msg->descriptor);
}

BOOL METHOD(USBDevice, Hidd_USBDevice, GetStatus)
{
    USBDevice_Request req;

    req.bmRequestType = UT_READ_DEVICE;
    req.bRequest = UR_GET_STATUS;
    req.wValue = AROS_WORD2LE(0);
    req.wIndex = AROS_WORD2LE(0);
    req.wLength = AROS_WORD2LE(sizeof(usb_status_t));

    return HIDD_USBDevice_ControlMessage(o, NULL, &req, msg->status, sizeof(usb_status_t));
}


void * METHOD(USBDevice, Hidd_USBDevice, CreatePipe)
{
    DeviceData *dev = OOP_INST_DATA(cl, o);

    return HIDD_USBDrv_CreatePipe(dev->bus, msg->type, dev->fast, dev->address, msg->endpoint, msg->period, msg->maxpacket ? msg->maxpacket : dev->maxpacket, msg->timeout);
}

void METHOD(USBDevice, Hidd_USBDevice, DeletePipe)
{
    DeviceData *dev = OOP_INST_DATA(cl, o);

    HIDD_USBDrv_DeletePipe(dev->bus, msg->pipe);
}

void METHOD(USBDevice, Hidd_USBDevice, SetTimeout)
{
    DeviceData *dev = OOP_INST_DATA(cl, o);

    HIDD_USBDrv_SetTimeout(dev->bus, msg->pipe ? msg->pipe : dev->default_pipe, msg->timeout);
}

BOOL METHOD(USBDevice, Hidd_USBDevice, ControlMessage)
{
    DeviceData *dev = OOP_INST_DATA(cl, o);
    APTR pipe = msg->pipe ? msg->pipe : dev->default_pipe;

    return HIDD_USBDrv_ControlTransfer(dev->bus, pipe, msg->request, msg->buffer, msg->length);
}

BOOL METHOD(USBDevice, Hidd_USBDevice, BulkTransfer)
{
    DeviceData *dev = OOP_INST_DATA(cl, o);
    APTR pipe = msg->pipe;

    if (pipe)
    	return HIDD_USBDrv_BulkTransfer(dev->bus, pipe, msg->buffer, msg->length);
    else
    	return FALSE;
}

static BOOL set_config(OOP_Object *o, int c)
{
    USBDevice_Request req;

    req.bmRequestType = UT_WRITE_DEVICE;
    req.bRequest = UR_SET_CONFIG;
    req.wValue = AROS_WORD2LE(c);
    req.wIndex = AROS_WORD2LE(0);
    req.wLength = AROS_WORD2LE(0);

    return HIDD_USBDevice_ControlMessage(o, NULL, &req, NULL, 0);
}

static usb_interface_descriptor_t *find_idesc(usb_config_descriptor_t *cd, int ifaceidx, int altidx)
{
    char *p = (char *)cd;
    char *end = p + AROS_LE2WORD(cd->wTotalLength);
    usb_interface_descriptor_t *d;
    int curidx, lastidx, curaidx = 0;

    for (curidx = lastidx = -1; p < end; ) {
        d = (usb_interface_descriptor_t *)p;
        D(bug("[USBDevice] find_idesc: idx=%d(%d) altidx=%d(%d) len=%d "
                "type=%d\n",
                ifaceidx, curidx, altidx, curaidx,
                d->bLength, d->bDescriptorType));
        if (d->bLength == 0) /* bad descriptor */
            break;
        p += d->bLength;
        if (p <= end && d->bDescriptorType == UDESC_INTERFACE) {
            if (d->bInterfaceNumber != lastidx) {
                lastidx = d->bInterfaceNumber;
                curidx++;
                curaidx = 0;
            } else
                curaidx++;
            if (ifaceidx == curidx && altidx == curaidx)
                return (d);
        }
    }
    return (NULL);
}

static inline usb_endpoint_descriptor_t *find_edesc(usb_config_descriptor_t *cd, int ifaceidx, int altidx,
                int endptidx)
{
    char *p = (char *)cd;
    char *end = p + AROS_WORD2LE(cd->wTotalLength);
    usb_interface_descriptor_t *d;
    usb_endpoint_descriptor_t *e;
    int curidx;

    d = find_idesc(cd, ifaceidx, altidx);
    if (d == NULL)
        return (NULL);
    if (endptidx >= d->bNumEndpoints) /* quick exit */
        return (NULL);

    curidx = -1;
    for (p = (char *)d + d->bLength; p < end; ) {
        e = (usb_endpoint_descriptor_t *)p;
        if (e->bLength == 0) /* bad descriptor */
            break;
        p += e->bLength;
        if (p <= end && e->bDescriptorType == UDESC_INTERFACE)
            return (NULL);
        if (p <= end && e->bDescriptorType == UDESC_ENDPOINT) {
            curidx++;
            if (curidx == endptidx)
                return (e);
        }
    }
    return (NULL);
}


static BOOL fill_iface(OOP_Class *cl, OOP_Object *o, int ifaceidx, int altidx)
{
    DeviceData *dev = OOP_INST_DATA(cl, o);
    char *p, *end;
    InterfaceData  *ifc = &dev->interfaces[ifaceidx];
    usb_interface_descriptor_t *idesc;
    int endpt, nendpt;

    D(bug("[USBDevice] fill_iface: ifaceidx=%d altidx=%d\n",
            ifaceidx, altidx));

    idesc = find_idesc(dev->config_desc, ifaceidx, altidx);
    if (idesc == NULL)
            return FALSE;

    ifc->interface = idesc;
    ifc->index = ifaceidx;
    ifc->altindex = altidx;

    nendpt = ifc->interface->bNumEndpoints;

    D(bug("[USBDevice] fill_iface: found idesc nendpt=%d\n", nendpt));

    if (nendpt != 0)
    {
        ifc->endpoints = AllocVecPooled(SD(cl)->MemPool, nendpt * sizeof(EndpointData));
        if (ifc->endpoints == NULL)
                return FALSE;
    } else
        ifc->endpoints = NULL;

    p = (char *)ifc->interface + ifc->interface->bLength;
    end = (char *)dev->config_desc + AROS_LE2WORD(dev->config_desc->wTotalLength);
#define ed ((usb_endpoint_descriptor_t *)p)

    for (endpt = 0; endpt < nendpt; endpt++)
    {
        D(bug("[USBDevice] fill_iface: endpt=%d\n", endpt));
        for (; p < end; p += ed->bLength) {
            D(bug("[USBDevice] fill_iface: p=%p end=%p "
                    "len=%d type=%d\n",
                    p, end, ed->bLength, ed->bDescriptorType));
            if (p + ed->bLength <= end && ed->bLength != 0 &&
                    ed->bDescriptorType == UDESC_ENDPOINT)
                goto found;
            if (ed->bLength == 0 ||
                    ed->bDescriptorType == UDESC_INTERFACE)
                break;
        }
        /* passed end, or bad desc */
        bug("[USBDevice] fill_iface: bad descriptor(s): %s\n",
                ed->bLength == 0 ? "0 length" :
                ed->bDescriptorType == UDESC_INTERFACE ? "iface desc":"out of data");
        goto bad;
    found:
        D(bug("[USBDevice] fill_iface: endpoint descriptor %p\n", ed));
        ifc->endpoints[endpt].endpoint = ed;
        p += ed->bLength;
    }
    return TRUE;

bad:
    if (ifc->endpoints != NULL) {
        FreeVecPooled(SD(cl)->MemPool, ifc->endpoints);
        ifc->endpoints = NULL;
    }
    return FALSE;
}

static void free_iface(OOP_Class *cl, OOP_Object *o, int i)
{
    DeviceData *dev = OOP_INST_DATA(cl, o);
    InterfaceData *iface = &dev->interfaces[i];
    if (iface->endpoints)
        FreeVecPooled(SD(cl)->MemPool, iface->endpoints);
}

BOOL METHOD(USBDevice, Hidd_USBDevice, Configure)
{
    DeviceData *dev = OOP_INST_DATA(cl, o);
    usb_config_descriptor_t cd, *cdp;
    int i, length;
    BOOL err;

    D(bug("[USBDevice::Configure] Configure(%d)\n", msg->configNr));

    if (dev->config != USB_UNCONFIG_NO)
    {
        int i, ifaces = dev->config_desc->bNumInterface;
        for (i=0; i < ifaces; i++)
            free_iface(cl, o, i);
        FreeVecPooled(SD(cl)->MemPool, dev->interfaces);
        FreeVecPooled(SD(cl)->MemPool, dev->config_desc);
        dev->interfaces = NULL;
        dev->config_desc = NULL;
        dev->config = USB_UNCONFIG_NO;
    }

    if (msg->configNr == USB_UNCONFIG_INDEX)
    {
        D(bug("[USBDevice::Configure] Unconfiguring "));
        if ((err = set_config(o, USB_UNCONFIG_NO)))
            D(bug("with success\n"));
        else
            D(bug("with ERROR\n"));
        return err;
    }

    HIDD_USBDevice_GetConfigDescriptor(o, 0, &cd);
    length = AROS_LE2WORD(cd.wTotalLength);

    D(bug("[USBDevice::Configure] Fetching config descriptor of length %d\n", length));

    cdp = AllocVecPooled(SD(cl)->MemPool, length);
    if (cdp == NULL)
        return FALSE;

    for (i=0; i < 3; i++)
    {
        if (HIDD_USBDevice_GetDescriptor(o, UDESC_CONFIG, msg->configNr, length, cdp))
            break;
        D(bug("[USBDevice::Configure]  retry...\n"));
        USBDelay(dev->tr, 200);
    }

    if (i == 3)
    {
        D(bug("[USBDevice::Configure]  failed...\n"));
    }

    /* TODO: Power! Self powered? */
    err = set_config(o, cdp->bConfigurationValue);

    D(bug("[USBDevice::Configure] Allocating %d interfaces\n", cdp->bNumInterface ));
    dev->interfaces = AllocVecPooled(SD(cl)->MemPool, sizeof(InterfaceData) * cdp->bNumInterface);
    if (!dev->interfaces)
    {
        /* NOMEM! */
    }

    dev->config_desc = cdp;
    dev->config = cdp->bConfigurationValue;

    for (i = 0; i < cdp->bNumInterface; i++)
    {
        err = fill_iface(cl, o, i, 0);
        if (!err)
        {
            D(bug("[USBDevice::Configure] error at interface %d\n", i));
            while (--i >= 0)
                free_iface(cl, o, i);
            return FALSE;
        }
    }

    return TRUE;
}

usb_interface_descriptor_t * METHOD(USBDevice, Hidd_USBDevice, GetInterface)
{
    DeviceData *dev = OOP_INST_DATA(cl, o);
    usb_interface_descriptor_t *d = NULL;

    if (dev->config != USB_UNCONFIG_NO)
    {
        if (msg->interface < dev->config_desc->bNumInterface)
        {
            d = dev->interfaces[msg->interface].interface;
        }
    }

    return d;
}

usb_endpoint_descriptor_t * METHOD(USBDevice, Hidd_USBDevice, GetEndpoint)
{
    DeviceData *dev = OOP_INST_DATA(cl, o);
    usb_endpoint_descriptor_t *d = NULL;

    if (dev->config != USB_UNCONFIG_NO)
    {
        if (msg->interface < dev->config_desc->bNumInterface)
        {
            if (msg->endpoint < dev->interfaces[msg->interface].interface->bNumEndpoints)
            {
                d = dev->interfaces[msg->interface].endpoints[msg->endpoint].endpoint;
            }
        }
    }

    DumpDescriptor((usb_descriptor_t *)d);

    return d;
}

void METHOD(USBDevice, Root, Dispose)
{
    DeviceData *dev = OOP_INST_DATA(cl, o);
    struct Library *base = &BASE(cl->UserData)->LibNode;

    D(bug("[USB] USBDevice::Dispose\n"));

    if (dev->next)
        OOP_DisposeObject(dev->next);

//   Do not unconfigure. The device may not exist already...
//    HIDD_USBDevice_Configure(o, USB_UNCONFIG_INDEX);

    if (dev->product_name && dev->product_name != unknown_name)
        FreeVecPooled(SD(cl)->MemPool, dev->product_name);

    if (dev->manufacturer_name && dev->manufacturer_name != unknown_manufacturer)
        FreeVecPooled(SD(cl)->MemPool, dev->manufacturer_name);

    if (dev->serialnumber_name && dev->serialnumber_name != unknown_serial)
        FreeVecPooled(SD(cl)->MemPool, dev->serialnumber_name);

    if (dev->default_pipe)
        HIDD_USBDrv_DeletePipe(dev->bus, dev->default_pipe);

    USBDeleteTimer(dev->tr);

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    base->lib_OpenCnt--;
}

void METHOD(USBDevice, Root, Get)
{
    uint32_t idx;
    DeviceData *dev = OOP_INST_DATA(cl, o);

    if (IS_USBDEVICE_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_USBDevice_Address:
                *msg->storage = dev->address;
                break;

            case aoHidd_USBDevice_Bus:
                *msg->storage = (intptr_t)dev->bus;
                break;

            case aoHidd_USBDevice_Hub:
                *msg->storage = (intptr_t)dev->hub;
                break;

            case aoHidd_USBDevice_Fast:
                *msg->storage = dev->fast;
                break;

            case aoHidd_USBDevice_MaxPacketSize:
                *msg->storage = dev->maxpacket;
                break;

            case aoHidd_USBDevice_ProductID:
                *msg->storage = AROS_LE2WORD(dev->descriptor.idProduct);
                break;

            case aoHidd_USBDevice_VendorID:
                *msg->storage = AROS_LE2WORD(dev->descriptor.idVendor);
                break;

            case aoHidd_USBDevice_ProductName:
                *msg->storage = (intptr_t)dev->product_name;
                break;

            case aoHidd_USBDevice_ManufacturerName:
                *msg->storage = (intptr_t)dev->manufacturer_name;
                break;

            case aoHidd_USBDevice_SerialNumber:
                *msg->storage = (intptr_t)dev->serialnumber_name;
                break;

            case aoHidd_USBDevice_Next:
                *msg->storage = (intptr_t)dev->next;
                break;

            case aoHidd_USBDevice_Interface:
                *msg->storage = (intptr_t)dev->iface;
                break;

            case aoHidd_USBDevice_InterfaceNumber:
                *msg->storage = dev->interfaces[dev->iface].interface->bInterfaceNumber;
                break;

            default:
                OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
                break;
        }
    }
    else
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

void METHOD(USBDevice, Root, Set)
{
    uint32_t idx;
    struct TagItem *tag;
    struct TagItem *tags = msg->attrList;

    while ((tag = NextTagItem(&tags)))
    {
        if (IS_USBDEVICE_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_USBDevice_Address:
                    usb_SetAddress(cl, o, tag->ti_Data);
                    break;

                default:
                    break;
            }

        }
        else
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
