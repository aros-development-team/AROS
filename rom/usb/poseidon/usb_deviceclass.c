/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/utility.h>

#include <hidd/usb.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "debug.h"

#include "poseidon.library.h"

OOP_Object *USBDevice__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    OOP_Object *usbDevice = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (usbDevice)
    {
        struct USBDevice *data = OOP_INST_DATA(cl, usbDevice);
        D(bug ("[USB:Device] %s: Device Entry @ 0x%p\n", __func__, data);)
    }
    return usbDevice;
}

VOID USBDevice__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    return;
}

void USBDevice__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
