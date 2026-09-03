/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef DEBUG
#define DEBUG 1
#endif
#include <aros/debug.h>

#include "pipe/p_screen.h"
#include "llvmpipe/lp_texture.h"
#include "llvmpipe/lp_public.h"
#include "util/format/u_format.h"
#include "util/u_math.h"

#include <proto/oop.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <cybergraphx/cybergraphics.h>

#include <hidd/gallium.h>
#include <gallium/gallium.h>

#include "llvmpipe_intern.h"

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
struct HiddLlvmpipeDisplaytarget
{
    enum pipe_format fmt;
    APTR data;
};

struct HiddLlvmpipeDisplaytarget * HiddLlvmpipe_Displaytarget(struct sw_displaytarget * dt)
{
    return (struct HiddLlvmpipeDisplaytarget *)dt;
}

static bool
HiddLlvmpipe_IsFormatSupported( struct sw_winsys *ws,
                                          unsigned tex_usage,
                                          enum pipe_format format )
{

    bug("[LLVMPipe] %s: fmt #%d\n", __PRETTY_FUNCTION__, format);

    return true;
}


static struct sw_displaytarget *
HiddLlvmpipe_CreateDisplaytarget( struct sw_winsys *ws,
                            unsigned tex_usage,
                            enum pipe_format format,
                            unsigned width, unsigned height,
                            unsigned alignment,
                            const void *front_private,
                            unsigned *stride )
{
    struct HiddLlvmpipeDisplaytarget * spdt =
        AllocVec(sizeof(struct HiddLlvmpipeDisplaytarget), MEMF_PUBLIC | MEMF_CLEAR);
    if (!spdt)
    {
        bug("[LLVMPipe] %s: ERROR - displaytarget header alloc failed fmt=%d size=%ux%u align=%u\n",
            __PRETTY_FUNCTION__, format, width, height, alignment);
        return NULL;
    }

    *stride = align(util_format_get_stride(format, width), alignment);
    spdt->data = AllocVec(*stride * height, MEMF_PUBLIC | MEMF_CLEAR);
    if (!spdt->data)
    {
        bug("[LLVMPipe] %s: ERROR - displaytarget data alloc failed fmt=%d size=%ux%u stride=%u\n",
            __PRETTY_FUNCTION__, format, width, height, *stride);
        FreeVec(spdt);
        return NULL;
    }
    spdt->fmt = format;

    bug("[LLVMPipe] %s: step 1 create dt fmt=%d size=%ux%u stride=%u align=%u front=%p dt=%p data=%p\n",
        __PRETTY_FUNCTION__, format, width, height, *stride, alignment, front_private, spdt,
        spdt ? spdt->data : NULL);

    return (struct sw_displaytarget *)spdt;
}

static void
HiddLlvmpipe_DestroyDisplaytarget(struct sw_winsys *ws, struct sw_displaytarget *dt)
{
    struct HiddLlvmpipeDisplaytarget * spdt = HiddLlvmpipe_Displaytarget(dt);
    
    if (spdt)
    {
        FreeVec(spdt->data);
        FreeVec(spdt);
    }
}

static void *
HiddLlvmpipe_MapDisplaytarget(struct sw_winsys *ws, struct sw_displaytarget *dt,
    unsigned flags)
{
    struct HiddLlvmpipeDisplaytarget * spdt = HiddLlvmpipe_Displaytarget(dt);
    bug("[LLVMPipe] %s: step 2 map dt=%p flags=0x%x data=%p\n",
        __PRETTY_FUNCTION__, dt, flags, spdt ? spdt->data : NULL);
    if (!spdt)
    {
        bug("[LLVMPipe] %s: ERROR - map called with NULL displaytarget\n", __PRETTY_FUNCTION__);
        return NULL;
    }
    return spdt->data;
}

static void
HiddLlvmpipe_UnMapDisplaytarget(struct sw_winsys *ws, struct sw_displaytarget *dt)
{
    bug("[LLVMPipe] %s: step 3 unmap dt=%p\n", __PRETTY_FUNCTION__, dt);
}

/*  Displaytarget support code ends */

