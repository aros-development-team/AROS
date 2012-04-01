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

#include <exec/types.h>
#include <oop/oop.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <hidd/hidd.h>
#include <usb/usb.h>

#include <proto/oop.h>
#include <proto/dos.h>

#include <stdint.h>

#include "usb.h"
#include "misc.h"

OOP_Object *METHOD(USB, Root, New)
{
    D(bug("[USB] USB::New()\n"));

    BASE(cl->UserData)->LibNode.lib_OpenCnt++;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    D(bug("[USB] USB::New() = %p\n", o));

    if (!o)
        BASE(cl->UserData)->LibNode.lib_OpenCnt--;

    return o;
}

struct pRoot_Dispose {
    OOP_MethodID        mID;
};

void METHOD(USB, Root, Dispose)
{
    struct Library *base = &BASE(cl->UserData)->LibNode;

    D(bug("[USB] USB::Dispose\n"));

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    base->lib_OpenCnt--;
}

void METHOD(USB, Root, Get)
{
    uint32_t idx;
    D(bug("[USB] USB::Get\n"));

    if (IS_USB_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_USB_Bus:
            {
                D(bug("[USB] USB Get Bus. *msg->storage = %p\n", *msg->storage));
                usb_driver_t *driver;

                ObtainSemaphore(&SD(cl)->driverListLock);

                if (*msg->storage)
                {
                    ForeachNode(&SD(cl)->driverList, driver)
                    {
                        if ((IPTR)driver->d_Driver == *msg->storage)
                            break;
                    }

                    if (driver)
                        driver = (usb_driver_t *)GetSucc(&driver->d_Node);
                }
                else
                    driver = (usb_driver_t *)GetHead(&SD(cl)->driverList);

                D(bug("[USB] driver=%p\n", driver));

                if (driver)
                    *msg->storage = (IPTR)driver->d_Driver;
                else
                    *msg->storage = (IPTR)NULL;

                ReleaseSemaphore(&SD(cl)->driverListLock);
                break;
            }
            default:
                OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
    }
    else
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL METHOD(USB, Hidd_USB, AttachDriver)
{
    BOOL retval = FALSE;
    D(bug("[USB] USB::AttachDriver(%p)\n", msg->driverObject));

    if (msg->driverObject)
    {
        usb_driver_t *drv = AllocPooled(SD(cl)->MemPool, sizeof(struct usb_driver));

        if (drv)
        {
            int i;
            drv->d_Driver = msg->driverObject;
            InitSemaphore(&drv->d_Lock);

            for (i=0; i < (BITMAP_SIZE/32); i++) {
                drv->bitmap[i] = 0;
            }
            setBitmap(drv->bitmap, 0);
            setBitmap(drv->bitmap, 1);

            ObtainSemaphore(&SD(cl)->driverListLock);
            AddTail(&SD(cl)->driverList, &drv->d_Node);
            ReleaseSemaphore(&SD(cl)->driverListLock);

            //HIDD_USBHub_OnOff(drv->d_Driver, FALSE);
            HIDD_USBHub_OnOff(drv->d_Driver, TRUE);

            retval = TRUE;
        }
    }


    return retval;
}

BOOL METHOD(USB, Hidd_USB, DetachDriver)
{
    D(bug("[USB] USB::DetachDriver()\n"));

    return FALSE;
}

void METHOD(USB, Hidd_USB, AddClass)
{
    struct usb_ExtClass *ec = NULL;
    int found = 0;

    D(bug("[USB] USB::AddClass(\"%s\")\n", msg->className));

    ForeachNode(&SD(cl)->extClassList, ec)
    {
        if (!strcmp(msg->className, ec->ec_ShortName))
        {
            found = 1;
            break;
        }
    }

    if (!found)
    {
        D(bug("[USB] Class not on the list. Adding it.\n"));

        ec = AllocVecPooled(SD(cl)->MemPool, sizeof(struct usb_ExtClass));

        ec->ec_Node.ln_Name = AllocVecPooled(SD(cl)->MemPool, strlen(msg->className)+1);
        CopyMem(msg->className, ec->ec_Node.ln_Name, strlen(msg->className)+1);
        ec->ec_ShortName = AllocVecPooled(SD(cl)->MemPool, strlen(msg->className)+1);
        CopyMem(msg->className, (char *)ec->ec_ShortName, strlen(msg->className)+1);
        AddTail(&SD(cl)->extClassList, &ec->ec_Node);
    }
}

uint8_t METHOD(USB, Hidd_USB, AllocAddress)
{
    struct usb_driver *drv = NULL;
    uint8_t addr = 0;

    ObtainSemaphore(&SD(cl)->driverListLock);
    ForeachNode(&SD(cl)->driverList, drv)
    {
        if (drv->d_Driver == msg->driverObject)
            break;
    }
    ReleaseSemaphore(&SD(cl)->driverListLock);

    if (drv)
    {
        ObtainSemaphore(&drv->d_Lock);
        addr = allocBitmap(drv->bitmap);
        ReleaseSemaphore(&drv->d_Lock);
    }

    return addr;
}

void METHOD(USB, Hidd_USB, FreeAddress)
{
    struct usb_driver *drv = NULL;

    ObtainSemaphore(&SD(cl)->driverListLock);
    ForeachNode(&SD(cl)->driverList, drv)
    {
        if (drv->d_Driver == msg->driverObject)
            break;
    }
    ReleaseSemaphore(&SD(cl)->driverListLock);

    if (drv)
    {
        ObtainSemaphore(&drv->d_Lock);
        freeBitmap(drv->bitmap, msg->address);
        ReleaseSemaphore(&drv->d_Lock);
    }
}

OOP_Object *METHOD(USB, Hidd_USB, NewDevice)
{
    OOP_Object *new_device = NULL;
    APTR pipe;
    OOP_Object *bus;
    usb_device_descriptor_t descriptor;
    usb_config_descriptor_t config;
    void *cdesc;
    uint8_t address;

    USBDevice_Request request = {
            bmRequestType:      UT_READ_DEVICE,
            bRequest:           UR_GET_DESCRIPTOR,
            wValue:             AROS_WORD2LE(UDESC_DEVICE << 8),
            wIndex:             AROS_WORD2LE(0),
            wLength:            AROS_WORD2LE(8)
    };

    memset(&descriptor, 0, sizeof(descriptor));

    OOP_GetAttr(msg->hub, aHidd_USBDevice_Bus, (IPTR*)&bus);

    if (bus)
    {
        struct usb_ExtClass *ec;
        struct usb_driver *drv = NULL;

        ObtainSemaphore(&SD(cl)->driverListLock);
        ForeachNode(&SD(cl)->driverList, drv)
        {
            if (drv->d_Driver == bus)
                break;
        }
        ReleaseSemaphore(&SD(cl)->driverListLock);

        ObtainSemaphore(&drv->d_Lock);

        pipe = HIDD_USBDrv_CreatePipe(bus, PIPE_Control, msg->fast, 0, 0, 0, 8, 100);

        HIDD_USBDrv_ControlTransfer(bus, pipe, &request, &descriptor, 8);
        D(bug("[USB] USB::NewDevice()\n"));
        D(
          DumpDescriptor(&descriptor);
        );

        if ((address = HIDD_USB_AllocAddress(o, bus)))
        {
            USBDevice_Request req = {
                    bmRequestType:  UT_WRITE_DEVICE,
                    bRequest:       UR_SET_ADDRESS,
                    wValue:         AROS_WORD2LE(address),
                    wIndex:         AROS_WORD2LE(0),
                    wLength:        AROS_WORD2LE(0)
            };

            HIDD_USBDrv_ControlTransfer(bus, pipe, &req, NULL, 0);

            HIDD_USBDrv_DeletePipe(bus, pipe);

            pipe = HIDD_USBDrv_CreatePipe(bus, PIPE_Control, msg->fast, address, 0, 0, descriptor.bMaxPacketSize, 100);

            if (!pipe)
            {
                bug("[USB] Could not set device address\n");
                return NULL;
            }
        }

        request.wValue = AROS_WORD2LE(UDESC_CONFIG << 8);
        request.wLength = AROS_WORD2LE(USB_CONFIG_DESCRIPTOR_SIZE);

        HIDD_USBDrv_ControlTransfer(bus, pipe, &request, &config, USB_CONFIG_DESCRIPTOR_SIZE);

        cdesc = AllocVecPooled(SD(cl)->MemPool, AROS_LE2WORD(config.wTotalLength));
        if (cdesc)
        {
            request.wLength = config.wTotalLength;
            HIDD_USBDrv_ControlTransfer(bus, pipe, &request, cdesc, AROS_LE2WORD(config.wTotalLength));
        }

        HIDD_USBDrv_DeletePipe(bus, pipe);

        struct TagItem tags[] = {
                { aHidd_USBDevice_Interface,        0 },
                { aHidd_USBDevice_Address,          address },
                { aHidd_USBDevice_Next,             0 },
                { aHidd_USBDevice_Hub,              (uintptr_t)msg->hub },
                { aHidd_USBDevice_Fast,             msg->fast },
                { aHidd_USBDevice_MaxPacketSize,    descriptor.bMaxPacketSize },
                { TAG_DONE, 0UL },
        };

        switch(descriptor.bDeviceClass)
        {
            case UDCLASS_HUB:
                new_device = OOP_NewObject(NULL, (STRPTR)CLID_Hidd_USBHub, tags);
                HIDD_USBHub_OnOff(new_device, TRUE);
                break;

            default:
            {
                int i;

                /* Try a match for every interface */
                for (i = config.bNumInterface; i > 0; i--)
                {
                    D(bug("[USB] Looking for a driver for interface no. %d\n", i));
                    tags[0].ti_Data = i - 1;

                    ForeachNode(&SD(cl)->extClassList, ec)
                    {
                        D(bug("[USB] Trying external class \"%s\"\n", ec->ec_Node.ln_Name));

                        if (!ec->ec_LibBase)
                            ec->ec_LibBase = OpenLibrary(ec->ec_Node.ln_Name, 0);

                        if (ec->ec_LibBase)
                        {
                            void *clid = AROS_LVO_CALL3(void *,
                                                          AROS_LCA(usb_device_descriptor_t *, &descriptor, A0),
                                                          AROS_LCA(usb_config_descriptor_t *, cdesc, A1),
                                                          AROS_LCA(int, i - 1, D0),
                                                          struct Library *, ec->ec_LibBase, 5,);
                            if (clid)
                            {
                                new_device = OOP_NewObject(NULL, (STRPTR)clid, tags);

                                if (new_device)
                                {
                                    tags[2].ti_Data = (intptr_t)new_device;
                                    break;      /* One loop up */
                                }
                            }
                        }
                    }
                }

                break;
            }
        }

        if (cdesc)
            FreeVecPooled(SD(cl)->MemPool, cdesc);

        ReleaseSemaphore(&drv->d_Lock);
    }

    return new_device;
}
