/*
    Copyright 2010-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include "pipe/p_screen.h"
#include "softpipe/sp_texture.h"
#include "softpipe/sp_public.h"
#include "softpipe/sp_screen.h"
#include "util/u_format.h"
#include "util/u_math.h"

#include <proto/oop.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <cybergraphx/cybergraphics.h>

#include <hidd/gallium.h>
#include <gallium/gallium.h>

#include "softpipe_intern.h"

#if (AROS_BIG_ENDIAN == 1)
#define AROS_PIXFMT RECTFMT_RAW   /* Big Endian Archs. */
#else
#define AROS_PIXFMT RECTFMT_BGRA32   /* Little Endian Archs. */
#endif

#define CyberGfxBase    (&BASE(cl->UserData)->sd)->CyberGfxBase
#define UtilityBase    (&BASE(cl->UserData)->sd)->UtilityBase

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (SD(cl)->hiddGalliumAB)

/*  Displaytarget support code */
struct HiddSoftpipeDisplaytarget
{
    enum pipe_format fmt;
    APTR data;
};

struct HiddSoftpipeDisplaytarget * HiddSoftpipe_Displaytarget(struct sw_displaytarget * dt)
{
    return (struct HiddSoftpipeDisplaytarget *)dt;
}

static boolean
HiddSoftpipe_IsFormatSupported( struct sw_winsys *ws,
                                          unsigned tex_usage,
                                          enum pipe_format format )
{
   return TRUE;
}


static struct sw_displaytarget *
HiddSoftpipe_CreateDisplaytarget( struct sw_winsys *ws,
                            unsigned tex_usage,
                            enum pipe_format format,
                            unsigned width, unsigned height,
                            unsigned alignment,
                            const void *front_private,
                            unsigned *stride )
{
    struct HiddSoftpipeDisplaytarget * spdt = 
        AllocVec(sizeof(struct HiddSoftpipeDisplaytarget), MEMF_PUBLIC | MEMF_CLEAR);
    
    *stride = align(util_format_get_stride(format, width), alignment);
    spdt->data = AllocVec(*stride * height, MEMF_PUBLIC | MEMF_CLEAR);
    spdt->fmt = format;

    bug("[SoftPipe] %s: fmt #%d\n", __PRETTY_FUNCTION__, spdt->fmt);

    return (struct sw_displaytarget *)spdt;
}

static void
HiddSoftpipe_DestroyDisplaytarget(struct sw_winsys *ws, struct sw_displaytarget *dt)
{
    struct HiddSoftpipeDisplaytarget * spdt = HiddSoftpipe_Displaytarget(dt);
    
    if (spdt)
    {
        FreeVec(spdt->data);
        FreeVec(spdt);
    }
}

static void *
HiddSoftpipe_MapDisplaytarget(struct sw_winsys *ws, struct sw_displaytarget *dt,
    unsigned flags)
{
    struct HiddSoftpipeDisplaytarget * spdt = HiddSoftpipe_Displaytarget(dt);
    return spdt->data;
}

static void
HiddSoftpipe_UnMapDisplaytarget(struct sw_winsys *ws, struct sw_displaytarget *dt)
{
    /* No op */
}

/*  Displaytarget support code ends */

OOP_Object *METHOD(HiddSoftpipe, Root, New)
{
    IPTR interfaceVers;

    D(bug("[SoftPipe] %s()\n", __PRETTY_FUNCTION__));

    interfaceVers = GetTagData(aHidd_Gallium_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != GALLIUM_INTERFACE_VERSION)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HiddGalliumSoftpipeData * HiddSoftpipe_DATA = OOP_INST_DATA(cl, o);

        HiddSoftpipe_DATA->softpipe_obj = o;

        HiddSoftpipe_DATA->softpipe_winsys.destroy                            = NULL;
        HiddSoftpipe_DATA->softpipe_winsys.is_displaytarget_format_supported  = HiddSoftpipe_IsFormatSupported;
        HiddSoftpipe_DATA->softpipe_winsys.displaytarget_create               = HiddSoftpipe_CreateDisplaytarget;
        HiddSoftpipe_DATA->softpipe_winsys.displaytarget_from_handle          = NULL;
        HiddSoftpipe_DATA->softpipe_winsys.displaytarget_get_handle           = NULL;
        HiddSoftpipe_DATA->softpipe_winsys.displaytarget_map                  = HiddSoftpipe_MapDisplaytarget;
        HiddSoftpipe_DATA->softpipe_winsys.displaytarget_unmap                = HiddSoftpipe_UnMapDisplaytarget;
        HiddSoftpipe_DATA->softpipe_winsys.displaytarget_display              = NULL;
        HiddSoftpipe_DATA->softpipe_winsys.displaytarget_destroy              = HiddSoftpipe_DestroyDisplaytarget;
    }

    return o;
}

VOID METHOD(HiddSoftpipe, Root, Dispose)
{
    D(bug("[SoftPipe] %s()\n", __PRETTY_FUNCTION__));

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(HiddSoftpipe, Root, Get)
{
#if (0)
    struct HiddGalliumSoftpipeData * HiddSoftpipe_DATA = OOP_INST_DATA(cl, o);
#endif
    ULONG idx;

    D(bug ("[SoftPipe] %s()\n", __PRETTY_FUNCTION__));

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

APTR METHOD(HiddSoftpipe, Hidd_Gallium, CreatePipeScreen)
{
    struct HiddGalliumSoftpipeData * HiddSoftpipe_DATA = OOP_INST_DATA(cl, o);
    struct pipe_screen *screen = NULL;

    D(bug ("[SoftPipe] %s()\n", __PRETTY_FUNCTION__));

    screen = softpipe_create_screen(&HiddSoftpipe_DATA->softpipe_winsys);

    D(bug ("[SoftPipe] %s: screen @ 0x%p\n", __PRETTY_FUNCTION__, screen));

    return screen;
}

VOID METHOD(HiddSoftpipe, Hidd_Gallium, DisplayResource)
{
    struct HiddGalliumSoftpipeData * HiddSoftpipe_DATA = OOP_INST_DATA(cl, o);
    struct softpipe_resource * spr = softpipe_resource(msg->resource);
    struct RastPort * rp;
    APTR * data = spr->data;

    D(bug ("[SoftPipe] %s()\n", __PRETTY_FUNCTION__));

    if ((data == NULL) && (spr->dt != NULL))
        data = HiddSoftpipe_DATA->softpipe_winsys.displaytarget_map(&HiddSoftpipe_DATA->softpipe_winsys, spr->dt, 0);

    if (data)
    {
        rp = CreateRastPort();

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

        FreeRastPort(rp);
    }

    if ((spr->data == NULL) && (data != NULL))
        HiddSoftpipe_DATA->softpipe_winsys.displaytarget_unmap(&HiddSoftpipe_DATA->softpipe_winsys, spr->dt);
}
