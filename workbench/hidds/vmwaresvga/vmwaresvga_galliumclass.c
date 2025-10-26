/*
    Copyright 2015-2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <utility/tagitem.h>

#include <cybergraphx/cybergraphics.h>

#include <hidd/gallium.h>
#include <gallium/gallium.h>

#include "vmwaresvga_intern.h"

#include "pipe/p_context.h"      // For struct pipe_context
#include "pipe/p_state.h"        // For struct pipe_transfer
#include "util/u_box.h"          // For u_box_2d()

#if (AROS_BIG_ENDIAN == 1)
#define AROS_PIXFMT RECTFMT_RAW   /* Big Endian Archs. */
#else
#define AROS_PIXFMT RECTFMT_BGRA32   /* Little Endian Archs. */
#endif

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
    struct pipe_context *pipe;

    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__);)

    screen = svga_screen_create(&data->wssbase);
    D(bug("[VMWareSVGA:Gallium] %s: screen @ 0x%p\n", __func__, screen));
    if (screen)
    {
        data->spipe = screen->context_create(screen, data, 0);
        bug("[VMWareSVGA:Gallium] %s: pipe @ 0x%p\n", __func__, data->spipe);
    }

    return screen;
}

VOID METHOD(GalliumVMWareSVGA, Hidd_Gallium, DisplayResource)
{
    struct HIDDGalliumVMWareSVGAData * data = OOP_INST_DATA(cl, o);
    struct pipe_resource *res = (struct pipe_resource *)msg->resource;
    struct pipe_context *pipe = data->spipe;
    struct pipe_transfer *transfer = NULL;
    uint8_t *mapped;
    struct RastPort *rp;

    D(
        bug("[VMWareSVGA:Gallium] %s: pipe @ %p\n", __func__, pipe);
        bug("[VMWareSVGA:Gallium] %s: resource @ %p\n", __func__, res);
        bug("[VMWareSVGA:Gallium] %s: bitmap @ %p (%u,%u -> %u,%u)\n", __func__,
            msg->bitmap, msg->srcx, msg->srcy, msg->srcx + msg->width, msg->srcy + msg->height);
    )

    if (!pipe || !res || !msg->bitmap)
        return;

    struct pipe_box box;
    u_box_2d(msg->srcx, msg->srcy, msg->width, msg->height, &box);

    // Map the resource for CPU read
    mapped = pipe->transfer_map(pipe, res, 0, PIPE_TRANSFER_READ, &box, &transfer);
    if (!mapped) {
        bug("[VMWareSVGA:Gallium] transfer_map failed\n");
        return;
    }

    D(bug("[VMWareSVGA:Gallium] %s: resource mapped @ 0x%p\n", __func__, mapped));

    rp = CreateRastPort();
    if (rp) {
        rp->BitMap = msg->bitmap;

        WritePixelArray(
            mapped,
            0, 0,
            transfer->stride,               // Row stride
            rp,
            msg->dstx, msg->dsty,
            msg->width, msg->height,
            AROS_PIXFMT);

        FreeRastPort(rp);
        D(bug("[VMWareSVGA:Gallium] %s: resource output\n", __func__);)
    }

    // Unmap
    pipe->transfer_unmap(pipe, transfer);
    D(bug("[VMWareSVGA:Gallium] %s: done\n", __func__);)
}
