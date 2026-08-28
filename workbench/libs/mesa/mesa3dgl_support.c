/*
    Copyright (C) 2009-2019, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

/* Before the rest: something further down defines SCHED_RR, which turns
 * the KRN_SchedType enum in aros/kernel.h into a syntax error. */
#ifdef MESA3DGL_HAVE_COREAPI
#include <aros/kernel.h>
#include <proto/kernel.h>
#include <hardware/bcm2708.h>
#endif

#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/dos.h>
#include <proto/icon.h>

#include <workbench/workbench.h>

#include <cybergraphx/cybergraphics.h>
#include <graphics/rpattr.h>

#include "mesa3dgl_support.h"

/* Render-scale divisor (ENV:VC4_RENDER_SCALE, 1-4): GL renders at 1/N
 * of the drawable size and the vc4 present path shows it through the
 * HVS hardware scaler — fragment cost drops by N^2. Only honoured when
 * the vc4 shim is linked in (weak marker below) AND the display side can
 * actually scale a plane; elsewhere the fallback blit can't scale and
 * would draw a small image in the corner. */
ULONG mesa3dgl_render_scale = 1;

/* Marker: the GalliumCoreAPI table exists only in builds that carry a
 * runtime-bound hw driver (raspi/vc4) — the render-scale feature is
 * driver-side scaling, so gate on it. */
#ifdef MESA3DGL_HAVE_COREAPI
extern const void *gallium_core_get_api(void);
#else
static inline const void *gallium_core_get_api(void) { return (const void *)0; }
#endif

/*
 * Second gate: only HVS4 builds a scaled plane, so on the BCM2711 a
 * scaled request would leave a 1/N image in the top-left corner. Not a
 * build-time decision - one aarch64 build ships both gallium hidds - and
 * the driver has no attribute to ask without pulling oop.library in here.
 */
#ifdef MESA3DGL_HAVE_COREAPI
static BOOL MESA3DGLScalerAvailable(VOID)
{
    struct Library *KernelBase = OpenResource("kernel.resource");

    if (!KernelBase)
        return FALSE;

    return KrnGetSystemAttr(KATTR_PeripheralBase) != BCM2711_PERIIOBASE;
}
#else
static inline BOOL MESA3DGLScalerAvailable(VOID) { return FALSE; }
#endif

/* Per-program override: VC4_RENDER_SCALE tooltype in the program's icon
 * (.info next to the executable). Runs in the caller's process context,
 * so GetProgramDir() and the process name identify the right program. */
static BOOL MESA3DGLReadScaleToolType(ULONG *scale)
{
    struct Library *IconBase;
    struct Process *me = (struct Process *)FindTask(NULL);
    struct DiskObject *dob;
    BPTR progdir, olddir;
    TEXT name[108];
    STRPTR val;
    BOOL found = FALSE;

    if (me->pr_Task.tc_Node.ln_Type != NT_PROCESS)
        return FALSE;

    progdir = GetProgramDir();
    if (progdir == BNULL)
        return FALSE;

    if (GetProgramName(name, sizeof(name)) == DOSFALSE || name[0] == '\0')
    {
        /* WB-launched: no CLI name set, the process carries the tool name */
        name[sizeof(name) - 1] = '\0';
        strncpy(name, me->pr_Task.tc_Node.ln_Name, sizeof(name) - 1);
    }

    IconBase = OpenLibrary("icon.library", 0);
    if (!IconBase)
        return FALSE;

    olddir = CurrentDir(progdir);
    dob = GetDiskObject(FilePart(name));
    if (dob)
    {
        val = FindToolType(dob->do_ToolTypes, "VC4_RENDER_SCALE");
        if (val && val[0] >= '1' && val[0] <= '4' && val[1] == '\0')
        {
            *scale = val[0] - '0';
            found = TRUE;
        }
        FreeDiskObject(dob);
    }
    CurrentDir(olddir);
    CloseLibrary(IconBase);

    return found;
}

