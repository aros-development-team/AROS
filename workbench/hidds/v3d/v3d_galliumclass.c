/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    VideoCore VI (V3D) - Gallium HIDD class.

    Unlike vc4gallium, which splits the Mesa driver from its winsys across
    a bridge, the Mesa v3d driver is linked into this module: its types are
    ours to dereference, so presenting a rendered resource needs no ABI
    boundary. What it does need is detiling - V3D renders into UIF-tiled
    memory, and a straight copy of that to a linear framebuffer produces
    the tile pattern instead of a picture. Mesa's own tiling code is in
    the archive, so the detiler is the one it uses itself.
*/

#define DEBUG 0
#include <aros/debug.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <cybergraphx/cybergraphics.h>
#include <hidd/gallium.h>

#include "v3d_intern.h"

#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "broadcom/common/v3d_limits.h"
#include "v3d_bufmgr.h"
#include "v3d_resource.h"
#include "v3d_tiling.h"

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase (SD(cl)->hiddGalliumAB)

/* What the driver actually renders, in WritePixelArray's terms. The pipe
 * format is a little-endian RGBA word, which reaches memory as B,G,R,A -
 * naming it ARGB instead swaps the channels, which is what turned the
 * triangle's colours inside out. */
#if (AROS_BIG_ENDIAN == 1)
#define V3D_PIXFMT  RECTFMT_RAW
#else
#define V3D_PIXFMT  RECTFMT_BGRA32
#endif

struct pipe_screen;
struct pipe_screen_config;
struct renderonly;
extern struct pipe_screen *v3d_screen_create(int fd,
    const struct pipe_screen_config *config, struct renderonly *ro);

struct GalliumCoreAPI;
extern int gca_bind(const struct GalliumCoreAPI *api);
extern const char *gca_slot_name(unsigned int slot);

/* Set for the drmIoctl override macro, which has no other way to reach
 * the driver's state. */
extern struct V3DData *g_v3d_data;

OOP_Object *HiddV3D__Root__New(OOP_Class *cl, OOP_Object *o,
                               struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
        SD(cl)->coreapi = (APTR)GetTagData(aHidd_Gallium_CoreAPI, 0,
                                           msg->attrList);
    return o;
}

APTR HiddV3D__Hidd_Gallium__CreatePipeScreen(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_CreatePipeScreen *msg)
{
    struct V3DData *sd = SD(cl);
    struct pipe_screen *screen;
    int bres;

    if (!sd->powered)
    {
        D(bug("[V3D] GPU not available\n"));
        return NULL;
    }

    if (!sd->coreapi)
    {
        bug("[V3D] GalliumCoreAPI table missing (old mesa3dgl?)\n");
        return NULL;
    }

    /* Bind the Mesa-core trampolines before any driver code runs. */
    bres = gca_bind((const struct GalliumCoreAPI *)sd->coreapi);
    if (bres > 0)
    {
        bug("[V3D] GalliumCoreAPI bind refused: slot %d is not '%s'\n",
            bres - 1, gca_slot_name((unsigned int)(bres - 1)));
        return NULL;
    }
    if (bres < 0)
    {
        bug("[V3D] GalliumCoreAPI bind refused (code %d)\n", bres);
        return NULL;
    }

    g_v3d_data = sd;

    /* fd is a dummy: the ioctl override never looks at it. No driconf and
     * no renderonly - we present through the display driver ourselves. */
    screen = v3d_screen_create(0, NULL, NULL);
    if (!screen)
    {
        bug("[V3D] v3d_screen_create failed\n");
        return NULL;
    }

    D(bug("[V3D] pipe_screen at %p\n", screen));
    return screen;
}

VOID HiddV3D__Hidd_Gallium__DestroyPipeScreen(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_DestroyPipeScreen *msg)
{
    struct V3DData *sd = SD(cl);

    if (msg->screen)
    {
        struct pipe_screen *screen = (struct pipe_screen *)msg->screen;

        /* Quiesce first: context teardown can flush a trailing job, and
         * the screen is about to free the memory it renders from. */
        v3d_wait_idle(sd);
        screen->destroy(screen);
    }
}

/*
 * Present one rectangle of a rendered resource into a bitmap.
 *
 * The GPU has to be idle before the pixels are read, and the read itself
 * goes through Mesa's detiler unless the slice is already raster - both
 * are correctness, not optimisation. WritePixelArray then places the
 * linear rows, which works for any bitmap; routing a linear resource
 * straight into the vcgfx framebuffer through DMA is the obvious next
 * step, and the reason this is a separate method from the flip path.
 */
VOID HiddV3D__Hidd_Gallium__DisplayResource(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_DisplayResource *msg)
{
    struct V3DData *sd = SD(cl);
    struct v3d_resource *rsc = (struct v3d_resource *)msg->resource;
    struct v3d_resource_slice *slice;
    struct RastPort *rp;
    UBYTE *base, *src;
    ULONG stride;
    APTR detiled = NULL;

    if (!rsc || !rsc->bo || !msg->bitmap
        || msg->width == 0 || msg->height == 0)
    {
        bug("[V3D] present: nothing to do (rsc=%p bo=%p bm=%p %ux%u)\n",
            rsc, rsc ? rsc->bo : NULL, msg->bitmap,
            (unsigned)msg->width, (unsigned)msg->height);
        return;
    }

    slice = &rsc->slices[0];
    v3d_wait_idle(sd);
    v3d_flush_caches(sd);

    /* bo->map is populated lazily - a target the GPU rendered into has
     * never been mapped, so reading the field directly finds NULL and
     * every frame goes silently missing. */
    base = v3d_bo_map(rsc->bo);
    if (!base)
    {
        bug("[V3D] present: cannot map bo %u\n",
            (unsigned)rsc->bo->handle);
        return;
    }

    if (slice->tiling == VC5_TILING_RASTER)
    {
        src = base + slice->offset
            + msg->srcy * slice->stride + msg->srcx * rsc->cpp;
        stride = slice->stride;
    }
    else
    {
        /* Tiled: pull the rectangle out through Mesa's own detiler, which
         * is the only thing that knows the UIF layout for this format. */
        struct pipe_box box;

        stride = msg->width * rsc->cpp;
        detiled = AllocVec(stride * msg->height, MEMF_ANY);
        if (!detiled)
            return;

        box.x = msg->srcx;
        box.y = msg->srcy;
        box.z = 0;
        box.width = msg->width;
        box.height = msg->height;
        box.depth = 1;

        v3d_load_tiled_image(detiled, stride, base + slice->offset,
                             slice->stride, slice->tiling, rsc->cpp,
                             slice->padded_height, &box);
        src = detiled;
    }

    /* Once per geometry change: enough to tell a present that never
     * happens from one landing in the wrong place. */
    {
        static ULONG w = 0, h = 0, n = 0;

        if (n < 20 && (msg->width != w || msg->height != h))
        {
            w = msg->width; h = msg->height; n++;
            bug("[V3D] present %ux%u cpp=%u tiling=%u stride=%u -> %u,%u\n",
                (unsigned)msg->width, (unsigned)msg->height,
                (unsigned)rsc->cpp, (unsigned)slice->tiling,
                (unsigned)stride, (unsigned)msg->dstx, (unsigned)msg->dsty);
        }
    }

    rp = CreateRastPort();
    if (rp)
    {
        rp->BitMap = msg->bitmap;
        WritePixelArray(src, 0, 0, stride, rp, msg->dstx, msg->dsty,
                        msg->width, msg->height, V3D_PIXFMT);
        FreeRastPort(rp);
    }

    if (detiled)
        FreeVec(detiled);
}
