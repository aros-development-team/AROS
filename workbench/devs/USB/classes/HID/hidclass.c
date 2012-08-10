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

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <usb/usb.h>
#include <usb/usb_core.h>
#include <usb/hid.h>
#include "hid.h"

#include <proto/oop.h>
#include <proto/dos.h>

static usb_interface_descriptor_t *find_idesc(usb_config_descriptor_t *cd, int ifaceidx, int altidx)
{
    char *p = (char *)cd;
    char *end = p + AROS_LE2WORD(cd->wTotalLength);
    usb_interface_descriptor_t *d;
    int curidx, lastidx, curaidx = 0;

    for (curidx = lastidx = -1; p < end; ) {
        d = (usb_interface_descriptor_t *)p;

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

static AROS_UFIH1(HidInterrupt, HidData *, hid)
{
    AROS_USERFUNC_INIT

    uint8_t reportid = 0;

    /* Invalidate the cache. Report has been sent through DMA */
    CacheClearE(hid->buffer, hid->buflen, CACRF_InvalidateD);

    if (hid->nreport != 1)
    {
        reportid = hid->buffer[0];

        /* And let the class handle it */
        HIDD_USBHID_ParseReport(hid->o, reportid, &hid->buffer[1], hid->buflen);
    }
    else
        HIDD_USBHID_ParseReport(hid->o, 0, hid->buffer, hid->buflen);

    return 0;

    AROS_USERFUNC_EXIT
}

BOOL METHOD(HID, Hidd_USBHID, GetReportDescriptor)
{
    USBDevice_Request req;
    intptr_t ifnr;

    OOP_GetAttr(o, aHidd_USBDevice_InterfaceNumber, &ifnr);

    req.bmRequestType = UT_READ_INTERFACE;
    req.bRequest = UR_GET_DESCRIPTOR;
    req.wValue = AROS_WORD2LE(UDESC_REPORT << 8 | 0);
    req.wIndex = AROS_WORD2LE(ifnr);
    req.wLength = AROS_WORD2LE(msg->length);

    return HIDD_USBDevice_ControlMessage(o, NULL, &req, msg->buffer, msg->length);
}

BOOL METHOD(HID, Hidd_USBHID, SetIdle)
{
    USBDevice_Request req;
    intptr_t ifnr;

    D(bug("[HID] HID::SetIdle(%d, %d)\n", msg->duration, msg->id));

    OOP_GetAttr(o, aHidd_USBDevice_InterfaceNumber, &ifnr);

    req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
    req.bRequest = UR_SET_IDLE;
    req.wValue = AROS_WORD2LE(msg->duration << 8| msg->id);
    req.wIndex = AROS_WORD2LE(ifnr);
    req.wLength = AROS_WORD2LE(0);

    return HIDD_USBDevice_ControlMessage(o, NULL, &req, NULL, 0);
}

BOOL METHOD(HID, Hidd_USBHID, SetReport)
{
    USBDevice_Request req;
    intptr_t ifnr;

    D({
        uint8_t *b = msg->report;
        int i;

        bug("[HID] HID::SetReport(%d, %d, \"", msg->type, msg->id);
        for (i=0; i < msg->length; i++)
            bug("%02x", b[i]);
        bug("\")\n");
    });

    OOP_GetAttr(o, aHidd_USBDevice_InterfaceNumber, &ifnr);

    req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
    req.bRequest = UR_SET_REPORT;
    req.wValue = AROS_WORD2LE(msg->type << 8| msg->id);
    req.wIndex = AROS_WORD2LE(ifnr);
    req.wLength = AROS_WORD2LE(msg->length);

    return HIDD_USBDevice_ControlMessage(o, NULL, &req, msg->report, msg->length);
}

BOOL METHOD(HID, Hidd_USBHID, SetProtocol)
{
    USBDevice_Request req;
    intptr_t ifnr;

    D(bug("[HID] HID::SetProtocol(%s)\n", msg->protocol ? "Report":"Boot"));

    OOP_GetAttr(o, aHidd_USBDevice_InterfaceNumber, &ifnr);

    req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
    req.bRequest = UR_SET_PROTOCOL;
    req.wValue = AROS_WORD2LE(msg->protocol);
    req.wIndex = AROS_WORD2LE(ifnr);
    req.wLength = AROS_WORD2LE(0);

    return HIDD_USBDevice_ControlMessage(o, NULL, &req, NULL, 0);
}

void METHOD(HID, Hidd_USBHID, ParseReport)
{
    HidData *hid = OOP_INST_DATA(cl, o);
    int i;
    uint8_t *data = hid->buffer;
    int len = hid->buflen;

    if (hid->nreport != 1)
    {
        data++;
        len--;
    }

    D(bug("[HID] Unknown report id %d: ", msg->id));

    for (i=0; i < len; i++)
        D(bug("%02x ", data[i]));

    D(bug("\n"));
}

usb_hid_descriptor_t *METHOD(HID, Hidd_USBHID, GetHidDescriptor)
{
    HidData *hid = OOP_INST_DATA(cl, o);
    usb_hid_descriptor_t *hdesc = NULL, *hd;
    char *p, *end;
    intptr_t iface;

    OOP_GetAttr(o, aHidd_USBDevice_Interface, &iface);

    if (hid->cdesc)
    {
        usb_interface_descriptor_t *idesc = find_idesc(hid->cdesc, iface, 0);
        D(bug("[HID] cdesc=%p, idesc=%p\n", hid->cdesc, idesc));

        if (!idesc)
            return NULL;

        p = (char *)idesc + idesc->bLength;
        end = (char *)hid->cdesc + AROS_LE2WORD(hid->cdesc->wTotalLength);

        for (; p < end; p += hd->bLength)
        {
            hd = (usb_hid_descriptor_t *)p;
            D(bug("[HID] p=%p bLength=%d bDescriptorType=%x\n", p, hd->bLength, hd->bDescriptorType));

            if (p + hd->bLength <= end && hd->bDescriptorType == UDESC_HID)
            {
                hdesc = hd;
                break;
            }
            if (hd->bDescriptorType == UDESC_INTERFACE)
                break;
        }
    }
    D(bug("[HID] hdesc=%p\n", hdesc));
    return hdesc;
}

OOP_Object *METHOD(HID, Root, New)
{
    D(bug("[HID] HID::New()\n"));

    BASE(cl->UserData)->LibNode.lib_OpenCnt++;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        HidData *hid = OOP_INST_DATA(cl, o);
        usb_config_descriptor_t cdesc;
        intptr_t iface;

        OOP_GetAttr(o, aHidd_USBDevice_Interface, &iface);

        hid->sd = SD(cl);
        hid->o = o;

        D(bug("[HID::New()] HidData=%p Configuring the device...\n", hid));

        /* Configure the HID device */
        HIDD_USBDevice_Configure(o, 0);

        D(bug("[HID::New()] Getting device and config descriptors...\n"));
        HIDD_USBDevice_GetDeviceDescriptor(o, &hid->ddesc);
        HIDD_USBDevice_GetConfigDescriptor(o, 0, &cdesc);

        HIDD_USBHID_SetIdle(o, 0, 0);

        if (AROS_LE2WORD(cdesc.wTotalLength))
            hid->cdesc = AllocVecPooled(SD(cl)->MemPool, AROS_LE2WORD(cdesc.wTotalLength));

        if (hid->cdesc)
        {
            uint16_t repid;

            D(bug("[HID::New()] Getting config descriptor of size %d...\n",AROS_LE2WORD(cdesc.wTotalLength)));

            HIDD_USBDevice_GetDescriptor(o, UDESC_CONFIG, 0, AROS_LE2WORD(cdesc.wTotalLength), hid->cdesc);
            hid->hd = HIDD_USBHID_GetHidDescriptor(o);
            if (hid->hd) {
                D(bug("[HID::New()] Hid descriptor @ %p\n", hid->hd));
                D(bug("[HID::New()] Number of Report descriptors: %d\n", hid->hd->bNumDescriptors));
                hid->reportLength = AROS_LE2WORD(hid->hd->descrs[0].wDescriptorLength);
                hid->report = AllocVecPooled(SD(cl)->MemPool, hid->reportLength);

                D(bug("[HID::New()] Getting report descriptor 1 of size %d\n", hid->reportLength));

                HIDD_USBHID_GetReportDescriptor(o, hid->reportLength, hid->report);

                hid->nreport = hid_maxrepid(hid->report, hid->reportLength) + 1;
                hid->buflen = 0;

                for (repid = 0; repid < hid->nreport; repid++)
                {
                    uint16_t repsize = hid_report_size(hid->report, hid->reportLength, hid_input, repid);
                    D(bug("[HID::New()] Report %d: size %d\n", repid, repsize));
                    if (repsize > hid->buflen)
                        hid->buflen = repsize;
                }

                /* If there is more than one report ID, make a space for report ID */
                if (hid->nreport != 1)
                    hid->buflen++;

                hid->buffer = AllocVecPooled(SD(cl)->MemPool, hid->buflen);

                D(bug("[HID::New()] Length of input report is %d\n", hid->buflen));

                usb_endpoint_descriptor_t *ep = HIDD_USBDevice_GetEndpoint(o, iface, 0);

                D(bug("[HID::New()] Endpoint descriptor %p addr %02x\n", ep, ep->bEndpointAddress));

                if (ep)
                {
                    if ((ep->bmAttributes & UE_XFERTYPE) != UE_INTERRUPT)
                    {
                        bug("[HID::New()] Wrong endpoint type\n");
    // TODO: unconfigure, error, coercemethod
                    }

                    OOP_Object *drv = NULL;
                    OOP_GetAttr(o, aHidd_USBDevice_Bus, (IPTR *)&drv);

                    if (drv)
                    {
                        hid->interrupt.is_Data = hid;
                        hid->interrupt.is_Code = (VOID_FUNC)HidInterrupt;
                        hid->intr_pipe = HIDD_USBDevice_CreatePipe(o, PIPE_Interrupt, ep->bEndpointAddress, ep->bInterval, AROS_LE2WORD(ep->wMaxPacketSize), 0);
                        if (hid->intr_pipe) {
                            HIDD_USBDrv_AddInterrupt(drv, hid->intr_pipe, hid->buffer, hid->buflen, &hid->interrupt);
                            D(bug("[HID] HID::New() = %p\n", o));
                            return o;
                        }
                    }
                }
                if (hid->report)
                    FreeVecPooled(SD(cl)->MemPool, hid->report);
                if (hid->buffer)
                    FreeVecPooled(SD(cl)->MemPool, hid->buffer);
            }
            FreeVecPooled(SD(cl)->MemPool, hid->cdesc);
        }
    }

    D(bug("[HID] HID::New() = NULL\n"));

    BASE(cl->UserData)->LibNode.lib_OpenCnt--;

    return NULL;
}

void METHOD(HID, Root, Dispose)
{
    HidData *hid = OOP_INST_DATA(cl, o);
    OOP_Object *drv = NULL;
    struct Library *base = &BASE(cl->UserData)->LibNode;

    OOP_GetAttr(o, aHidd_USBDevice_Bus, (intptr_t *)&drv);

    HIDD_USBDrv_RemInterrupt(drv, hid->intr_pipe, &hid->interrupt);
    HIDD_USBDevice_DeletePipe(o, hid->intr_pipe);

    if (hid->report)
        FreeVecPooled(SD(cl)->MemPool, hid->report);
    if (hid->cdesc)
        FreeVecPooled(SD(cl)->MemPool, hid->cdesc);
    if (hid->buffer)
        FreeVecPooled(SD(cl)->MemPool, hid->buffer);

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    base->lib_OpenCnt--;
}

/*
 * The MatchCLID library call gets the device descriptor and full config
 * descriptor (including the interface and endpoint descriptors). It will
 * return CLID_Hidd_USBHID if the requested interface conforms to HID.
 */
AROS_LH3(void *, MatchCLID,
         AROS_LHA(usb_device_descriptor_t *, dev,       A0),
         AROS_LHA(usb_config_descriptor_t *, cfg,       A1),
         AROS_LHA(int,                       i,         D0),
         LIBBASETYPEPTR, LIBBASE, 5, HID)
{
    AROS_LIBFUNC_INIT

    void *clid = NULL;

    D(bug("[HID] MatchCLID(%p, %p)\n", dev, cfg));

    if (dev->bDeviceClass == UDCLASS_IN_INTERFACE)
    {
        usb_interface_descriptor_t *iface = find_idesc(cfg, i, 0);

        D(bug("[HID] UDCLASS_IN_INTERFACE OK. Checking interface %d\n", i));
        D(bug("[HID] iface %d @ %p protocol %d\n", i, iface, iface->bInterfaceProtocol));

        if (iface->bInterfaceClass == UICLASS_HID)
        {
            D(bug("[HID] HID Class found. Handling it.\n"));

            switch (iface->bInterfaceProtocol)
            {
                case 1:
                    D(bug("[HID] Keyboard protocol\n"));
                    clid = CLID_Hidd_USBKeyboard;
                    break;

                case 2:
                    D(bug("[HID] Mouse protocol\n"));
                    clid = CLID_Hidd_USBMouse;
                    break;

                default:
                    D(bug("[HID] Protocol unknown, using default class\n"));
                    clid = CLID_Hidd_USBHID;
                    break;
            }
        }
    }

    return clid;

    AROS_LIBFUNC_EXIT
}