static VOID MESA3DGLReadRenderScale(VOID)
{
    TEXT buf[8];

    mesa3dgl_render_scale = 1;
    if (!gallium_core_get_api() || !MESA3DGLScalerAvailable())
        return;

    if (GetVar("VC4_RENDER_SCALE", buf, sizeof(buf), 0) > 0
        && buf[0] >= '1' && buf[0] <= '4')
        mesa3dgl_render_scale = buf[0] - '0';

    /* Icon tooltype (per program) overrides the env variable (global) */
    MESA3DGLReadScaleToolType(&mesa3dgl_render_scale);
}

VOID MESA3DGLSelectRastPort(struct mesa3dgl_context * ctx, struct TagItem * tagList)
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    ctx->Screen = (struct Screen *)GetTagData(GLA_Screen, 0, tagList);
    ctx->window = (struct Window *)GetTagData(GLA_Window, 0, tagList);
    ctx->visible_rp = (struct RastPort *)GetTagData(GLA_RastPort, 0, tagList);

    if (ctx->Screen)
    {
        D(bug("[MESA3DGL] %s: Screen @ 0x%p\n", __func__, ctx->Screen));
        if (ctx->window)
        {
            D(bug("[MESA3DGL] %s: Window @ 0x%p\n", __func__, ctx->window));
            if (!(ctx->visible_rp))
            {
                /* Use the windows rastport */
                ctx->visible_rp = ctx->window->RPort;
                D(bug("[MESA3DGL] %s: Windows RastPort @ 0x%p\n", __func__, ctx->visible_rp));
            }
        }
        else
        {
            if (!(ctx->visible_rp))
            {
                /* Use the screens rastport */
                ctx->visible_rp = &ctx->Screen->RastPort;
                D(bug("[MESA3DGL] %s: Screens RastPort @ 0x%p\n", __func__, ctx->visible_rp));
            }
        }
    }
    else
    {
        /* Not passed a screen */
        if (ctx->window)
        {
            D(bug("[MESA3DGL] %s: Window @ 0x%p\n", __func__, ctx->window));
            /* Use the windows Screen */
            ctx->Screen = ctx->window->WScreen;
            D(bug("[MESA3DGL] %s: Windows Screen @ 0x%p\n", __func__, ctx->Screen));

            if (!(ctx->visible_rp))
            {
                /* Use the windows rastport */
                ctx->visible_rp = ctx->window->RPort;
                D(bug("[MESA3DGL] %s: Windows RastPort @ 0x%p\n", __func__, ctx->visible_rp));
            }
        }
        else
        {
            /* Only Passed A Rastport */
            D(bug("[MESA3DGL] %s: Using RastPort only!\n"));
        }
    }

    D(bug("[MESA3DGL] %s: Using RastPort @ 0x%p\n", __func__, ctx->visible_rp));

    /* Window/layer geometry at context creation: a fresh SDL window's
     * layer coming up collapsed (~1px) shows here. */
    D({
        struct Layer *l = ctx->visible_rp ? ctx->visible_rp->Layer : NULL;

        bug("[MESA3DGL] ctx: win=%p rp=%p layer=%p", ctx->window,
            ctx->visible_rp, l);
        if (ctx->window)
            bug(" win %ldx%ld at %ld,%ld", (LONG)ctx->window->Width,
                (LONG)ctx->window->Height, (LONG)ctx->window->LeftEdge,
                (LONG)ctx->window->TopEdge);
        if (l)
            bug(" layer bounds %ld,%ld..%ld,%ld", (LONG)l->bounds.MinX,
                (LONG)l->bounds.MinY, (LONG)l->bounds.MaxX,
                (LONG)l->bounds.MaxY);
        bug("\n");
    })
}

