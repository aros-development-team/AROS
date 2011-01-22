/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gallium_intern.h"

#include <aros/debug.h>
#include <proto/oop.h>

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (SD(cl)->galliumAttrBase)

/* METHODS */
OOP_Object *METHOD(Gallium, Root, New)
{
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
            /* Overload the property */
            case aoHidd_Gallium_GalliumInterfaceVersion:
                *msg->storage = GALLIUM_INTERFACE_VERSION;
                return;
        }
    }
    
    /* Use parent class for all other properties */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

APTR METHOD(Gallium, Hidd_Gallium, CreatePipeScreen)
{
    bug("[gallium.hidd] Abstract CreatePipeScreen called\n");
    return NULL;
}

VOID METHOD(Gallium, Hidd_Gallium, DisplaySurface)
{
    bug("[gallium.hidd] Abstract DisplaySurface called. This method is OBSOLETE.\n");
}

VOID METHOD(Gallium, Hidd_Gallium, DisplayResource)
{
    bug("[gallium.hidd] Abstract DisplaySurface called\n");
}

