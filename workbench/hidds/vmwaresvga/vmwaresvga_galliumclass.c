/*
    Copyright 2015-2019, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <utility/tagitem.h>

#include <hidd/gallium.h>
#include <gallium/gallium.h>

#include "vmwaresvga_intern.h"

// ****************************************************************************
//                      Gallium Hidd Methods
// ****************************************************************************

OOP_Object *METHOD(GalliumVMWareSVGA, Root, New)
{
    IPTR interfaceVers;

    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__);)

    interfaceVers = GetTagData(aHidd_Gallium_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != GALLIUM_INTERFACE_VERSION)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        struct HIDDGalliumVMWareSVGAData * data = OOP_INST_DATA(cl, o);

        memset(data, 0, sizeof(struct HIDDGalliumVMWareSVGAData));

        data->wsgo                              = o;
        data->hwdata                            = &XSD(cl)->data;

        VMWareSVGA_WSScr_WinSysInit(data);
    }

    return o;
}

VOID METHOD(GalliumVMWareSVGA, Root, Dispose)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__);)

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(GalliumVMWareSVGA, Root, Get)
{
    ULONG idx;

    if (IS_GALLIUM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            /* Overload the property */
            case aoHidd_Gallium_InterfaceVersion:
                *msg->storage = GALLIUM_INTERFACE_VERSION;
                return;
        }
    }

    /* Use parent class for all other properties */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

APTR METHOD(GalliumVMWareSVGA, Hidd_Gallium, CreatePipeScreen)
{
    struct HIDDGalliumVMWareSVGAData * data = OOP_INST_DATA(cl, o);
    struct pipe_screen *screen = NULL;

    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__);)

    screen = svga_screen_create(&data->wssbase);

    D(bug("[VMWareSVGA:Gallium] %s: screen @ 0x%p\n", __func__, screen));

    return screen;
}

VOID METHOD(GalliumVMWareSVGA, Hidd_Gallium, DisplayResource)
{
    struct pipe_resource *res = (struct pipe_resource *)msg->resource;
    struct pipe_screen *resScreen;

    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__);)
    D(bug("[VMWareSVGA:Gallium] %s: resource @ 0x%p\n", __func__, msg->resource);)
    D(bug("[VMWareSVGA:Gallium] %s: bitmap @ 0x%p\n", __func__, msg->bitmap);)

    resScreen = res->screen;

    D(bug("[VMWareSVGA:Gallium] %s: resource screen @ 0x%p\n", __func__, resScreen);)
}
