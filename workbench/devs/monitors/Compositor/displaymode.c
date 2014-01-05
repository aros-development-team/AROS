/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/utility.h>
#include "compositor_intern.h"

static HIDDT_ModeID FindBestHiddMode(struct HIDDCompositorData *compdata, ULONG width, ULONG height, ULONG depth, ULONG *res_depth)
{
    HIDDT_ModeID mode = vHidd_ModeID_Invalid;
    OOP_Object *sync, *pf;
    IPTR w, h, d;
    ULONG dw, dh, delta;
    ULONG found_delta  = -1;
    ULONG found_width  = 0;
    ULONG found_height = 0;
    ULONG found_depth  = 0;
    HIDDT_ModeID found_mode = vHidd_ModeID_Invalid;

    D(bug("[%s] Finding best match for mode %ux%ux%u\n", __PRETTY_FUNCTION__, width, height, depth));

    while ((mode = HIDD_Gfx_NextModeID(compdata->gfx, mode, &sync, &pf)) != vHidd_ModeID_Invalid)
    {
        BOOL match;

        D(bug("[%s] Checking mode 0x%08X... ", __PRETTY_FUNCTION__, mode));
        if (OOP_GET(pf, aHidd_PixFmt_ColorModel) != vHidd_ColorModel_TrueColor)
        {
            D(bug("Skipped (not truecolor)\n"));
            continue;
        }

        OOP_GetAttr(sync, aHidd_Sync_HDisp, &w);
        OOP_GetAttr(sync, aHidd_Sync_VDisp, &h);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth, &d);

        dw = w > width  ? w - width  : w < width  ? width  - w : 1;
        dh = h > height ? h - height : h < height ? height - h : 1;
        delta = dw * dh;

        match = FALSE;
        if (delta < found_delta)
        {
            /* If mode resolution is closer to the needed one, we've got a better match */
            found_delta  = delta;
            found_width  = w;
            found_height = h;

            match = TRUE;
        }
        else if (delta == found_delta)
        {
            /* If resolution is the same as that of current candidate mode, we can look at depth. */
            if (found_depth > depth)
            {
                /*
                 * Candidate mode if deeper than requested. We can supersede it with another mode
                 * of smaller depth, but which still matches our request.
                 */
                if ((d < found_depth) && (d >= depth))
                    match = TRUE;
            }
            else if (found_depth < depth)
            {
                /*
                 * We want better depth than current candidate.
                 * In this case anything deeper will do.
                 */
                if (d > found_depth)
                    match = TRUE;
            }
        }

        if (match)
        {
            /*
             * Mode with the same delta, but larger depth, may supersede
             * previous mode, if we prefer deeper ones.
             */
            D(bug("Selected (%ldx%ldx%ld, delta = %u)", w, h, d, delta));
            found_depth = d;
            found_mode  = mode;
        }
        D(bug("\n"));
    }

    /* Store mode information */ 
    compdata->displayrect.MinX = 0;
    compdata->displayrect.MinY = 0;
    compdata->displayrect.MaxX = found_width  - 1;
    compdata->displayrect.MaxY = found_height - 1;
    *res_depth = found_depth;

    return found_mode;
}

/* Follows the resolution of biggest screen */
static void CalculateParametersAlpha(struct HIDDCompositorData *compdata, ULONG *wantedwidth,
        ULONG *wantedheight, ULONG *wanteddepth)
{
    struct StackBitMapNode  *n;
    OOP_Object              *sync, *pf;
    IPTR                    curwidth, curheight, curdepth, modeid;

    for (n = (struct StackBitMapNode *)compdata->bitmapstack.mlh_TailPred;
             n->n.mln_Pred; n = (struct StackBitMapNode *)n->n.mln_Pred)
    {
        if ((!(n->sbmflags & COMPF_ALPHA)) && (n->sbmflags & STACKNODEF_DISPLAYABLE))
        {
            OOP_GetAttr(n->bm, aHidd_BitMap_ModeID, &modeid);
            HIDD_Gfx_GetMode(compdata->gfx, modeid, &sync, &pf);

            if (sync)
            {
                /* Get the needed size */
                OOP_GetAttr(sync, aHidd_Sync_HDisp, &curwidth);
                OOP_GetAttr(sync, aHidd_Sync_VDisp, &curheight);
            }
            else
            {
                OOP_GetAttr(n->bm, aHidd_BitMap_Width, &curwidth);
                OOP_GetAttr(n->bm, aHidd_BitMap_Height, &curheight);
            }

            if (OOP_GET(pf, aHidd_PixFmt_ColorModel) == vHidd_ColorModel_TrueColor)
            {
                OOP_GetAttr(pf, aHidd_PixFmt_Depth, &curdepth);
            }
            else
            {
                /* If we have a LUT bitmap on stack, we request atleast 16-bit screen.
                   if DEEPLUT is set be use 24bit instead (for better color matching) */
                if (compdata->flags & COMPSTATEF_DEEPLUT)
                    curdepth = 24;
                else
                    curdepth = 16;
            }
        }
        else
        {
            OOP_GetAttr(n->bm, aHidd_BitMap_Width, &curwidth);
            OOP_GetAttr(n->bm, aHidd_BitMap_Height, &curheight);

            if (n->sbmflags & COMPF_ALPHA)
                curdepth = 24;
            else
            {
                OOP_GetAttr(n->bm, aHidd_BitMap_PixFmt, (IPTR *)&pf);
                if (OOP_GET(pf, aHidd_PixFmt_ColorModel) == vHidd_ColorModel_TrueColor)
                {
                    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &curdepth);
                }
                else
                {
                    if (compdata->flags & COMPSTATEF_DEEPLUT)
                        curdepth = 24;
                    else
                        curdepth = 16;
                }
            }
        }
        if ((ULONG)curwidth > *wantedwidth)
            *wantedwidth = (ULONG)curwidth;
        if ((ULONG)curheight > *wantedheight)
            *wantedheight = (ULONG)curheight;
        if ((ULONG)curdepth > *wanteddepth)
            *wanteddepth = (ULONG)curdepth;
    }
}

