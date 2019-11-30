/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

/* 
    AmigaVideo Compositor class.
    Manages the copperlist to display amiga gfx screen(s).
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <graphics/monitor.h>
#include <graphics/modeid.h>

#define CMDDEBUGUNIMP(x)
#define CMDDEBUGPIXEL(x)
#define DEBUG_TEXT(x)
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "amigavideo_hidd.h"
#include "amigavideo_compositor.h"

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

/* PUBLIC METHODS */
OOP_Object *METHOD(AmigaVideoCompositor, Root, New)
{
    struct amigavideo_staticdata *csd = CSD(cl);

    D(bug("[AmigaVideo:Compositor] %s()\n", __func__));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    if(o)
    {
        struct amigacomposit_data * compdata = OOP_INST_DATA(cl, o);
        struct Library *OOPBase = csd->cs_OOPBase;
        struct Library *UtilityBase = csd->cs_UtilityBase;
        BOOL dodispose = FALSE;

        InitSemaphore(&compdata->semaphore);

        NEWLIST(&compdata->bitmapstack);
        csd->compositedbms = &compdata->bitmapstack;

        compdata->gfx = (OOP_Object *)GetTagData(aHidd_Compositor_GfxHidd, 0, msg->attrList);
        if (compdata->gfx == NULL)
            dodispose = TRUE;
#if (0)
        else
        {
            /* Create GC object that will be used for drawing operations */
            compdata->gc = HIDD_Gfx_CreateObject(compdata->gfx, CSD(cl)->basegc, NULL);
            if (compdata->gc == NULL)
                dodispose = TRUE;
        }
#endif

        if (dodispose)
        {
            /* Creation failed */
            OOP_MethodID disposemid;
            disposemid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg)&disposemid);
            o = NULL;
        }
    }

    return o;
}

