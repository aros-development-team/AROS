/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/gallium.h>
#include <proto/oop.h>
#include <aros/debug.h>

#include <gallium/pipe/p_screen.h>
#include "softpipe/sp_texture.h"
#include "softpipe/sp_public.h"
#include "state_tracker/sw_winsys.h"

#include "softpipe_intern.h"

#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <cybergraphx/cybergraphics.h>

#if (AROS_BIG_ENDIAN == 1)
#define AROS_PIXFMT RECTFMT_ARGB32   /* Big Endian Archs. */
#else
#define AROS_PIXFMT RECTFMT_BGRA32   /* Little Endian Archs. */
#endif

#define CyberGfxBase    (&BASE(cl->UserData)->sd)->SoftpipeCyberGfxBase

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (SD(cl)->hiddGalliumAB)

struct HiddSoftpipeWinSys
{
    struct HIDDT_WinSys base;
    struct sw_winsys    swws;
};

static struct HiddSoftpipeWinSys *
HiddSoftpipeCreateSoftpipeWinSys( void )
{
    struct HiddSoftpipeWinSys * ws = NULL;

    ws = (struct HiddSoftpipeWinSys *)AllocVec
        (sizeof(struct HiddSoftpipeWinSys), MEMF_PUBLIC|MEMF_CLEAR);

    /* Fill in this struct with callbacks that pipe will need to
    * communicate with the window system, buffer manager, etc. 
    */

    /* Since Mesa 7.9 softpipe mo longer uses pipe_winsys - it uses sw_winsys */
    ws->base.base.buffer_create         = NULL;
    ws->base.base.user_buffer_create    = NULL;
    ws->base.base.buffer_map            = NULL;
    ws->base.base.buffer_unmap          = NULL;
    ws->base.base.buffer_destroy        = NULL;
    ws->base.base.surface_buffer_create = NULL;
    ws->base.base.fence_reference       = NULL;
    ws->base.base.fence_signalled       = NULL;
    ws->base.base.fence_finish          = NULL;
    ws->base.base.flush_frontbuffer     = NULL;
    ws->base.base.get_name              = NULL;
    ws->base.base.destroy               = NULL;
    
    /* Fill in with functions is displaytarget is ever used*/
    ws->swws.destroy                            = NULL;
    ws->swws.is_displaytarget_format_supported  = NULL;
    ws->swws.displaytarget_create               = NULL;
    ws->swws.displaytarget_from_handle          = NULL;
    ws->swws.displaytarget_get_handle           = NULL;
    ws->swws.displaytarget_map                  = NULL;
    ws->swws.displaytarget_unmap                = NULL;
    ws->swws.displaytarget_display              = NULL;
    ws->swws.displaytarget_destroy              = NULL;
    
    return ws;
}

OOP_Object *METHOD(SoftpipeGallium, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    return o;
}

VOID METHOD(SoftpipeGallium, Root, Get)
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

APTR METHOD(SoftpipeGallium, Hidd_Gallium, CreatePipeScreen)
{
    struct HiddSoftpipeWinSys * softpipews;
    struct pipe_screen *screen;

    softpipews = HiddSoftpipeCreateSoftpipeWinSys();
    if (softpipews == NULL)
        return NULL;

    screen = softpipe_create_screen(&softpipews->swws);
    if (screen == NULL)
        goto fail;

    /* Force a pipe_winsys pointer (Mesa 7.9 or never) */
    screen->winsys = (struct pipe_winsys *)softpipews;

    /* Preserve pointer to HIDD driver */
    softpipews->base.driver = o;

    return screen;

fail:
    if (softpipews && softpipews->base.base.destroy)
        softpipews->base.base.destroy((struct pipe_winsys *)softpipews);

    return NULL;
}

VOID METHOD(SoftpipeGallium, Hidd_Gallium, DisplaySurface)
{
    struct pipe_surface * surf = (struct pipe_surface *)msg->surface;
    struct softpipe_resource *spr = softpipe_resource(surf->texture);
    struct RastPort * rp = CloneRastPort(msg->rastport);

    WritePixelArray(
        spr->data, 
        msg->left,
        msg->top,
        spr->stride[surf->level],
        rp, 
        msg->relx, 
        msg->rely, 
        msg->width, 
        msg->height, 
        AROS_PIXFMT);

    FreeRastPort(rp);
}

