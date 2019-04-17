/*
    Copyright 2010-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/oop.h>
#include <proto/utility.h>

#include <gallium/gallium.h>

#include "gallium_intern.h"
#include "pipe/p_state.h"

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (SD(cl)->galliumAttrBase)

/* METHODS */
OOP_Object *METHOD(HiddGallium, Root, New)
{
    D(bug("[Gallium] %s()\n", __PRETTY_FUNCTION__));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    if (o)
    {
#if (0)
        struct HiddGalliumData * HiddGallium_DATA = OOP_INST_DATA(cl, o);
#endif
    }

    return o;
}

VOID METHOD(HiddGallium, Root, Get)
{
#if (0)
    struct HiddGalliumData * HiddGallium_DATA = OOP_INST_DATA(cl, o);
#endif
    ULONG idx;

    D(bug("[Gallium] %s()\n", __PRETTY_FUNCTION__));

    if (IS_GALLIUM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_Gallium_InterfaceVersion:
                *msg->storage = 0;                                      // subclass should provide this!
                return;
        }
    }

    /* Use parent class for all other properties */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

APTR METHOD(HiddGallium, Hidd_Gallium, CreatePipeScreen)
{
    D(bug("[Gallium] %s()\n", __PRETTY_FUNCTION__));

    return NULL;
}

VOID METHOD(HiddGallium, Hidd_Gallium, DestroyPipeScreen)
{
    D(bug("[Gallium] %s()\n", __PRETTY_FUNCTION__));
}

#if (0)
VOID METHOD(HiddGallium, Hidd_Gallium, DisplaySurface)
{
    struct pHidd_Gallium_DisplayResource drmsg = {
    mID : OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_DisplayResource),
    resource    : (APTR)(((struct pipe_surface *)msg->surface)->texture),
    srcx        : msg->left,
    srcy        : msg->top,
    bitmap      : msg->rastport->BitMap,
    dstx        : msg->absx,
    dsty        : msg->absy,
    width       : msg->width,
    height      : msg->height
    };

    D(bug ("[Gallium] %s()\n", __PRETTY_FUNCTION__));

    OOP_DoMethod(o, (OOP_Msg)&drmsg);
}
#endif

VOID METHOD(HiddGallium, Hidd_Gallium, DisplayResource)
{
    D(bug ("[Gallium] %s()\n", __PRETTY_FUNCTION__));
}