VOID METHOD(AmigaVideoCompositor, Hidd_Compositor, BitMapStackChanged)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigacomposit_data * compdata = OOP_INST_DATA(cl, o);
    struct GfxBase *GfxBase = (struct GfxBase *)csd->cs_GfxBase;
    struct Library *OOPBase = csd->cs_OOPBase;
    struct HIDD_ViewPortData * vpdata;
    OOP_Object *bm = NULL;
    struct amigabm_data *bmdata, *bmdatprev, *bmdatiprev;
    UWORD *copperdata = NULL, *copperdatai = NULL;

    D(bug("[AmigaVideo:Compositor] %s()\n", __func__));

    LOCK_COMPOSITOR_WRITE

    /* Purge existing list contents... */
    ForeachNodeSafe(&compdata->bitmapstack, bmdata, bmdatprev)
    {
        Remove(&bmdata->node);
    }

    csd->palmode = (GfxBase->DisplayFlags & NTSC) == 0;

    if (msg->data)
    {
        IPTR tags[] = {aHidd_BitMap_Visible, TRUE, TAG_DONE};
        IPTR screen_start, screen_finish = (STANDARD_DENISE_MAX << 1);
        struct copper2data *copperfirst;
        BOOL interlaced = FALSE;

        for (vpdata = msg->data; vpdata; vpdata = vpdata->Next)
        {
            IPTR val;
            D(bug("[AmigaVideo:Compositor] %s: Screen bitmap @ 0x%p for viewport 0x%p\n", __func__, vpdata->Bitmap, vpdata);)

            if (vpdata->Bitmap)
            {
                OOP_GetAttr(vpdata->Bitmap, aHidd_BitMap_LeftEdge, &val);
                D(bug("[AmigaVideo:Compositor] %s:    x = %d\n", __func__, (LONG)val);)
                OOP_GetAttr(vpdata->Bitmap, aHidd_BitMap_TopEdge, &val);
                D(bug("[AmigaVideo:Compositor] %s:    y = %d\n", __func__, (LONG)val);)
                if ((LONG)val < (LONG)screen_finish)
                    screen_finish = val;
                else
                {
                    D(bug("[AmigaVideo:Compositor] %s: # screen is obscured - skipping...\n", __func__);)
                    continue;
                }

                if (OOP_OCLASS(vpdata->Bitmap) == csd->amigabmclass)
                {
                    struct Node *next;

                    D(bug("[AmigaVideo:Compositor] %s:    ** valid amigavideo bitmap\n", __func__);)
                    bmdata = OOP_INST_DATA(OOP_OCLASS(vpdata->Bitmap), vpdata->Bitmap);
                    if ((csd->ecs_agnus) && ((bmdata->modeid & MONITOR_ID_MASK) == PAL_MONITOR_ID)) {
                        csd->palmode = TRUE;
                    }

                    /* Enqueue based on the Y position so we can iterate over the list pushing the copper info .. */
                    bmdata->node.ln_Name = (char *)vpdata;
                    ForeachNode(&compdata->bitmapstack, bmdatprev)
                    {
                        if ((LONG)val < bmdatprev->topedge)
                            break;
                    }
                    bmdata->node.ln_Pred	            = bmdatprev->node.ln_Pred;
                    bmdata->node.ln_Succ	            = &bmdatprev->node;
                    bmdatprev->node.ln_Pred->ln_Succ    = &bmdata->node;
                    bmdatprev->node.ln_Pred	            = &bmdata->node;
                }
            }
        }

        setpalntsc(csd);
        screen_finish = 0;
        bmdatprev = bmdatiprev = NULL;

        ForeachNode(&compdata->bitmapstack, bmdata)
        {
            vpdata = (struct HIDD_ViewPortData *)bmdata->node.ln_Name;
            if (!copperdata)
            {
                copperfirst = &bmdata->copper2;
                copperdata = bmdata->copper2.copper2;
            }
            if ((bmdata->interlace) && (!copperdatai))
                copperdatai = bmdata->copper2i.copper2;

            screen_start = bmdata->topedge;
            if (bmdata->node.ln_Succ && bmdata->node.ln_Succ->ln_Succ)
            {
                screen_finish = ((struct amigabm_data *)(bmdata->node.ln_Succ))->topedge - (((struct amigabm_data *)(bmdata->node.ln_Succ))->copper2.extralines + 1);
                if ((bmdata->interlace) && (!((struct amigabm_data *)(bmdata->node.ln_Succ))->interlace))
                    screen_finish <<= 1;
                else if ((!bmdata->interlace) && (((struct amigabm_data *)(bmdata->node.ln_Succ))->interlace))
                    screen_finish >>= 1;
            }
            else
            {
                screen_finish = bmdata->height;
            }
            D(
              bug("[AmigaVideo:Compositor] %s:  -- adjusting copperlist data for viewport @ 0x%p (%d -> %d)\n", __func__, vpdata, screen_start, screen_finish);
              bug("[AmigaVideo:Compositor] %s:  -- copperlist data @ 0x%p", __func__, bmdata->copper2.copper2);
              if (bmdata->interlace)
                  bug(" (copperlist-i data @ 0x%p)", bmdata->copper2i.copper2);
              bug("\n");
             )

            bmdata->displayheight = limitheight(csd, (screen_finish - screen_start), bmdata->interlace, FALSE);

            /* if we are the first displayed screen, set Modes to bcplcon0 */
            if (copperdata == bmdata->copper2.copper2)
            {
                copperdata = bmdata->copper2.copper2_bplcon0;
                GfxBase->Modes = copperdata[1];
                D(bug("[AmigaVideo:Compositor] %s: Gfxbase->Modes = %04x\n", __func__, GfxBase->Modes);)

                copperdata = bmdata->copper2.copper2;
            }

            if (bmdatprev)
            {
                D(bug("[AmigaVideo:Compositor] %s:  -- attaching to existing copperlist chain\n", __func__);)
                setcopperlisttail(csd, bmdatprev->copper2.copper2_tail, &bmdata->copper2, TRUE);
            }

           if (bmdata->interlace)
           {
                if (bmdatiprev)
                    setcopperlisttail(csd, bmdatiprev->copper2i.copper2_tail, &bmdata->copper2i, TRUE);

                setcopperlisttail(csd, bmdata->copper2i.copper2_tail, copperfirst, FALSE);
            }
            else
                setcopperlisttail(csd, bmdata->copper2.copper2_tail, copperfirst, FALSE);

            OOP_SetAttrs(vpdata->Bitmap, (struct TagItem *)tags);

            bmdatprev = bmdata;
            if (bmdata->interlace)
                bmdatiprev = bmdata;
        }
    }

    if (copperdata)
    {
        volatile struct Custom *custom = (struct Custom*)0xdff000;

        /* Make sure GfxBase points to the first copperlist in the chain ...*/
        GfxBase->LOFlist = copperdata;
        if (!copperdatai)
            GfxBase->SHFlist = copperdata;
        else
            GfxBase->SHFlist = copperdatai;

        custom->dmacon = 0x8100;

        if (csd->acb)
             csd->acb(csd->acbdata, bm);

    }
    else {
        D(bug("[AmigaVideo:Compositor] %s:  no visible screens .. blanking\n", __func__);)
        resetmode(csd);
    }

    UNLOCK_COMPOSITOR
}

VOID METHOD(AmigaVideoCompositor, Hidd_Compositor, BitMapRectChanged)
{
    struct amigacomposit_data * compdata = OOP_INST_DATA(cl, o);

    D(bug("[AmigaVideo:Compositor] %s()\n", __func__);)

    LOCK_COMPOSITOR_READ

    UNLOCK_COMPOSITOR
}

VOID METHOD(AmigaVideoCompositor, Hidd_Compositor, BitMapPositionChanged)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigacomposit_data * compdata = OOP_INST_DATA(cl, o);

    D(bug("[AmigaVideo:Compositor] %s()\n", __func__);)

    LOCK_COMPOSITOR_WRITE

    struct amigabm_data *bmdata = OOP_INST_DATA(OOP_OCLASS(msg->bm), msg->bm);
    setscroll(csd, bmdata);

    UNLOCK_COMPOSITOR
}

VOID METHOD(AmigaVideoCompositor, Hidd_Compositor, ValidateBitMapPositionChange)
{
    struct amigacomposit_data * compdata = OOP_INST_DATA(cl, o);

    D(bug("[AmigaVideo:Compositor] %s()\n", __func__);)

    LOCK_COMPOSITOR_READ

    UNLOCK_COMPOSITOR
}