/* Follows the resolution of topmost screen */
static void CalculateParametersRegular(struct HIDDCompositorData *compdata, ULONG *wantedwidth,
        ULONG *wantedheight, ULONG *wanteddepth)
{
    struct StackBitMapNode  *n;
    OOP_Object              *sync, *pf;
    IPTR                    width, height, depth, modeid;

    /*
     * Examine all bitmaps in the stack to figure out the needed depth.
     * We need a maximum depth of all depths in order to keep correct colors.
     * But not less than 16 bits, because we can't compose on a LUT screen.
     *
     * If a LUT bitmap is present in the stack (depth < 9), we request truecolor
     * screen for better color presentation.
     *
     * We examine bitmaps in reverse order, in this case 'sync' will hold
     * information about the top bitmap when we exit the loop.
     * Size of our composited mode needs to be as close as possible to that one.
     */

     for (n = (struct StackBitMapNode *)compdata->bitmapstack.mlh_TailPred;
          n->n.mln_Pred; n = (struct StackBitMapNode *)n->n.mln_Pred)
     {
         OOP_GetAttr(n->bm, aHidd_BitMap_ModeID, &modeid);
         HIDD_Gfx_GetMode(compdata->gfx, modeid, &sync, &pf);

         if (OOP_GET(pf, aHidd_PixFmt_ColorModel) == vHidd_ColorModel_TrueColor)
         {
             OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
             if ((ULONG)depth > *wanteddepth)
                 *wanteddepth = (ULONG)depth;
         }
         else
         {
             /*
              * If we have a LUT bitmap on stack, we request 24-bit screen
              * for better color transfer.
              */
             *wanteddepth = 24;
         }
     }

     /* Get the needed size */
     OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
     OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);

     *wantedwidth   = (ULONG)width;
     *wantedheight  = (ULONG)height;
}

void UpdateDisplayMode(struct HIDDCompositorData *compdata)
{
    ULONG                   wantedwidth = 0, wantedheight = 0, wanteddepth = 16;
    ULONG                   found_depth;
    IPTR                    modeid;

    D(bug("[Compositor] %s()\n", __PRETTY_FUNCTION__));

    if (compdata->flags & COMPSTATEF_HASALPHA)
        CalculateParametersAlpha(compdata, &wantedwidth, &wantedheight, &wanteddepth);
    else
        CalculateParametersRegular(compdata, &wantedwidth, &wantedheight, &wanteddepth);

    D(bug("[%s] Prefered mode %ldx%ldx%d\n", __PRETTY_FUNCTION__, wantedwidth, wantedheight, wanteddepth));

    modeid = FindBestHiddMode(compdata, wantedwidth, wantedheight, wanteddepth, &found_depth);
    D(bug("[%s] Composition Display ModeID 0x%08X [current 0x%08X]\n", __PRETTY_FUNCTION__, modeid, compdata->displaymode));

    if (modeid != compdata->displaymode)
    {
        /* The mode is different. Need to prepare information needed for compositing */
        struct TagItem gctags[] =
        {
            { aHidd_GC_Foreground, 0x99999999 }, 
            { TAG_DONE           , 0          }
        };

        /* Signal mode change */ 
        compdata->displaymode = modeid;
        compdata->displaydepth = wanteddepth;
        compdata->modeschanged = TRUE;

        /* Get gray foregound */
        if (found_depth < 24)
            gctags[0].ti_Data = 0x9492;

        OOP_SetAttrs(compdata->gc, gctags);
    }
}