OOP_Object *METHOD(HiddLlvmpipe, Root, New)
{
    IPTR interfaceVers;

    D(bug("[LLVMPipe] %s()\n", __PRETTY_FUNCTION__));

    interfaceVers = GetTagData(aHidd_Gallium_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != GALLIUM_INTERFACE_VERSION)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HiddGalliumLlvmpipeData * HiddLlvmpipe_DATA = OOP_INST_DATA(cl, o);

        HiddLlvmpipe_DATA->llvmpipe_obj = o;

        HiddLlvmpipe_DATA->llvmpipe_winsys.destroy                            = NULL;
#if defined(AROS_LLVMPIPE_HAS_FRONTEND_SW_WINSYS)
        HiddLlvmpipe_DATA->llvmpipe_winsys.get_fd                             = NULL;
#endif
        HiddLlvmpipe_DATA->llvmpipe_winsys.is_displaytarget_format_supported  = HiddLlvmpipe_IsFormatSupported;
        HiddLlvmpipe_DATA->llvmpipe_winsys.displaytarget_create               = HiddLlvmpipe_CreateDisplaytarget;
        HiddLlvmpipe_DATA->llvmpipe_winsys.displaytarget_from_handle          = NULL;
        HiddLlvmpipe_DATA->llvmpipe_winsys.displaytarget_get_handle           = NULL;
        HiddLlvmpipe_DATA->llvmpipe_winsys.displaytarget_map                  = HiddLlvmpipe_MapDisplaytarget;
        HiddLlvmpipe_DATA->llvmpipe_winsys.displaytarget_unmap                = HiddLlvmpipe_UnMapDisplaytarget;
        HiddLlvmpipe_DATA->llvmpipe_winsys.displaytarget_display              = NULL;
        HiddLlvmpipe_DATA->llvmpipe_winsys.displaytarget_destroy              = HiddLlvmpipe_DestroyDisplaytarget;
#if defined(AROS_LLVMPIPE_HAS_FRONTEND_SW_WINSYS)
        HiddLlvmpipe_DATA->llvmpipe_winsys.displaytarget_create_mapped        = NULL;
#endif
    }
    else
    {
        D(bug("[LLVMPipe] %s: OOP_DoSuperMethod returned NULL\n", __PRETTY_FUNCTION__));
    }

    return o;
}

VOID METHOD(HiddLlvmpipe, Root, Dispose)
{
    D(bug("[LLVMPipe] %s()\n", __PRETTY_FUNCTION__));

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(HiddLlvmpipe, Root, Get)
{
#if (0)
    struct HiddGalliumLlvmpipeData * HiddLlvmpipe_DATA = OOP_INST_DATA(cl, o);
#endif
    ULONG idx;

    D(bug ("[LLVMPipe] %s()\n", __PRETTY_FUNCTION__));

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

APTR METHOD(HiddLlvmpipe, Hidd_Gallium, CreatePipeScreen)
{
    struct HiddGalliumLlvmpipeData * HiddLlvmpipe_DATA = OOP_INST_DATA(cl, o);
    struct pipe_screen *screen = NULL;

    D(bug ("[LLVMPipe] %s()\n", __PRETTY_FUNCTION__));
    D(bug("[LLVMPipe] %s: step 6 create pipe screen begin winsys=%p\n",
          __PRETTY_FUNCTION__, &HiddLlvmpipe_DATA->llvmpipe_winsys));

    screen = llvmpipe_create_screen(&HiddLlvmpipe_DATA->llvmpipe_winsys);

    D(bug ("[LLVMPipe] %s: screen @ 0x%p\n", __PRETTY_FUNCTION__, screen));

    return screen;
}

VOID METHOD(HiddLlvmpipe, Hidd_Gallium, DisplayResource)
{
    struct HiddGalliumLlvmpipeData * HiddLlvmpipe_DATA = OOP_INST_DATA(cl, o);
    struct llvmpipe_resource *lpr = llvmpipe_resource(msg->resource);
    struct RastPort * rp;
    APTR data = lpr->tex_data ? lpr->tex_data : lpr->data;
    BOOL mapped = FALSE;

    D(bug ("[LLVMPipe] %s()\n", __PRETTY_FUNCTION__));
    D(bug("[LLVMPipe] %s: step 7 resource=%p target=%d stride=%u tex_data=%p data=%p dt=%p\n",
          __PRETTY_FUNCTION__, lpr, lpr->base.target, lpr->row_stride[0],
          lpr->tex_data, lpr->data, lpr->dt));

    if ((data == NULL) && (lpr->dt != NULL))
    {
        D(bug("[LLVMPipe] %s: step 8 map displaytarget\n", __PRETTY_FUNCTION__));
        data = HiddLlvmpipe_DATA->llvmpipe_winsys.displaytarget_map(&HiddLlvmpipe_DATA->llvmpipe_winsys, lpr->dt, 0);
        mapped = TRUE;
    }

    if (data)
    {
        D(bug("[LLVMPipe] %s: step 9 write data=%p bitmap=%p rect=%dx%d src=%d,%d dst=%d,%d\n",
              __PRETTY_FUNCTION__, data, msg->bitmap, msg->width, msg->height,
              msg->srcx, msg->srcy, msg->dstx, msg->dsty));
        rp = CreateRastPort();

        rp->BitMap = msg->bitmap;

        WritePixelArray(
            data,
            msg->srcx,
            msg->srcy,
            lpr->row_stride[0],
            rp,
            msg->dstx,
            msg->dsty,
            msg->width,
            msg->height,
            AROS_PIXFMT);

        FreeRastPort(rp);
    }

    if (mapped)
    {
        D(bug("[LLVMPipe] %s: step 10 unmap displaytarget\n", __PRETTY_FUNCTION__));
        HiddLlvmpipe_DATA->llvmpipe_winsys.displaytarget_unmap(&HiddLlvmpipe_DATA->llvmpipe_winsys, lpr->dt);
    }
}