BOOL MESA3DGLStandardInit(struct mesa3dgl_context * ctx, struct TagItem *tagList)
{
    LONG requestedwidth = 0, requestedheight = 0;
    LONG requestedright = 0, requestedbottom = 0;
    LONG defaultleft = 0, defaulttop = 0;
    LONG defaultright = 0, defaultbottom = 0;

    D(bug("[MESA3DGL] %s(ctx @ 0x%p, taglist @ 0x%p)\n", __func__, ctx, tagList));

    MESA3DGLReadRenderScale();

    /* Set the defaults based on window information */
    if (ctx->window)
    {
        if(!(ctx->window->Flags & WFLG_GIMMEZEROZERO))
        {
            defaultleft     = ctx->window->BorderLeft;
            defaulttop      = ctx->window->BorderTop;
            defaultright    = ctx->window->BorderRight;
            defaultbottom   = ctx->window->BorderBottom;
        }
    }

    D(bug("[MESA3DGL] %s: Using RastPort @ 0x%p\n", __func__, ctx->visible_rp));

    ctx->visible_rp = CloneRastPort(ctx->visible_rp);

    D(bug("[MESA3DGL] %s: Cloned RastPort @ 0x%p\n", __func__, ctx->visible_rp));

    /* We assume left and top are given or if there is a window, set to border left/top
       or if there is no window set to 0 */
    ctx->left = GetTagData(GLA_Left, defaultleft, tagList);
    ctx->top = GetTagData(GLA_Top, defaulttop, tagList);

    requestedright = GetTagData(GLA_Right, -1, tagList);
    requestedbottom = GetTagData(GLA_Bottom, -1, tagList);
    requestedwidth = GetTagData(GLA_Width, -1 , tagList);
    requestedheight = GetTagData(GLA_Height, -1 , tagList);

    /* Calculate rastport dimensions */
    ctx->visible_rp_width =
        ctx->visible_rp->Layer->bounds.MaxX - ctx->visible_rp->Layer->bounds.MinX + 1;

    ctx->visible_rp_height =
        ctx->visible_rp->Layer->bounds.MaxY - ctx->visible_rp->Layer->bounds.MinY + 1;

    /* right will be either passed or calculated from width or 0 */
    ctx->right = 0;
    if (requestedright < 0)
    {
        if (requestedwidth >= 0)
        {
            requestedright = ctx->visible_rp_width - ctx->left - requestedwidth;
            if (requestedright < 0) requestedright = 0;
        }
        else
            requestedright = defaultright; /* Set the default here, not in GetDataData */
    }
    ctx->right = requestedright;

    /* bottom will be either passed or calculated from height or 0 */
    ctx->bottom = 0;
    if (requestedbottom < 0)
    {
        if (requestedheight >= 0)
        {
            requestedbottom = ctx->visible_rp_height - ctx->top - requestedheight;
            if (requestedbottom < 0) requestedbottom = 0;
        }
        else
            requestedbottom = defaultbottom; /* Set the default here, not in GetDataData */
    }
    ctx->bottom = requestedbottom;

    /* Init screen information */
    if (ctx->Screen)
        ctx->BitsPerPixel  = GetCyberMapAttr(ctx->Screen->RastPort.BitMap, CYBRMATTR_BPPIX) * 8;

    D(bug("[MESA3DGL] %s: Context Base dimensions set -:\n", __func__));
    D(bug("[MESA3DGL] %s:    ctx->visible_rp_width        = %d\n", __func__, ctx->visible_rp_width));
    D(bug("[MESA3DGL] %s:    ctx->visible_rp_height       = %d\n", __func__, ctx->visible_rp_height));
    D(bug("[MESA3DGL] %s:    ctx->left                    = %d\n", __func__, ctx->left));
    D(bug("[MESA3DGL] %s:    ctx->right                   = %d\n", __func__, ctx->right));
    D(bug("[MESA3DGL] %s:    ctx->top                     = %d\n", __func__, ctx->top));
    D(bug("[MESA3DGL] %s:    ctx->bottom                  = %d\n", __func__, ctx->bottom));

    return TRUE;
}

