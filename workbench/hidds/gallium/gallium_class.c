/*
    Copyright 2010-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/oop.h>
#include <proto/utility.h>

#include "gallium_intern.h"

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (SD(cl)->galliumAttrBase)

/* METHODS */
OOP_Object *METHOD(Gallium, Root, New)
{
    IPTR interfaceVers;

    D(bug("[gallium.hidd] %s()\n", __func__));

    interfaceVers = GetTagData(aHidd_Gallium_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != GALLIUM_INTERFACE_VERSION)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    return o;
}

VOID METHOD(Gallium, Root, Get)
{
    ULONG idx;

    if (IS_GALLIUM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_Gallium_InterfaceVersion:
                *msg->storage = GALLIUM_INTERFACE_VERSION;
                return;
        }
    }
    
    /* Use parent class for all other properties */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

APTR METHOD(Gallium, Hidd_Gallium, CreatePipeScreen)
{
    D(bug("[gallium.hidd] %s()\n", __func__));
    return NULL;
}

VOID METHOD(Gallium, Hidd_Gallium, DestroyPipeScreen)
{
    D(bug("[gallium.hidd] %s()\n", __func__));
}

VOID METHOD(Gallium, Hidd_Gallium, DisplayResource)
{
    D(bug("[gallium.hidd] %s()\n", __func__));
}

