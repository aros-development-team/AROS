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
#include "softpipe/sp_screen.h"
#include "util/u_format.h"
#include "util/u_math.h"
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

/*  Displaytarget support code */
struct HiddSoftpipeDisplaytarget
{
    APTR data;
};

struct HiddSoftpipeDisplaytarget * HiddSoftpipeDisplaytarget(struct sw_displaytarget * dt)
{
    return (struct HiddSoftpipeDisplaytarget *)dt;
}

static struct sw_displaytarget *
HiddSoftpipeCreateDisplaytarget(struct sw_winsys *winsys, unsigned tex_usage,
    enum pipe_format format, unsigned width, unsigned height,
    unsigned alignment, unsigned *stride)
{
    struct HiddSoftpipeDisplaytarget * spdt = 
        AllocVec(sizeof(struct HiddSoftpipeDisplaytarget), MEMF_PUBLIC | MEMF_CLEAR);
    
    *stride = align(util_format_get_stride(format, width), alignment);
    spdt->data = AllocVec(*stride * height, MEMF_PUBLIC | MEMF_CLEAR);
    
    return (struct sw_displaytarget *)spdt;
}

static void
HiddSoftpipeDestroyDisplaytarget(struct sw_winsys *ws, struct sw_displaytarget *dt)
{
    struct HiddSoftpipeDisplaytarget * spdt = HiddSoftpipeDisplaytarget(dt);
    
    if (spdt)
    {
        FreeVec(spdt->data);
        FreeVec(spdt);
    }
}

static void *
HiddSoftpipeMapDisplaytarget(struct sw_winsys *ws, struct sw_displaytarget *dt,
    unsigned flags)
{
    struct HiddSoftpipeDisplaytarget * spdt = HiddSoftpipeDisplaytarget(dt);
    return spdt->data;
}

static void
HiddSoftpipeUnMapDisplaytarget(struct sw_winsys *ws, struct sw_displaytarget *dt)
{
    /* No op */
}

/*  Displaytarget support code ends */

static struct HiddSoftpipeWinSys *
HiddSoftpipeCreateSoftpipeWinSys(void)
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
    ws->swws.displaytarget_create               = HiddSoftpipeCreateDisplaytarget;
    ws->swws.displaytarget_from_handle          = NULL;
    ws->swws.displaytarget_get_handle           = NULL;
    ws->swws.displaytarget_map                  = HiddSoftpipeMapDisplaytarget;
    ws->swws.displaytarget_unmap                = HiddSoftpipeUnMapDisplaytarget;
    ws->swws.displaytarget_display              = NULL;
    ws->swws.displaytarget_destroy              = HiddSoftpipeDestroyDisplaytarget;
    
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

VOID METHOD(SoftpipeGallium, Hidd_Gallium, DisplayResource)
{
    struct softpipe_resource * spr = softpipe_resource(msg->resource);
    struct sw_winsys * swws = softpipe_screen(spr->base.screen)->winsys;
    struct RastPort * rp = CreateRastPort();
    APTR * data = spr->data;

    if ((data == NULL) && (spr->dt != NULL))
        data = swws->displaytarget_map(swws, spr->dt, 0);

    if (data)
    {
        rp->BitMap = msg->bitmap;

        WritePixelArray(
            data, 
            msg->srcx,
            msg->srcy,
            spr->stride[0],
            rp, 
            msg->dstx, 
            msg->dsty, 
            msg->width, 
            msg->height, 
            AROS_PIXFMT);
    }

    if ((spr->data == NULL) && (data != NULL))
        swws->displaytarget_unmap(swws, spr->dt);

    FreeRastPort(rp);
}