VOID MESA3DGLRecalculateBufferWidthHeight(struct mesa3dgl_context * ctx)
{
    ULONG newwidth = 0;
    ULONG newheight = 0;
    
    D(bug("[MESA3DGL] %s(0x%p)\n", __func__, ctx));

    ctx->visible_rp_width =
        ctx->visible_rp->Layer->bounds.MaxX - ctx->visible_rp->Layer->bounds.MinX + 1;

    ctx->visible_rp_height =
        ctx->visible_rp->Layer->bounds.MaxY - ctx->visible_rp->Layer->bounds.MinY + 1;

    /* NOTE: when a window's layer bounds read ~1px while the window
     * itself is fully sized, faking the framebuffer size here does NOT
     * help — AROS clips all rendering/blitting to the layer, so
     * presentation stays confined to those few pixels. Such a bug has to
     * be fixed where the window and its layer are created, not here.
     * Left as the plain layer read. */


    newwidth = ctx->visible_rp_width - ctx->left - ctx->right;
    newheight = ctx->visible_rp_height - ctx->top - ctx->bottom;

    if (newwidth < 0) newwidth = 0;
    if (newheight < 0) newheight = 0;

    /* Render-scale: draw at 1/N, present through the HVS scaler. Even
     * width keeps pixel formats happy; the present path upscales by
     * the exact src/dest ratio so rounding never skews the aspect. */
    if (mesa3dgl_render_scale > 1)
    {
        newwidth = (newwidth / mesa3dgl_render_scale) & ~1;
        newheight = newheight / mesa3dgl_render_scale;
        if (newwidth < 2) newwidth = 2;
        if (newheight < 2) newheight = 2;
    }
    
    
    if ((newwidth != ctx->framebuffer->width) || (newheight != ctx->framebuffer->height))
    {
        /* rp_w/h are pre-scale/pre-border. */
        D(bug("[MESA3DGL] fb resize: %lux%lu -> %lux%lu (rp %lux%lu, scale %lu)\n",
            (ULONG)ctx->framebuffer->width, (ULONG)ctx->framebuffer->height,
            (ULONG)newwidth, (ULONG)newheight,
            (ULONG)ctx->visible_rp_width, (ULONG)ctx->visible_rp_height,
            (ULONG)mesa3dgl_render_scale));

        /* The drawing area size has changed. Buffer must change */
        D(bug("[MESA3DGL] %s: current height    =   %d\n", __func__, ctx->framebuffer->height));
        D(bug("[MESA3DGL] %s: current width     =   %d\n", __func__, ctx->framebuffer->width));
        D(bug("[MESA3DGL] %s: new height        =   %d\n", __func__, newheight));
        D(bug("[MESA3DGL] %s: new width         =   %d\n", __func__, newwidth));
        
        ctx->framebuffer->width = newwidth;
        ctx->framebuffer->height = newheight;
        ctx->framebuffer->resized = TRUE;
        
        if (ctx->window)
        {
            struct Rectangle rastcliprect;
            struct TagItem crptags[] =
            {
                { RPTAG_ClipRectangle      , (IPTR)&rastcliprect },
                { RPTAG_ClipRectangleFlags , (RPCRF_RELRIGHT | RPCRF_RELBOTTOM) },
                { TAG_DONE }
            };
        
            D(bug("[MESA3DGL] %s: Clipping Rastport to Window's dimensions\n", __func__));

            /* Clip the rastport to the visible area */
            rastcliprect.MinX = ctx->left;
            rastcliprect.MinY = ctx->top;
            rastcliprect.MaxX = ctx->left + ctx->framebuffer->width;
            rastcliprect.MaxY = ctx->top + ctx->framebuffer->height;
            SetRPAttrsA(ctx->visible_rp, crptags);
        }
    }
    D(bug("[MESA3DGL] %s: done\n", __func__));
}

VOID MESA3DGLFreeContext(struct mesa3dgl_context * ctx)
{
    if (ctx)
    {
        FreeVec(ctx);
    }
}

