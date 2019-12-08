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
#ifndef GRAPHICS_VIEW_H
#   include <graphics/view.h>
#endif

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

        NEWLIST(&compdata->visbmstack);
        NEWLIST(&compdata->obscuredbmstack);
        csd->compositedbms = &compdata->visbmstack;
        csd->obscuredbms = &compdata->obscuredbmstack;
        NEWLIST(&csd->c2fragments);
        NEWLIST(&csd->c2ifragments);

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
    struct amigabm_data *bmdata, *bmdatprev;

    D(bug("[AmigaVideo:Compositor] %s()\n", __func__));

    LOCK_COMPOSITOR_WRITE

    /*
     * Detach existing list contents...
     */
    ForeachNodeSafe(&compdata->visbmstack, bmdata, bmdatprev)
    {
        bmdata->node.ln_Pri = -1;
        Remove(&bmdata->node);
        if ((!(bmdata->interlace)) && (bmdata->bmcl))
        {
            FreeVec(bmdata->bmcl->CopSStart);
            bmdata->bmcl->CopSStart = NULL;
        }
    }

    ForeachNodeSafe(&compdata->obscuredbmstack, bmdata, bmdatprev)
    {
        bmdata->node.ln_Pri = -1;
        Remove(&bmdata->node);
        if ((!(bmdata->interlace)) && (bmdata->bmcl))
        {
            FreeVec(bmdata->bmcl->CopSStart);
            bmdata->bmcl->CopSStart = NULL;
        }
    }

    {
        struct Node *frag, *fragtmp;
        ForeachNodeSafe(&csd->c2fragments, frag, fragtmp)
        {
            Remove(frag);
        }

        ForeachNodeSafe(&csd->c2ifragments, frag, fragtmp)
        {
            Remove(frag);
        }
    }

    csd->palmode = (GfxBase->DisplayFlags & NTSC) == 0;

    if (msg->data)
    {
        IPTR tags[] = {aHidd_BitMap_Visible, TRUE, TAG_DONE};
        IPTR screen_start, screen_finish = (STANDARD_DENISE_MAX << 1);
        struct copper2data *clfirst;
        csd->interlaced = FALSE;
        int scdepth = 0;

        for (vpdata = msg->data; vpdata; vpdata = vpdata->Next)
        {
            D(bug("[AmigaVideo:Compositor] %s: screen-bitmap @ 0x%p for viewport 0x%p\n", __func__, vpdata->Bitmap, vpdata->vpe->ViewPort);)

            if (vpdata->Bitmap && (OOP_OCLASS(vpdata->Bitmap) == csd->amigabmclass))
            {
                struct Node *next;

                bmdata = OOP_INST_DATA(OOP_OCLASS(vpdata->Bitmap), vpdata->Bitmap);
                bmdata->node.ln_Pri = scdepth++;

                D(
                  bug("[AmigaVideo:Compositor] %s: ** valid amigavideo bitmap @ depth %d\n", __func__, bmdata->node.ln_Pri);
                  bug("[AmigaVideo:Compositor] %s:    bmdata @ 0x%p\n", __func__, bmdata);
                  bug("[AmigaVideo:Compositor] %s:    x = %d\n", __func__, (LONG)bmdata->leftedge);
                  bug("[AmigaVideo:Compositor] %s:    y = %d\n", __func__, (LONG)bmdata->topedge);
                 )

                if ((LONG)bmdata->topedge < (LONG)screen_finish)
                    screen_finish = bmdata->topedge - (bmdata->copld.extralines + 1);
                else
                {
                    D(bug("[AmigaVideo:Compositor] %s: # screen is obscured - skipping...\n", __func__);)
                    AddTail(&compdata->obscuredbmstack, &bmdata->node);
                    continue;
                }

                if (bmdata->interlace)
                    csd->interlaced = TRUE;

                if ((csd->ecs_agnus) && ((bmdata->modeid & MONITOR_ID_MASK) == PAL_MONITOR_ID)) {
                    csd->palmode = TRUE;
                }

                /*
                 * Enqueue the bitmap based on its Y co-ord, so
                 * that the list can be iterated over chaining the copperlist's ..
                 */
                ForeachNode(&compdata->visbmstack, bmdatprev)
                {
                    if (bmdata->topedge < bmdatprev->topedge)
                        break;
                }
                bmdata->node.ln_Pred	            = bmdatprev->node.ln_Pred;
                bmdata->node.ln_Succ	            = &bmdatprev->node;
                bmdatprev->node.ln_Pred->ln_Succ    = &bmdata->node;
                bmdatprev->node.ln_Pred	            = &bmdata->node;
            }
        }

        setpalntsc(csd);
        screen_finish = 0;

        D(
            if (csd->interlaced)
                bug("[AmigaVideo:Compositor] %s: display contains an interlaced screen\n", __func__);
         )

        ForeachNode(&compdata->visbmstack, bmdata)
        {
            vpdata = (struct HIDD_ViewPortData *)bmdata->node.ln_Name;
            struct ViewPort  *bmvp = vpdata->vpe->ViewPort;

            D(bug("[AmigaVideo:Compositor] %s:  -- adjusting copperlist data for viewport @ 0x%p\n", __func__, bmvp);)

            AddTail(&csd->c2fragments, &bmdata->copld.cnode);
            D(bug("[AmigaVideo:Compositor] %s:  -- ViewPort->DspIns    = 0x%p\n", __func__, bmvp->DspIns);)
            D(bug("[AmigaVideo:Compositor] %s:  --         ->CopLStart = 0x%p\n", __func__, bmvp->DspIns->CopLStart);)
            if (bmdata->interlace)
            {
                D(bug("[AmigaVideo:Compositor] %s:  --         ->CopSStart = 0x%p\n", __func__, bmvp->DspIns->CopSStart);)
                AddTail(&csd->c2ifragments, &bmdata->copsd.cnode);
            }
            else if (csd->interlaced)
            {
                if (!(bmvp->DspIns->CopSStart))
                    bmvp->DspIns->CopSStart = AllocVec((bmvp->DspIns->MaxCount << 2), MEMF_CLEAR | MEMF_CHIP);

                D(bug("[AmigaVideo:Compositor] %s:  -- (dummy) ->CopSStart = 0x%p\n", __func__, bmvp->DspIns->CopSStart);)

                /* copy the copperlist data ... */
                CopyMemQuick(bmvp->DspIns->CopLStart, bmvp->DspIns->CopSStart, (bmvp->DspIns->MaxCount << 2));

                /* copy adjusted pointers ... */
                bmdata->copsd.copper2_palette = bmvp->DspIns->CopSStart + (bmdata->copld.copper2_palette - bmvp->DspIns->CopLStart);
                bmdata->copsd.copper2_scroll = bmvp->DspIns->CopSStart + (bmdata->copld.copper2_scroll - bmvp->DspIns->CopLStart);
                bmdata->copsd.copper2_bpl = bmvp->DspIns->CopSStart + (bmdata->copld.copper2_bpl - bmvp->DspIns->CopLStart);
                bmdata->copsd.copper2_tail = bmvp->DspIns->CopSStart + (bmdata->copld.copper2_tail - bmvp->DspIns->CopLStart);
                bmdata->copsd.extralines = bmdata->copld.extralines;
                if (bmdata->copld.copper2_palette_aga_lo)
                    bmdata->copsd.copper2_palette_aga_lo = bmvp->DspIns->CopSStart + (bmdata->copld.copper2_palette_aga_lo - bmvp->DspIns->CopLStart);
                if (bmdata->copld.copper2_fmode)
                    bmdata->copsd.copper2_fmode = bmvp->DspIns->CopSStart + (bmdata->copld.copper2_fmode - bmvp->DspIns->CopLStart);

                AddTail(&csd->c2ifragments, &bmdata->copsd.cnode);
            }

            /* calculate and adjust the screens visible region */
            screen_start = bmdata->topedge;
            if (bmdata->node.ln_Succ && bmdata->node.ln_Succ->ln_Succ)
            {
                screen_finish = ((struct amigabm_data *)(bmdata->node.ln_Succ))->topedge - (((struct amigabm_data *)(bmdata->node.ln_Succ))->copld.extralines + 1);
                if ((bmdata->interlace) && (!((struct amigabm_data *)(bmdata->node.ln_Succ))->interlace))
                    screen_finish <<= 1;
                else if ((!bmdata->interlace) && (((struct amigabm_data *)(bmdata->node.ln_Succ))->interlace))
                    screen_finish >>= 1;
            }
            else
            {
                screen_finish = bmdata->height - 1;
            }
            bmdata->displayheight = limitheight(csd, (screen_finish - screen_start) + 1, bmdata->interlace, FALSE);
            D(bug("[AmigaVideo:Compositor] %s:  -- screen range = %d -> %d (%d rows)\n", __func__, screen_start, screen_finish, bmdata->displayheight);)
            setcopperscroll(csd, bmdata, ((csd->interlaced == TRUE) || (bmdata->interlace == TRUE)));

#if !USE_UCOP_DIRECT
            if ((bmdata->bmucl) && !(bmdata->bmucl->Flags & (1<<15)))
            {
                D(bug("[AmigaVideo:Compositor] %s:  -- copying user-copperlist data ...\n", __func__);)
                CopyMemQuick(bmdata->bmucl->CopLStart, bmdata->copld.copper2_tail, bmdata->bmuclsize);
                bmdata->copld.copper2_tail = (APTR)((IPTR)bmdata->copld.copper2_tail + bmdata->bmuclsize);
                if ((bmdata->interlace) || (csd->interlaced))
                {
                    if (bmdata->bmucl->CopSStart)
                        CopyMemQuick(bmdata->bmucl->CopSStart, bmdata->copsd.copper2_tail, bmdata->bmuclsize);
                    else
                        CopyMemQuick(bmdata->bmucl->CopLStart, bmdata->copsd.copper2_tail, bmdata->bmuclsize);
                    bmdata->copsd.copper2_tail = (APTR)((IPTR)bmdata->copsd.copper2_tail + bmdata->bmuclsize);
                }
                bmdata->bmucl->Flags |= (1<<15);
            }
#endif

            /* link the copperlist fragments */
            if (bmdata->copld.cnode.mln_Pred && bmdata->copld.cnode.mln_Pred->mln_Pred)
            {
                setcopperlisttail(csd, ((struct copper2data *)bmdata->copld.cnode.mln_Pred)->copper2_tail, bmdata->bmcl->CopLStart, TRUE);                        // PREV CopL >> THIS CopL
                D(bug("[AmigaVideo:Compositor] %s:     CopL @ 0x%p jmp to CopL @ 0x%p\n", __func__, bmdata->copld.cnode.mln_Pred, &bmdata->copld);)
            }
            else
            {
                /* update the copper1 data to reflect the topmost screen */
                csd->copper1[5] = csd->bplcon3 | bmdata->bplcon3 | ((bmdata->sprite_res + 1) << 6);
                csd->copper1[9] = csd->bplcon3 | bmdata->bplcon3 | (1 << 9) | ((bmdata->sprite_res + 1) << 6);
                csd->copper1[13] = csd->bplcon3 | bmdata->bplcon3 | ((bmdata->sprite_res + 1) << 6);
                csd->copper1[27] = csd->bplcon2 | ((csd->aga && !(bmdata->modeid & EXTRAHALFBRITE_KEY)) ? 0x0200 : 0);
                if (csd->aga && csd->aga_enabled)
                {
                    csd->copper1[7] = bmdata->copld.copper2_palette[3];
                    csd->copper1[11] = bmdata->copld.copper2_palette[3];
                }
                else
                {
                    csd->copper1[7] = bmdata->copld.copper2_palette[1];
                    csd->copper1[11] = bmdata->copld.copper2_palette[1];
                }
            }

            if ((bmdata->interlace) || (csd->interlaced))
            {
                if (bmdata->copsd.cnode.mln_Pred && bmdata->copsd.cnode.mln_Pred->mln_Pred)
                {
                    setcopperlisttail(csd, ((struct copper2data *)bmdata->copsd.cnode.mln_Pred)->copper2_tail, bmdata->bmcl->CopSStart, TRUE);                   // PREV CopS >> THIS CopS
                    D(bug("[AmigaVideo:Compositor] %s:     CopS @ 0x%p jmp to CopS @ 0x%p\n", __func__, bmdata->copsd.cnode.mln_Pred, &bmdata->copsd);)
                    clfirst = (struct copper2data *)GetHead(&csd->c2ifragments);
                    bmdatprev = BMDATFROMCOPSD(clfirst);
                    setcopperlisttail(csd, bmdata->copld.copper2_tail, bmdatprev->bmcl->CopSStart, FALSE);                                                        // THIS CopL -> FIRST CopS
                    D(bug("[AmigaVideo:Compositor] %s:     CopL @ 0x%p point to CopS @ 0x%p\n", __func__, &bmdata->copld, clfirst);)
                }
                else
                {
                    setcopperlisttail(csd, bmdata->copld.copper2_tail, bmdata->bmcl->CopSStart, FALSE);                                                           // THIS CopL -> THIS CopS
                    D(bug("[AmigaVideo:Compositor] %s:     CopL @ 0x%p point to CopS @ 0x%p\n", __func__, &bmdata->copld, &bmdata->copsd);)
                }
                
                bmdatprev = BMDATFROMCOPLD(GetHead(&csd->c2fragments));
                setcopperlisttail(csd, bmdata->copsd.copper2_tail, bmdatprev->bmcl->CopLStart, FALSE);                                                           // THIS CopS -> FIRST CopL
                D(bug("[AmigaVideo:Compositor] %s:     CopS @ 0x%p point to CopL @ 0x%p\n", __func__, &bmdata->copsd, &bmdatprev->copld);)
            }
            else
            {
                if (!IsListEmpty(&csd->c2ifragments))
                {
                    clfirst = (struct copper2data *)GetHead(&csd->c2ifragments);
                    bmdatprev = BMDATFROMCOPSD(clfirst);
                    D(bug("[AmigaVideo:Compositor] %s:     CopL @ 0x%p point to CopS @ 0x%p\n", __func__, &bmdata->copld, clfirst);)
                    setcopperlisttail(csd, bmdata->copld.copper2_tail, bmdatprev->bmcl->CopSStart, FALSE);                                                      // THIS CopL -> FIRST CopS
                }
                else
                {
                    clfirst = (struct copper2data *)GetHead(&csd->c2fragments);
                    bmdatprev = BMDATFROMCOPLD(clfirst);
                    D(bug("[AmigaVideo:Compositor] %s:     CopL @ 0x%p point to CopL @ 0x%p\n", __func__, &bmdata->copld, clfirst);)
                    setcopperlisttail(csd, bmdata->copld.copper2_tail, bmdatprev->bmcl->CopLStart, FALSE);                                                      // THIS CopL -> FIRST CopL
                }
            }

            /* inform the bitmap it is visible */
            OOP_SetAttrs(vpdata->Bitmap, (struct TagItem *)tags);
        }
    }

    if (!IsListEmpty(&csd->c2fragments))
    {
        volatile struct Custom *custom = (struct Custom*)0xdff000;
        struct copper2data *clfirst = (struct copper2data *)GetHead(&csd->c2fragments);
        bmdatprev = BMDATFROMCOPLD(clfirst);

        /* make sure GfxBase points to the first copperlist in the chain ...*/
        GfxBase->LOFlist = bmdatprev->bmcl->CopLStart;

        /* set GfxBase Modes to its bcplcon0 value */
        GfxBase->Modes = clfirst->copper2_scroll[3];
        D(bug("[AmigaVideo:Compositor] %s: Gfxbase->Modes = %04x\n", __func__, GfxBase->Modes);)

        if (IsListEmpty(&csd->c2ifragments))
            GfxBase->SHFlist = bmdatprev->bmcl->CopLStart;
        else
        {
            bmdatprev = BMDATFROMCOPSD(GetHead(&csd->c2ifragments));
            GfxBase->SHFlist = bmdatprev->bmcl->CopSStart;
        }

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
    D(bug("[AmigaVideo:Compositor] %s()\n", __func__);)
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
    D(bug("[AmigaVideo:Compositor] %s()\n", __func__);)
}

