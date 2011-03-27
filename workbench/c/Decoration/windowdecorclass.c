/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#include <clib/alib_protos.h>

#include <intuition/windecorclass.h>
#include <intuition/extensions.h>
#include <intuition/gadgetclass.h>
#include <graphics/rpattr.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <string.h>

#include <libraries/mui.h> /* TODO: REMOVE needed for get() */

#include "windowdecorclass.h"
#include "screendecorclass.h"
#include "drawfuncs.h"
#include "config.h"

#define SETIMAGE_WIN(id) wd->img_##id=sd->di.img_##id

struct windecor_data
{
    /* Pointers to images used for sys images */
    struct NewImage *img_close;
    struct NewImage *img_depth;
    struct NewImage *img_zoom;
    struct NewImage *img_up;
    struct NewImage *img_down;
    struct NewImage *img_left;
    struct NewImage *img_right;
    struct NewImage *img_mui;
    struct NewImage *img_popup;
    struct NewImage *img_snapshot;
    struct NewImage *img_iconify;
    struct NewImage *img_lock;

    BOOL             outline;
    BOOL             shadow;
    BOOL             barmasking;
    BOOL             closeright;
    BOOL             threestate;
    BOOL             barvert;
    BOOL             usegradients;
    BOOL             rounded;
    BOOL             filltitlebar;

    UWORD            winbarheight;
    UWORD            txt_align;

    LONG             BarJoinTB_o;
    LONG             BarJoinTB_s;
    LONG             BarPreGadget_o;
    LONG             BarPreGadget_s;
    LONG             BarPre_o;
    LONG             BarPre_s;
    LONG             BarLGadgetFill_o;
    LONG             BarLGadgetFill_s;
    LONG             BarJoinGB_o;
    LONG             BarJoinGB_s;
    LONG             BarLFill_o;
    LONG             BarLFill_s;
    LONG             BarJoinBT_o;
    LONG             BarJoinBT_s;
    LONG             BarTitleFill_o;
    LONG             BarTitleFill_s;
    LONG             BarRFill_o;
    LONG             BarRFill_s;
    LONG             BarJoinBG_o;
    LONG             BarJoinBG_s;
    LONG             BarRGadgetFill_o;
    LONG             BarRGadgetFill_s;
    LONG             BarPostGadget_o;
    LONG             BarPostGadget_s;
    LONG             BarPost_o;
    LONG             BarPost_s;

    LONG             ContainerTop_o, ContainerTop_s;
    LONG             ContainerVertTile_o, ContainerVertTile_s;
    LONG             ContainerBottom_o, ContainerBottom_s;
    LONG             KnobTop_o, KnobTop_s;
    LONG             KnobTileTop_o, KnobTileTop_s;
    LONG             KnobVertGripper_o, KnobVertGripper_s;
    LONG             KnobTileBottom_o, KnobTileBottom_s;
    LONG             KnobBottom_o, KnobBottom_s;
    LONG             ContainerLeft_o, ContainerLeft_s;
    LONG             ContainerHorTile_o, ContainerHorTile_s;
    LONG             ContainerRight_o, ContainerRight_s;
    LONG             KnobLeft_o, KnobLeft_s;
    LONG             KnobTileLeft_o, KnobTileLeft_s;
    LONG             KnobHorGripper_o, KnobHorGripper_s;
    LONG             KnobTileRight_o, KnobTileRight_s;
    LONG             KnobRight_o, KnobRight_s;
    LONG             sizeaddx, sizeaddy;
    LONG             updownaddx, updownaddy;
    LONG             leftrightaddx, leftrightaddy;
    LONG             rightbordergads, bottombordergads;
    LONG             rightbordernogads, bottombordernogads;
    LONG             horscrollerheight;
    LONG             scrollerinnerspacing;
    LONG             a_arc, d_arc;
    LONG             a_col_s, a_col_e;
    LONG             d_col_s, d_col_e;
    LONG             b_col_a, b_col_d;
    LONG             light, middle, dark;
    LONG             text_col, shadow_col;
};


static BOOL HasPropGadgetChanged(struct CachedPropGadget * cached, 
    struct Rectangle * proprect, struct Rectangle * renderrect, struct Window * window)
{
    /* if knob position has changed */
    if (cached->knobx != (renderrect->MinX - proprect->MinX))
        return  TRUE;
    if (cached->knoby != (renderrect->MinY - proprect->MinY))
        return  TRUE;

    /* if the window activity status has changed */
    if (cached->windowflags ^ (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX)))
        return  TRUE;

    /* If the size has changed */
    if (cached->width != (proprect->MaxX - proprect->MinX + 1))
        return  TRUE;
    if (cached->height != (proprect->MaxY - proprect->MinY + 1))
        return  TRUE;

    /* If knob size has changed */
    if (cached->knobwidth != (renderrect->MaxX - renderrect->MinX + 1))
        return  TRUE;
    if (cached->knobheight != (renderrect->MaxY - renderrect->MinY + 1))
        return  TRUE;

    /* If there is no cached bitmap at all (this can happen NOT only at first call) */
    if (cached->bm == NULL)
        return  TRUE;

    return FALSE;
}

static VOID CachePropGadget(struct CachedPropGadget * cached, 
    struct Rectangle * proprect, struct Rectangle * renderrect, 
    struct Window * window, struct BitMap * bitmap)
{
    cached->bm         = bitmap;
    cached->width      = proprect->MaxX - proprect->MinX + 1;
    cached->height     = proprect->MaxY - proprect->MinY + 1;
    cached->windowflags= (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX));
    cached->knobwidth  = renderrect->MaxX - renderrect->MinX + 1;
    cached->knobheight = renderrect->MaxY - renderrect->MinY + 1;
    cached->knobx      = renderrect->MinX - proprect->MinX;
    cached->knoby      = renderrect->MinY - proprect->MinY;
}

static int WriteTiledImageShape(BOOL fill, struct Window *win, struct NewLUT8Image *lut8, struct NewImage *ni, int sx, int sy, int sw, int sh, int xp, int yp, int dw, int dh)
{
    int     w = dw;
    int     x = xp;
    int     ddw;

    if (!ni->ok) return xp;

    if ((sw == 0) || (dw == 0)) return xp;

    if (win)
    {
        if (x > win->Width) return xp;
        if ((x + w) > win->Width) w = win->Width - x;
    }

    while (w > 0)
    {
        ddw = sw;
        if (w < ddw) ddw = w;
        WriteAlphaPixelArray(ni, lut8, sx, sy, x, yp, ddw, dh);
        w -= ddw;
        x += ddw;
    }
    return x;
}

static void getrightgadgetsdimensions(struct windecor_data *data, struct Window *win, int *xs, int *xe)
{
    struct Gadget *g;

    int     x0 = 1000000;
    int     x1 = 0;
    UWORD   type;

    for (g = win->FirstGadget; g; g = g->NextGadget)
    {
        if ((g->Flags & GFLG_RELRIGHT) == GFLG_RELRIGHT)
        {
            type = g->GadgetType & GTYP_SYSTYPEMASK;
            if (data->closeright)
            {
                if (((type & (GTYP_CLOSE | GTYP_WDEPTH | GTYP_WZOOM)) != 0) || (g->Activation & GACT_TOPBORDER))
                {
                    if (g->Width > 0)
                    {
                        if ((win->Width + g->LeftEdge) < x0) x0 = win->Width + g->LeftEdge;
                        if ((win->Width + g->LeftEdge + g->Width) > x1) x1 = win->Width + g->LeftEdge + g->Width;
                    }
                }
            }
            else
            {
                if (((type & (GTYP_WDEPTH | GTYP_WZOOM)) != 0)  || (g->Activation & GACT_TOPBORDER))
                {
                    if (g->Width > 0)
                    {
                        if ((win->Width + g->LeftEdge) < x0) x0 = win->Width + g->LeftEdge;
                        if ((win->Width + g->LeftEdge + g->Width) > x1) x1 = win->Width + g->LeftEdge + g->Width;
                    }
                }
            }
        }
    }
    if (x0 == 1000000) x0 = 0;
    *xs = x0;
    *xe = x1;
}

static void getleftgadgetsdimensions(struct windecor_data *data, struct Window *win, int *xs, int *xe)
{
    struct Gadget *g;

    int w = 0;
    int x0 = 1000000;
    int x1 = 0;
    for (g = win->FirstGadget; g; g = g->NextGadget)
    {
        w++;
        if (((g->Flags & GFLG_RELRIGHT) == 0) && (g->Activation & GACT_TOPBORDER))
        {
            if ((g->GadgetType & GTYP_WDRAGGING) == 0)
            {
                if (g->Width > 0)
                {
                    if (g->LeftEdge < x0) x0 = g->LeftEdge;
                    if ((g->LeftEdge + g->Width) > x1) x1 = g->LeftEdge + g->Width;
                }
            }
        }
    }
    if (x0 == 1000000) x0 = 0;
    *xs = x0;
    *xe = x1;
}

static void DrawShapePartialTitleBar(struct WindowData *wd, struct NewLUT8Image *shape, struct windecor_data *data, struct Window *window, UWORD align, UWORD start, UWORD width)
{
    int                 xl0, xl1, xr0, xr1, defwidth;
    ULONG               textlen = 0, titlelen = 0, textpixellen = 0;
    struct TextExtent   te;

    BOOL                hastitle;
    BOOL                hastitlebar;
    UWORD               textstart = 0, barh, x;
    int                     dy;

    struct RastPort    *rp = &window->WScreen->RastPort;
    hastitle = window->Title != NULL ? TRUE : FALSE;
    hastitlebar = (window->BorderTop == data->winbarheight) ? TRUE : FALSE;

    if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX))
    {
        dy = 0;
    }
    else
    {
        dy = data->winbarheight;
    }
    getleftgadgetsdimensions(data, window, &xl0, &xl1);
    getrightgadgetsdimensions(data, window, &xr0, &xr1);

    defwidth = (xl0 != xl1) ? data->BarPreGadget_s : data->BarPre_s;
    if(xr1 == 0)
    {
        xr1 = window->Width - data->BarPre_s;
        xr0 = window->Width - data->BarPre_s;
    }

    defwidth += (xl1 - xl0);

    defwidth += data->BarJoinGB_s;
    defwidth += data->BarJoinBT_s;
    defwidth += data->BarJoinTB_s;
    defwidth += data->BarJoinBG_s;
    defwidth += (xr1 - xr0);
    defwidth += (xr0 != xr1) ? data->BarPostGadget_s : data->BarPost_s;

    if (defwidth >= window->Width) hastitle = FALSE;
 
    if (hastitle)
    {
        titlelen = strlen(window->Title);
        textlen = TextFit(rp, window->Title, titlelen, &te, NULL, 1, window->Width-defwidth, window->BorderTop - 2);
        if (textlen)
        {
            textpixellen = te.te_Extent.MaxX - te.te_Extent.MinX + 1;
        }
    }

 
    if (wd->img_winbar_normal->ok && hastitlebar)
    {
        barh =  wd->img_winbar_normal->h;
        if (data->barvert)
        {
            if (barh > data->winbarheight) barh =  data->winbarheight;
        }
        x = 0;
        if (xl0 != xl1)
        {
            x = WriteTiledImageShape(data->filltitlebar, window, shape, wd->img_winbar_normal, data->BarPreGadget_o, dy, data->BarPreGadget_s, barh, x, 0, data->BarPreGadget_s, barh);
            if ((xl1-xl0) > 0) x = WriteTiledImageShape(data->filltitlebar, window, shape, wd->img_winbar_normal, data->BarLGadgetFill_o, dy, data->BarLGadgetFill_s, barh, x, 0, xl1-xl0, barh);
        }
        else
        {
            x = WriteTiledImageShape(data->filltitlebar, window, shape, wd->img_winbar_normal, data->BarPre_o, dy, data->BarPre_s, barh, x, 0, data->BarPreGadget_s, barh);
        }
        x = WriteTiledImageShape(data->filltitlebar, window, shape, wd->img_winbar_normal, data->BarJoinGB_o, dy, data->BarJoinGB_s, barh, x, 0, data->BarJoinGB_s, barh);
        if (hastitle && (textlen > 0))
        {
            switch(align)
            {
                case WD_DWTA_CENTER:
                    //BarLFill
                    x = WriteTiledImageShape(data->filltitlebar, window, shape, wd->img_winbar_normal, data->BarLFill_o, dy, data->BarLFill_s, barh, x, 0, 60, barh);
                    break;
                case WD_DWTA_RIGHT:
                    //BarLFill
                    break;
                default:
                case WD_DWTA_LEFT:
                    break;
            }
            x = WriteTiledImageShape(data->filltitlebar, window, shape, wd->img_winbar_normal, data->BarJoinBT_o, dy, data->BarJoinBT_s, barh, x, 0, data->BarJoinBT_s, barh);
            textstart = x;
            if (textpixellen > 0) x = WriteTiledImageShape(data->filltitlebar, window, shape, wd->img_winbar_normal, data->BarTitleFill_o, dy, data->BarTitleFill_s, barh, x, 0, textpixellen, barh);
            x = WriteTiledImageShape(data->filltitlebar, window, shape, wd->img_winbar_normal, data->BarJoinTB_o, dy, data->BarJoinTB_s, barh, x, 0, data->BarJoinTB_s, barh);
        }
        x = WriteTiledImageShape(data->filltitlebar, window, shape, wd->img_winbar_normal, data->BarRFill_o, dy, data->BarRFill_s, barh, x, 0, xr0 - x - data->BarJoinBG_s, barh);
        x = WriteTiledImageShape(data->filltitlebar, window, shape, wd->img_winbar_normal, data->BarJoinBG_o, dy, data->BarJoinBG_s, barh, x, 0, data->BarJoinBG_s, barh);
        if ((xr1-xr0) > 0) x = WriteTiledImageShape(data->filltitlebar, window, shape, wd->img_winbar_normal, data->BarRGadgetFill_o, dy, data->BarRGadgetFill_s, barh, x, 0, xr1-xr0, barh);
        if (xr0 != xr1)
        {
            x = WriteTiledImageShape(data->filltitlebar, window, shape, wd->img_winbar_normal, data->BarPostGadget_o, dy, data->BarPostGadget_s, barh, x, 0, data->BarPostGadget_s, barh);
        }
        else
        {
            x = WriteTiledImageShape(data->filltitlebar, window, shape, wd->img_winbar_normal, data->BarPost_o, dy, data->BarPost_s, barh, x, 0, data->BarPost_s, barh);
        }
    }
}

static VOID DrawPartialTitleBar(struct WindowData *wd, struct windecor_data *data, struct Window *window, struct RastPort *dst_rp, struct DrawInfo *dri, UWORD align, UWORD start, UWORD width, UWORD *pens)
{
    int                 xl0, xl1, xr0, xr1, defwidth;
    ULONG               textlen = 0, titlelen = 0, textpixellen = 0;
    struct TextExtent   te;
    struct RastPort    *rp;
    struct NewImage    *ni = NULL;

    BOOL                hasdepth;
    BOOL                haszoom;
    BOOL                hasclose;
    BOOL                hasdrag;
    BOOL                hastitle;
    BOOL                hastitlebar;
    UWORD               textstart = 0, barh, x;
    ULONG                   bc, color, s_col, e_col, arc;
    int                     dy;

    LONG    pen = -1;

    if ((wd->rp == NULL) || (window->Width != wd->w) || (window->BorderTop != wd->h))
    {
        if (wd->rp)
        {
            FreeBitMap(wd->rp->BitMap);
            FreeRastPort(wd->rp);
        }



        wd->h = window->BorderTop;
        wd->w = window->Width;
        wd->rp = NULL;


        rp = CreateRastPort();
        if (rp)
        {
            SetFont(rp, dri->dri_Font);
            rp->BitMap = AllocBitMap(window->Width, window->BorderTop, 1, 0, window->WScreen->RastPort.BitMap);
            if (rp->BitMap == NULL)
            {
                FreeRastPort(rp);
                return;
            }
        } else return;

        wd->rp = rp;

    } else rp = wd->rp;

    hastitle = window->Title != NULL ? TRUE : FALSE;
    hasclose = (window->Flags & WFLG_CLOSEGADGET) ? TRUE : FALSE;
    hasdepth = (window->Flags & WFLG_DEPTHGADGET) ? TRUE : FALSE;
    hasdrag = (window->Flags & WFLG_DRAGBAR) ? TRUE : FALSE;
    haszoom = ((window->Flags & WFLG_HASZOOM) || ((window->Flags & WFLG_SIZEGADGET) && hasdepth)) ? TRUE : FALSE;
    hastitlebar = (window->BorderTop == data->winbarheight) ? TRUE : FALSE;

    if (wd->img_border_normal->ok) ni = wd->img_border_normal;

    if (ni == NULL) data->usegradients = TRUE;

    color = 0x00cccccc;

    if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX))
    {
        s_col = data->a_col_s;
        e_col = data->a_col_e;
        arc = data->a_arc;
        dy = 0;
        bc = data->b_col_a;
        pen = wd->ActivePen;
    } else {
        s_col = data->d_col_s;
        e_col = data->d_col_e;
        arc = data->d_arc;
        dy = data->winbarheight;
        bc = data->b_col_d;
        if (!data->usegradients)
        {
            if (wd->img_border_deactivated->ok) ni = wd->img_border_deactivated;
        }
        pen = wd->DeactivePen;
    }


    if (data->filltitlebar)
    {
        if (data->usegradients) FillPixelArrayGradient(pen, wd->truecolor, rp, 0, 0, window->Width - 1, window->Height - 1, 0, 0, window->Width, window->BorderTop, s_col, e_col, arc);
        else DrawTileToRP(rp, ni, color, 0, 0, 0, 0, window->Width, window->BorderTop);
    }
    
    if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX))
    {
        dy = 0;
    }
    else
    {
        dy = data->winbarheight;
    }
    getleftgadgetsdimensions(data, window, &xl0, &xl1);
    getrightgadgetsdimensions(data, window, &xr0, &xr1);
    defwidth = (xl0 != xl1) ? data->BarPreGadget_s : data->BarPre_s;
    if(xr1 == 0)
    {
        xr1 = window->Width - data->BarPre_s;
        xr0 = window->Width - data->BarPre_s;
    }

    defwidth += (xl1 - xl0);

    defwidth += data->BarJoinGB_s;
    defwidth += data->BarJoinBT_s;
    defwidth += data->BarJoinTB_s;
    defwidth += data->BarJoinBG_s;
    defwidth += (xr1 - xr0);
    defwidth += (xr0 != xr1) ? data->BarPostGadget_s : data->BarPost_s;

    if (defwidth >= window->Width) hastitle = FALSE;

    if (hastitle)
    {
        titlelen = strlen(window->Title);
        textlen = TextFit(rp, window->Title, titlelen, &te, NULL, 1, window->Width-defwidth, window->BorderTop - 2);
        if (textlen)
        {
            textpixellen = te.te_Extent.MaxX - te.te_Extent.MinX + 1;
        }
    }

    if (wd->img_winbar_normal->ok && hastitlebar)
    {
        barh =  wd->img_winbar_normal->h;
        if (data->barvert)
        {
            if (barh > data->winbarheight) barh =  data->winbarheight;
        }
        x = 0;
        if (xl0 != xl1)
        {
            x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarPreGadget_o, dy, data->BarPreGadget_s, barh, x, 0, data->BarPreGadget_s, barh);
            if ((xl1-xl0) > 0) x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarLGadgetFill_o, dy, data->BarLGadgetFill_s, barh, x, 0, xl1-xl0, barh);
        }
        else
        {
            x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarPre_o, dy, data->BarPre_s, barh, x, 0, data->BarPreGadget_s, barh);
        }
        x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarJoinGB_o, dy, data->BarJoinGB_s, barh, x, 0, data->BarJoinGB_s, barh);
        if (hastitle && (textlen > 0))
        {
            switch(align)
            {
                case WD_DWTA_CENTER:
                    //BarLFill
                    x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarLFill_o, dy, data->BarLFill_s, barh, x, 0, 60, barh);
                    break;
                case WD_DWTA_RIGHT:
                    //BarLFill
                    break;
                default:
                case WD_DWTA_LEFT:
                    break;
            }
            x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarJoinBT_o, dy, data->BarJoinBT_s, barh, x, 0, data->BarJoinBT_s, barh);
            textstart = x;
            if (textpixellen > 0) x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarTitleFill_o, dy, data->BarTitleFill_s, barh, x, 0, textpixellen, barh);
            x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarJoinTB_o, dy, data->BarJoinTB_s, barh, x, 0, data->BarJoinTB_s, barh);
        }
        x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarRFill_o, dy, data->BarRFill_s, barh, x, 0, xr0 - x - data->BarJoinBG_s, barh);
        x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarJoinBG_o, dy, data->BarJoinBG_s, barh, x, 0, data->BarJoinBG_s, barh);
        if ((xr1-xr0) > 0) x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarRGadgetFill_o, dy, data->BarRGadgetFill_s, barh, x, 0, xr1-xr0, barh);
        if (xr0 != xr1)
        {
            x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarPostGadget_o, dy, data->BarPostGadget_s, barh, x, 0, data->BarPostGadget_s, barh);
        }
        else
        {
            x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarPost_o, dy, data->BarPost_s, barh, x, 0, data->BarPost_s, barh);
        }
    }

    if ((textlen > 0) && hastitle)
    {
        SetAPen(rp, pens[(window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX)) ? FILLTEXTPEN : TEXTPEN]);
        SetDrMd(rp, JAM1);
        UWORD   tx = textstart;
        UWORD   ty = ((data->winbarheight - dri->dri_Font->tf_YSize) >> 1) + dri->dri_Font->tf_Baseline;

        if (!wd->truecolor || ((data->outline == FALSE) && (data->shadow == FALSE)))
        {
            Move(rp, tx, ty);
            Text(rp, window->Title, textlen);
        }
        else if (data->outline)
        {

                SetSoftStyle(rp, FSF_BOLD, AskSoftStyle(rp));
                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->shadow_col, TAG_DONE);

                Move(rp, tx + 1, ty ); Text(rp, window->Title, textlen);
                Move(rp, tx + 2, ty ); Text(rp, window->Title, textlen);
                Move(rp, tx , ty ); Text(rp, window->Title, textlen);
                Move(rp, tx, ty + 1);  Text(rp, window->Title, textlen);
                Move(rp, tx, ty + 2);  Text(rp, window->Title, textlen);
                Move(rp, tx + 1, ty + 2);  Text(rp, window->Title, textlen);
                Move(rp, tx + 2, ty + 1);  Text(rp, window->Title, textlen);
                Move(rp, tx + 2, ty + 2);  Text(rp, window->Title, textlen);

                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->text_col, TAG_DONE);
                Move(rp, tx + 1, ty + 1);
                Text(rp, window->Title, textlen);
                SetSoftStyle(rp, FS_NORMAL, AskSoftStyle(rp));
        }
        else
        {
                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->shadow_col, TAG_DONE);
                Move(rp, tx + 1, ty + 1 );
                Text(rp, window->Title, textlen);

                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->text_col, TAG_DONE);
                Move(rp, tx, ty);
                Text(rp, window->Title, textlen);

        }
    }
    struct Gadget *g;

    for (g = window->FirstGadget; g; g = g->NextGadget)
    {
        if (g->Activation & GACT_TOPBORDER && (g->GadgetType & GTYP_SYSTYPEMASK) != GTYP_WDRAGGING)
        {
            int x, y;
            y = g->TopEdge;
            if (!(g->Flags & GFLG_RELRIGHT))
            {
                x = g->LeftEdge;
            }
            else
            {
                x = g->LeftEdge + window->Width - 1;
            }
            struct NewImage *ni = NULL;
            UWORD state = IDS_NORMAL;

            if ((window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX)) == 0)
            {
                state = IDS_INACTIVENORMAL;
            }
            else  if (g->Flags & GFLG_SELECTED) state = IDS_SELECTED;

            if (g->GadgetType & GTYP_SYSTYPEMASK) {
                switch(g->GadgetType & GTYP_SYSTYPEMASK)
                {
                    case GTYP_CLOSE:
                        ni = wd->img_close;
                        break;
                    case GTYP_WDEPTH:
                        ni = wd->img_depth;
                        break;
                    case GTYP_WZOOM:
                        ni = wd->img_zoom;
                        break;
                }
            }
            else
            {
                switch(g->GadgetID)
                {
                    case ETI_MUI:
                        ni = wd->img_mui;
                        break;

                    case ETI_PopUp:
                        ni = wd->img_popup;
                        break;

                    case ETI_Snapshot:
                        ni = wd->img_snapshot;
                        break;

                    case ETI_Iconify:
                        ni = wd->img_iconify;
                        break;

                    case ETI_Lock:
                        ni = wd->img_lock;
                        break;
                }
            }

            if (ni) DrawAlphaStateImageToRP((data->threestate ? 3 : 4)/*data*/, rp, ni, state, x, y, TRUE);
        }
    }
    BltBitMapRastPort(rp->BitMap, start, 0, dst_rp, start, 0, width, window->BorderTop, 0xc0);
}

static VOID DisposeWindowSkinning(struct windecor_data *data)
{

}

static BOOL InitWindowSkinning(STRPTR path, struct windecor_data *data, struct DecorImages * di) 
{
    char    buffer[256];
    char    *line, *v;
    BPTR    file;
    BPTR    lock;
    BPTR    olddir = 0;

    lock = Lock(path, ACCESS_READ);
    if (lock)
    {
        olddir = CurrentDir(lock);
    }
    else return FALSE;

    data->rounded = FALSE;
    data->threestate = FALSE;
    data->barmasking = FALSE;
    data->winbarheight = 0; //screen, window

    data->sizeaddx = 2;
    data->sizeaddy = 2;
    data->BarJoinTB_o = 0;
    data->BarJoinTB_s = 0;
    data->BarPreGadget_o = 0;
    data->BarPreGadget_s = 0;
    data->BarPre_o = 0;
    data->BarPre_s = 0;
    data->BarLGadgetFill_o = 0;
    data->BarLGadgetFill_s = 0;
    data->BarJoinGB_o = 0;
    data->BarJoinGB_s = 0;
    data->BarLFill_o = 0;
    data->BarLFill_s = 0;
    data->BarJoinBT_o = 0;
    data->BarJoinBT_s = 0;
    data->BarTitleFill_o = 0;
    data->BarTitleFill_s = 0;
    data->BarRFill_o = 0;
    data->BarRFill_s = 0;
    data->BarJoinBG_o = 0;
    data->BarJoinBG_s = 0;
    data->BarRGadgetFill_o = 0;
    data->BarRGadgetFill_s = 0;
    data->BarPostGadget_o = 0;
    data->BarPostGadget_s = 0;
    data->BarPost_o = 0;
    data->BarPost_s = 0;
    data->txt_align = WD_DWTA_LEFT;
    data->usegradients = FALSE;
    data->closeright = FALSE;
    data->barvert = FALSE;
    data->filltitlebar = FALSE;
    data->outline = FALSE;
    data->shadow = FALSE;

    data->a_col_s = 0xaaaaaaaa;
    data->a_col_e = 0xeeeeeeff;
    data->d_col_s = 0x66666666;
    data->d_col_e = 0xaaaaaabb;

    data->text_col = 0x00cccccc;
    data->shadow_col = 0x00444444;

    data->a_arc = 0;
    data->d_arc = 0;
    data->light = 320;
    data->middle = 240;
    data->dark = 128;

    file = Open("System/Config", MODE_OLDFILE);
    if (file)
    {
        do
        {
            line = FGets(file, buffer, 256);
            if (line)
            {
                if ((v = strstr(line, "NoInactiveSelected ")) == line) {
                    data->threestate = GetBool(v, "Yes");
                } else if ((v = strstr(line, "BarRounded ")) == line) {
                    data->rounded = GetBool(v, "Yes");
                } else if ((v = strstr(line, "WindowTitleMode ")) == line) {
                    data->outline = GetBool(v, "Outline");
                    data->shadow = GetBool(v, "Shadow");
                } else if ((v = strstr(line, "FillTitleBar ")) == line) {
                    data->filltitlebar = GetBool(v, "Yes");
                } else if ((v = strstr(line, "BarMasking ")) == line) {
                    data->barmasking = GetBool(v, "Yes");
                } else if ((v = strstr(line, "CloseRight ")) == line) {
                    data->closeright = GetBool(v, "Yes");
                } else if ((v = strstr(line, "UseGradients ")) == line) {
                    data->usegradients = GetBool(v, "Yes");
                } else if ((v = strstr(line, "BarLayout ")) == line) {
                    data->barvert = GetBool(v, "Vertical");
                } else  if ((v = strstr(line, "RightBorderGads ")) == line) {
                    data->rightbordergads = GetInt(v);
                } else  if ((v = strstr(line, "HorScrollerHeight ")) == line) {
                    data->horscrollerheight = GetInt(v);
                } else  if ((v = strstr(line, "ScrollerInnerSpacing ")) == line) {
                    data->scrollerinnerspacing = GetInt(v);
                } else  if ((v = strstr(line, "BottomBorderGads ")) == line) {
                    data->bottombordergads = GetInt(v);
                } else  if ((v = strstr(line, "RightBorderNoGads ")) == line) {
                    data->rightbordernogads = GetInt(v);
                } else  if ((v = strstr(line, "BottomBorderNoGads ")) == line) {
                    data->bottombordernogads = GetInt(v);
                } else  if ((v = strstr(line, "BarHeight ")) == line) {
                    data->winbarheight = GetInt(v); //screen, window
                } else  if ((v = strstr(line, "BarJoinTB ")) == line) {
                    GetIntegers(v, &data->BarJoinTB_o, &data->BarJoinTB_s);
                } else  if ((v = strstr(line, "BarPreGadget ")) == line) {
                    GetIntegers(v, &data->BarPreGadget_o, &data->BarPreGadget_s);
                } else  if ((v = strstr(line, "BarPre ")) == line) {
                    GetIntegers(v, &data->BarPre_o, &data->BarPre_s);
                } else  if ((v = strstr(line, "BarLGadgetFill ")) == line) {
                    GetIntegers(v, &data->BarLGadgetFill_o, &data->BarLGadgetFill_s);
                } else  if ((v = strstr(line, "BarJoinGB ")) == line) {
                    GetIntegers(v, &data->BarJoinGB_o, &data->BarJoinGB_s);
                } else  if ((v = strstr(line, "BarLFill ")) == line) {
                    GetIntegers(v, &data->BarLFill_o, &data->BarLFill_s);
                } else  if ((v = strstr(line, "BarJoinBT ")) == line) {
                    GetIntegers(v, &data->BarJoinBT_o, &data->BarJoinBT_s);
                } else  if ((v = strstr(line, "BarTitleFill ")) == line) {
                    GetIntegers(v, &data->BarTitleFill_o, &data->BarTitleFill_s);
                } else  if ((v = strstr(line, "BarRFill ")) == line) {
                    GetIntegers(v, &data->BarRFill_o, &data->BarRFill_s);
                } else  if ((v = strstr(line, "BarJoinBG ")) == line) {
                    GetIntegers(v, &data->BarJoinBG_o, &data->BarJoinBG_s);
                } else  if ((v = strstr(line, "BarRGadgetFill ")) == line) {
                    GetIntegers(v, &data->BarRGadgetFill_o, &data->BarRGadgetFill_s);
                } else  if ((v = strstr(line, "BarPostGadget ")) == line) {
                    GetIntegers(v, &data->BarPostGadget_o, &data->BarPostGadget_s);
                } else  if ((v = strstr(line, "BarPost ")) == line) {
                    GetIntegers(v, &data->BarPost_o, &data->BarPost_s);
                } else  if ((v = strstr(line, "ContainerTop ")) == line) {
                    GetIntegers(v, &data->ContainerTop_o, &data->ContainerTop_s);
                } else  if ((v = strstr(line, "ContainerVertTile ")) == line) {
                    GetIntegers(v, &data->ContainerVertTile_o, &data->ContainerVertTile_s);
                } else  if ((v = strstr(line, "KnobTop ")) == line) {
                    GetIntegers(v, &data->KnobTop_o, &data->KnobTop_s);
                } else  if ((v = strstr(line, "KnobTileTop ")) == line) {
                    GetIntegers(v, &data->KnobTileTop_o, &data->KnobTileTop_s);
                } else  if ((v = strstr(line, "KnobVertGripper ")) == line) {
                    GetIntegers(v, &data->KnobVertGripper_o, &data->KnobVertGripper_s);
                } else  if ((v = strstr(line, "KnobTileBottom ")) == line) {
                    GetIntegers(v, &data->KnobTileBottom_o, &data->KnobTileBottom_s);
                } else  if ((v = strstr(line, "KnobBottom ")) == line) {
                    GetIntegers(v, &data->KnobBottom_o, &data->KnobBottom_s);
                } else  if ((v = strstr(line, "ContainerBottom ")) == line) {
                    GetIntegers(v, &data->ContainerBottom_o, &data->ContainerBottom_s);
                } else  if ((v = strstr(line, "ContainerLeft ")) == line) {
                    GetIntegers(v, &data->ContainerLeft_o, &data->ContainerLeft_s);
                } else  if ((v = strstr(line, "ContainerHorTile ")) == line) {
                    GetIntegers(v, &data->ContainerHorTile_o, &data->ContainerHorTile_s);
                } else  if ((v = strstr(line, "KnobLeft ")) == line) {
                    GetIntegers(v, &data->KnobLeft_o, &data->KnobLeft_s);
                } else  if ((v = strstr(line, "KnobTileLeft ")) == line) {
                    GetIntegers(v, &data->KnobTileLeft_o, &data->KnobTileLeft_s);
                } else  if ((v = strstr(line, "KnobHorGripper ")) == line) {
                    GetIntegers(v, &data->KnobHorGripper_o, &data->KnobHorGripper_s);
                } else  if ((v = strstr(line, "KnobTileRight ")) == line) {
                    GetIntegers(v, &data->KnobTileRight_o, &data->KnobTileRight_s);
                } else  if ((v = strstr(line, "KnobRight ")) == line) {
                    GetIntegers(v, &data->KnobRight_o, &data->KnobRight_s);
                } else  if ((v = strstr(line, "ContainerRight ")) == line) {
                    GetIntegers(v, &data->ContainerRight_o, &data->ContainerRight_s);
                } else  if ((v = strstr(line, "AddSize ")) == line) {
                    GetIntegers(v, &data->sizeaddx, &data->sizeaddy);
                } else  if ((v = strstr(line, "AddUpDown ")) == line) {
                    GetIntegers(v, &data->updownaddx, &data->updownaddy);
                } else  if ((v = strstr(line, "AddLeftRight ")) == line) {
                    GetIntegers(v, &data->leftrightaddx, &data->leftrightaddy);
                } else  if ((v = strstr(line, "ActivatedGradient ")) == line) {
                    GetTripleIntegers(v, &data->a_col_s, &data->a_col_e, &data->a_arc);
                } else  if ((v = strstr(line, "DeactivatedGradient ")) == line) {
                    GetTripleIntegers(v, &data->d_col_s, &data->d_col_e, &data->d_arc);
                } else  if ((v = strstr(line, "ShadeValues ")) == line) {
                    GetTripleIntegers(v, &data->light, &data->middle, &data->dark);
                } else  if ((v = strstr(line, "BaseColors ")) == line) {
                    GetColors(v, &data->b_col_a, &data->b_col_d);
                } else  if ((v = strstr(line, "WindowTitleColors ")) == line) {
                    GetColors(v, &data->text_col, &data->shadow_col);
                }
            }
        }
        while(line);
        Close(file);
    }

    /* Set pointers to gadget images, used only to get gadget sizes as their
       are requested prior to creation of menu object */
    data->img_close     = di->img_close;
    data->img_depth     = di->img_depth;
    data->img_zoom      = di->img_zoom;
    data->img_up        = di->img_up;
    data->img_down      = di->img_down;
    data->img_left      = di->img_left;
    data->img_right     = di->img_right;
    data->img_mui       = di->img_mui;
    data->img_popup     = di->img_popup;
    data->img_snapshot  = di->img_snapshot;
    data->img_iconify   = di->img_iconify;
    data->img_lock      = di->img_lock;

    if (olddir) CurrentDir(olddir);
    UnLock(lock);

    if (data->img_close && data->img_depth && data->img_zoom && data->img_up && 
        data->img_down && data->img_left && data->img_right)
        return TRUE;

    DisposeWindowSkinning(data);
    return FALSE;
}

/**************************************************************************************************/
static IPTR windecor_new(Class *cl, Object *obj, struct opSet *msg)
{
    struct windecor_data *data;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
         data = INST_DATA(cl, obj);

         STRPTR path = (STRPTR) GetTagData(WDA_Configuration, (IPTR) "Theme:", msg->ops_AttrList);
         struct DecorImages * di = (struct DecorImages *) GetTagData(WDA_DecorImages, (IPTR)NULL, msg->ops_AttrList);

         if (!InitWindowSkinning(path, data, di))
         {
             CoerceMethod(cl, obj, OM_DISPOSE);
             obj = NULL;
         }
     }

    return (IPTR)obj;
}

/**************************************************************************************************/
static IPTR windecor_dispose(Class *cl, Object *obj, struct opSet *msg)
{
    struct windecor_data *data = INST_DATA(cl, obj);

    DisposeWindowSkinning(data);

    return 1;
}

/**************************************************************************************************/
static IPTR windecor_get(Class *cl, Object *obj, struct opGet *msg)
{
    switch(msg->opg_AttrID)
    {
        case WDA_TrueColorOnly:
            *msg->opg_Storage = TRUE;
            break;
        default:
            return DoSuperMethodA(cl, obj, (Msg)msg);
    }
    return 1;
}

/**************************************************************************************************/
static IPTR windecor_draw_sysimage(Class *cl, Object *obj, struct wdpDrawSysImage *msg)
{
    struct windecor_data   *data = INST_DATA(cl, obj);
    struct RastPort        *rp = msg->wdp_RPort;
    struct NewImage        *ni = NULL;
    struct WindowData      *wd = (struct WindowData *) msg->wdp_UserBuffer;
    LONG                    state = msg->wdp_State;
    LONG                    left = msg->wdp_X;
    LONG                    top = msg->wdp_Y;
    LONG                    width = msg->wdp_Width;
    LONG                    height = msg->wdp_Height;
    WORD                    addx = 0;
    WORD                    addy = 0;
    BOOL                    isset = FALSE;
    BOOL                    titlegadget = FALSE;

    switch(msg->wdp_Which)
    {
        case SIZEIMAGE:
            if (wd->img_size->ok)
            {
                ni = wd->img_size;
                isset = TRUE;
                if (data->threestate) 
                    addx = (data->rightbordergads - (wd->img_size->w / 3)) /2; 
                else 
                    addx = (data->rightbordergads - (wd->img_size->w >> 2)) /2;
                addy = (data->bottombordergads - wd->img_size->h) / 2;
            }
            break;

        case CLOSEIMAGE:
            if (wd->img_close->ok)
            {
                ni = wd->img_close;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case MUIIMAGE:
            if (wd->img_mui->ok)
            {
                ni = wd->img_mui;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case POPUPIMAGE:
            if (wd->img_popup->ok)
            {
                ni = wd->img_popup;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case SNAPSHOTIMAGE:
            if (wd->img_snapshot->ok)
            {
                ni = wd->img_snapshot;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case LOCKIMAGE:
            if (wd->img_lock->ok)
            {
                ni = wd->img_lock;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case ICONIFYIMAGE:
            if (wd->img_iconify->ok)
            {
                ni = wd->img_iconify;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case DEPTHIMAGE:
            if (wd->img_depth->ok)
            {
                ni = wd->img_depth;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case ZOOMIMAGE:
            if (wd->img_zoom->ok)
            {
                ni = wd->img_zoom;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case UPIMAGE:
            ni = wd->img_up;
            if (data->threestate) addx = (data->rightbordergads - (wd->img_up->w / 3)) /2; else addx = (data->rightbordergads - (wd->img_up->w >> 2)) /2;
            addy = data->updownaddy / 2;
            isset = TRUE;
            break;

        case DOWNIMAGE:
            ni = wd->img_down;
            if (data->threestate) addx = (data->rightbordergads - (wd->img_down->w / 3)) /2; else addx = (data->rightbordergads - (wd->img_down->w >> 2)) /2;
            addy = data->updownaddy / 2;
            isset = TRUE;
            break;

        case LEFTIMAGE:
            ni = wd->img_left;
            addx = data->leftrightaddx / 2;
            addy = (data->bottombordergads - wd->img_left->h) / 2;
            isset = TRUE;
            break;

        case RIGHTIMAGE:
            ni = wd->img_right;
            addx = data->leftrightaddx / 2;
            addy = (data->bottombordergads - wd->img_right->h) /2;
            isset = TRUE;
            break;

        default:
            return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    if (!isset) return DoSuperMethodA(cl, obj, (Msg)msg);

    if (wd && titlegadget) if (wd->rp) if (wd->rp->BitMap) BltBitMapRastPort(wd->rp->BitMap, left+addy, top+addy, rp, left+addy, top+addy, width, height, 0xc0);

    if (ni) DrawAlphaStateImageToRP((data->threestate ? 3 : 4)/*data*/, rp, ni, state, left+addx, top+addy, TRUE);

    return TRUE;
}

/**************************************************************************************************/
static IPTR windecor_draw_winborder(Class *cl, Object *obj, struct wdpDrawWinBorder *msg)
{
    struct windecor_data   *data = INST_DATA(cl, obj);
    struct RastPort        *rp = msg->wdp_RPort;
    struct Window          *window = msg->wdp_Window;
    struct WindowData      *wd = (struct WindowData *) msg->wdp_UserBuffer;
    struct NewImage        *ni = NULL;
    UWORD                  *pens = msg->wdp_Dri->dri_Pens;
    ULONG                   bc, color, s_col, e_col, arc;
    UWORD                   bl, bt, br, bb, ww, wh;
    LONG    pen = -1;
    int                     dy;

    if (wd->img_border_normal->ok) ni = wd->img_border_normal;

    if (ni == NULL) data->usegradients = TRUE;

    BOOL    tc = wd->truecolor;

    LONG    dpen = pens[SHADOWPEN];
    LONG    lpen = pens[SHINEPEN];
    LONG    mpen = pens[SHINEPEN];

    bl = window->BorderLeft;
    bt = window->BorderTop;
    bb = window->BorderBottom;
    br = window->BorderRight;
    ww = window->Width;
    wh = window->Height;

    color = 0x00cccccc;

    if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX))
    {
        pen = wd->ActivePen;
        s_col = data->a_col_s;
        e_col = data->a_col_e;
        arc = data->a_arc;
        dy = 0;
        bc = data->b_col_a;
    } else {
        pen = wd->DeactivePen;
        s_col = data->d_col_s;
        e_col = data->d_col_e;
        arc = data->d_arc;
        dy = data->winbarheight;
        bc = data->b_col_d;
        if (!data->usegradients)
        {
            if (wd->img_border_deactivated->ok) ni = wd->img_border_deactivated;
        }
    }

//     if (data->filltitlebar)
//     {
//         if (data->usegradients) FillPixelArrayGradient(pen, wd->truecolor, rp, 0, 0, window->Width, window->Height, 0, 0, window->Width, window->BorderTop, s_col, e_col, arc);
//         else DrawTileToRP(rp, ni, color, 0, 0, 0, 0, window->Width, window->BorderTop);
//     }

    if (window->BorderTop == data->winbarheight) DrawPartialTitleBar(wd, data, window, rp, msg->wdp_Dri, data->txt_align, 0, window->Width, pens);
    if (!(msg->wdp_Flags & WDF_DWB_TOP_ONLY))
    {
        if (window->BorderLeft > 2)
        {
            if (data->usegradients) FillPixelArrayGradient(pen, wd->truecolor, rp, 0, 0, window->Width-1, window->Height-1, 0, window->BorderTop, window->BorderLeft, window->Height - window->BorderTop, s_col, e_col, arc);
            else DrawTileToRP(rp, ni, color, 0, 0, 0, window->BorderTop, window->BorderLeft - 1, window->Height - window->BorderTop);
        }
        if (window->BorderRight > 2)
        {
            if (data->usegradients) FillPixelArrayGradient(pen, wd->truecolor, rp, 0, 0, window->Width-1, window->Height-1, window->Width - window->BorderRight , window->BorderTop, window->BorderRight, window->Height - window->BorderTop, s_col, e_col, arc);
            else DrawTileToRP(rp, ni, color, 0, 0, window->Width - window->BorderRight , window->BorderTop, window->BorderRight, window->Height - window->BorderTop);
        }
        if (window->BorderBottom > 2)
        {
            if (data->usegradients) FillPixelArrayGradient(pen, wd->truecolor, rp, 0, 0, window->Width-1, window->Height-1, 0, window->Height - window->BorderBottom , window->Width, window->BorderBottom, s_col, e_col, arc);
            else DrawTileToRP(rp, ni, color, 0, 0, 0, window->Height - window->BorderBottom , window->Width, window->BorderBottom);
        }

        int bbt = bt;

        if (bt != data->winbarheight) {
            int bq = 0;
            if (bt > 1) bq = bt - 1;
            if (window->BorderTop > 2)
            {
                if (data->usegradients) FillPixelArrayGradient(pen, wd->truecolor, rp, 0, 0, window->Width-1, window->Height-1, 0, 0 , window->Width - 1, window->BorderTop - 1, s_col, e_col, arc);
                else DrawTileToRP(rp, ni, color, 0, 0, 0, 0 , window->Width, window->BorderTop);
            }
            if (bt > 0) ShadeLine(dpen, tc, data->usegradients, rp, ni, bc, data->dark, 0, 0, 0, ww - 1, 0);
            if (bq > 0) ShadeLine(dpen, tc, data->usegradients, rp, ni, bc, data->dark, bq, 0, bq, ww - 1, bq);
            if (bt > 1) ShadeLine(lpen, tc, data->usegradients, rp, ni, bc, data->light, 1, 1, 1, ww - 2, 1);
            bbt = 0;
        }

        if (bl > 0) ShadeLine(dpen, tc, data->usegradients, rp, ni, bc, data->dark, bbt, 0, bbt, 0, wh - 1);
        if (bb > 0) ShadeLine(dpen, tc, data->usegradients, rp, ni, bc, data->dark, wh - 1, 0, wh - 1, ww - 1, wh - 1);
        if (br > 0) ShadeLine(dpen, tc, data->usegradients, rp, ni, bc, data->dark, bbt , ww - 1, bbt , ww - 1, wh - 1);
        if (bl > 1) ShadeLine(dpen, tc, data->usegradients, rp, ni, bc, data->dark, bbt, bl - 1, bbt, bl - 1, wh - bb);
        if (bb > 1) ShadeLine(dpen, tc, data->usegradients, rp, ni, bc, data->dark, wh - bb, bl - 1, wh - bb, ww - br, wh - bb);
        if (br > 1) ShadeLine(dpen, tc, data->usegradients, rp, ni, bc, data->dark, bbt , ww - br, bbt , ww - br, wh - bb);
        if (bl > 2) ShadeLine(lpen, tc, data->usegradients, rp, ni, bc, data->light, bbt, 1, bbt, 1, wh - 2);
        if (bl > 3) {
            if (bb > 1) ShadeLine(mpen, tc, data->usegradients, rp, ni, bc, data->middle, bbt, bl - 2, bbt, bl - 2, wh - bb + 1);
            else ShadeLine(mpen, tc, data->usegradients, rp, ni, bc, data->middle, bbt, bl - 2, bbt, bl - 2, wh - bb);
        }
        if (br > 2) ShadeLine(mpen, tc, data->usegradients, rp, ni, bc, data->middle, bbt, ww - 2, bbt, ww - 2, wh - 2);
        if (bb > 2) ShadeLine(mpen, tc, data->usegradients, rp, ni, bc, data->middle, wh - 2, 1, wh - 2, ww - 2, wh - 2);
        if (bb > 3) {
            if ((bl > 0) && (br > 0)) ShadeLine(lpen, tc, data->usegradients, rp, ni, bc, data->light, wh - bb + 1, bl, wh - bb + 1, ww - br, wh - bb + 1);
        }
        if (br > 3) {
            if (bb > 1) ShadeLine(lpen, tc, data->usegradients, rp, ni, bc, data->light, bbt, ww - br + 1, bbt, ww - br + 1, wh - bb + 1);
        }
    }
    return TRUE;
}

/**************************************************************************************************/
static IPTR windecor_layout_bordergadgets(Class *cl, Object *obj, struct wdpLayoutBorderGadgets *msg)
{
    struct windecor_data   *data = INST_DATA(cl, obj);
    struct Window          *window = msg->wdp_Window;
    struct Gadget          *gadget = msg->wdp_Gadgets;
    struct Gadget          *draggadget = NULL;
    struct WindowData      *wd = (struct WindowData *) msg->wdp_UserBuffer;
    ULONG                   eb = msg->wdp_ExtraButtons;

    BOOL                    hasdepth;
    BOOL                    haszoom;
    BOOL                    hasclose;
    BOOL                    hasdrag;
    BOOL                    hastitle;
    BOOL                    hassize;
    BOOL                    borderless;
    LONG                    width;
    LONG                    rightborder = 0;
    LONG                    bottomborder = 0;

    DoSuperMethodA(cl, obj, (Msg)msg);

    hastitle = window->Title != NULL ? TRUE : FALSE;
    hasclose = (window->Flags & WFLG_CLOSEGADGET) ? TRUE : FALSE;
    hassize = (window->Flags & WFLG_SIZEGADGET) ? TRUE : FALSE;
    hasdepth = (window->Flags & WFLG_DEPTHGADGET) ? TRUE : FALSE;
    hasdrag = (window->Flags & WFLG_DRAGBAR) ? TRUE : FALSE;
    borderless = (window->Flags & WFLG_BORDERLESS) ? TRUE : FALSE;
    haszoom = ((window->Flags & WFLG_HASZOOM) || ((window->Flags & WFLG_SIZEGADGET) && hasdepth)) ? TRUE : FALSE;


    if ((msg->wdp_Flags & WDF_LBG_SYSTEMGADGET) != 0)
    {
        if (gadget->GadgetType == GTYP_CUSTOMGADGET)
        {
            switch(gadget->GadgetID)
            {
                case ETI_MUI:
                    if (wd->img_mui->ok)
                    {
                        if (data->threestate) width = (wd->img_mui->w / 3); else width = (wd->img_mui->w >> 2);

                        gadget->Width = width;
                        gadget->Height = wd->img_mui->h;
                        gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;

                        if (haszoom)
                        {
                            if (data->threestate) width += (wd->img_zoom->w / 3); else width += (wd->img_zoom->w >> 2);
                        }
                        if (hasclose && data->closeright)
                        {
                            if (data->threestate) width += (wd->img_close->w / 3); else width += (wd->img_close->w >> 2);
                        }
                        if (hasdepth)
                        {
                            if (data->threestate) width += (wd->img_depth->w / 3); else width += (wd->img_depth->w >> 2);
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        else
                        {
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        gadget->Flags &= ~GFLG_RELWIDTH;
                        gadget->Flags |= GFLG_RELRIGHT;

                    }
                    break;

                case ETI_PopUp:
                    if (wd->img_popup->ok)
                    {
                        if (data->threestate) width = (wd->img_popup->w / 3); else width = (wd->img_popup->w >> 2);

                        gadget->Width = width;
                        gadget->Height = wd->img_popup->h;
                        gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;

                        if ((eb & ETG_MUI) != 0)
                        {
                            if (wd->img_mui->ok)
                            {
                                if (data->threestate) width += (wd->img_mui->w / 3); else width += (wd->img_mui->w >> 2);
                            }
                        }

                        if (haszoom)
                        {
                            if (data->threestate) width += (wd->img_zoom->w / 3); else width += (wd->img_zoom->w >> 2);
                        }

                        if (hasclose && data->closeright)
                        {
                            if (data->threestate) width += (wd->img_close->w / 3); else width += (wd->img_close->w >> 2);
                        }
                        if (hasdepth)
                        {
                            if (data->threestate) width += (wd->img_depth->w / 3); else width += (wd->img_depth->w >> 2);
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        else
                        {
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        gadget->Flags &= ~GFLG_RELWIDTH;
                        gadget->Flags |= GFLG_RELRIGHT;

                    }
                    break;

                case ETI_Snapshot:
                    if (wd->img_snapshot->ok)
                    {
                        if (data->threestate) width = (wd->img_snapshot->w / 3); else width = (wd->img_snapshot->w >> 2);

                        gadget->Width = width;
                        gadget->Height = wd->img_snapshot->h;
                        gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;

                        if ((eb & ETG_MUI) != 0)
                        {

                            if (wd->img_mui->ok)
                            {
                                if (data->threestate) width += (wd->img_mui->w / 3); else width += (wd->img_mui->w >> 2);
                            }
                        }

                        if ((eb & ETG_POPUP) != 0)
                        {
                            if (wd->img_popup->ok)
                            {
                                if (data->threestate) width += (wd->img_popup->w / 3); else width += (wd->img_popup->w >> 2);
                            }
                        }

                        if (haszoom)
                        {
                            if (data->threestate) width += (wd->img_zoom->w / 3); else width += (wd->img_zoom->w >> 2);
                        }

                        if (hasclose && data->closeright)
                        {
                            if (data->threestate) width += (wd->img_close->w / 3); else width += (wd->img_close->w >> 2);
                        }

                        if (hasdepth)
                        {
                            if (data->threestate) width += (wd->img_depth->w / 3); else width += (wd->img_depth->w >> 2);
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        else
                        {
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }

                        gadget->Flags &= ~GFLG_RELWIDTH;
                        gadget->Flags |= GFLG_RELRIGHT;

                    }
                    break;

                case ETI_Iconify:
                    if (wd->img_iconify->ok)
                    {
                        if (data->threestate) width = (wd->img_iconify->w / 3); else width = (wd->img_iconify->w >> 2);

                        gadget->Width = width;
                        gadget->Height = wd->img_iconify->h;
                        gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;

                        if ((eb & ETG_MUI) != 0)
                        {
                            if (wd->img_mui->ok)
                            {
                                if (data->threestate) width += (wd->img_mui->w / 3); else width += (wd->img_mui->w >> 2);
                            }
                        }

                        if ((eb & ETG_POPUP) != 0)
                        {
                            if (wd->img_popup->ok)
                            {
                                if (data->threestate) width += (wd->img_popup->w / 3); else width += (wd->img_popup->w >> 2);
                            }
                        }

                        if ((eb & ETG_SNAPSHOT) != 0)
                        {
                            if (wd->img_snapshot->ok)
                            {
                                if (data->threestate) width += (wd->img_snapshot->w / 3); else width += (wd->img_snapshot->w >> 2);
                            }
                        }

                        if (haszoom)
                        {
                            if (data->threestate) width += (wd->img_zoom->w / 3); else width += (wd->img_zoom->w >> 2);
                        }

                        if (hasclose && data->closeright)
                        {
                            if (data->threestate) width += (wd->img_close->w / 3); else width += (wd->img_close->w >> 2);
                        }
                        if (hasdepth)
                        {
                            if (data->threestate) width += (wd->img_depth->w / 3); else width += (wd->img_depth->w >> 2);
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        else
                        {
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        gadget->Flags &= ~GFLG_RELWIDTH;
                        gadget->Flags |= GFLG_RELRIGHT;

                    }
                    break;

                case ETI_Lock:
                    if (wd->img_lock->ok)
                    {
                        if (data->threestate) width = (wd->img_lock->w / 3); else width = (wd->img_lock->w >> 2);

                        gadget->Width = width;
                        gadget->Height = wd->img_lock->h;
                        gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;

                        if ((eb & ETG_MUI) != 0)
                        {
                            if (wd->img_mui->ok)
                            {
                                if (data->threestate) width += (wd->img_mui->w / 3); else width += (wd->img_mui->w >> 2);
                            }
                        }

                        if ((eb & ETG_POPUP) != 0)
                        {
                            if (wd->img_popup->ok)
                            {
                                if (data->threestate) width += (wd->img_popup->w / 3); else width += (wd->img_popup->w >> 2);
                            }
                        }

                        if ((eb & ETG_SNAPSHOT) != 0)
                        {
                            if (wd->img_snapshot->ok)
                            {
                                if (data->threestate) width += (wd->img_snapshot->w / 3); else width += (wd->img_snapshot->w >> 2);
                            }
                        }

                        if ((eb & ETG_ICONIFY) != 0)
                        {
                            if (wd->img_iconify->ok)
                            {
                                if (data->threestate) width += (wd->img_iconify->w / 3); else width += (wd->img_iconify->w >> 2);
                            }
                        }

                        if (haszoom)
                        {
                            if (data->threestate) width += (wd->img_zoom->w / 3); else width += (wd->img_zoom->w >> 2);
                        }

                        if (hasclose && data->closeright)
                        {
                            if (data->threestate) width += (wd->img_close->w / 3); else width += (wd->img_close->w >> 2);
                        }
                        if (hasdepth)
                        {
                            if (data->threestate) width += (wd->img_depth->w / 3); else width += (wd->img_depth->w >> 2);
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        else
                        {
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        gadget->Flags &= ~GFLG_RELWIDTH;
                        gadget->Flags |= GFLG_RELRIGHT;

                    }
                    break;

            }
        }
        else
        {
            switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
            {
                case GTYP_CLOSE:
                    if (data->threestate) width = (wd->img_close->w / 3); else width = (wd->img_close->w >> 2);
                    gadget->Width = width;
                    wd->closewidth = width;
                    gadget->Height = wd->img_close->h;
                    if (data->closeright)
                    {
                        gadget->Flags &= ~GFLG_RELWIDTH;
                        gadget->Flags |= GFLG_RELRIGHT;
                        gadget->LeftEdge = -data->BarPostGadget_s - width;
                    }
                    else
                    {
                        gadget->LeftEdge = data->BarPreGadget_s;
                    }
                    gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;
                    break;
    
                case GTYP_WDEPTH:
                    if (data->threestate) width = (wd->img_depth->w / 3); else width = (wd->img_depth->w >> 2);
                    gadget->Width = width;
                    wd->depthwidth = width;
                    gadget->Height = wd->img_depth->h;
                    if (hasclose && data->closeright)
                    {
                        if (data->threestate) width += (wd->img_close->w / 3); else width += (wd->img_close->w >> 2);
                    }
                    gadget->LeftEdge = -data->BarPostGadget_s - width;
                    gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;
                    gadget->Flags &= ~GFLG_RELWIDTH;
                    gadget->Flags |= GFLG_RELRIGHT;
                    break;
    
                case GTYP_WZOOM:
                    if (data->threestate) width = (wd->img_zoom->w / 3); else width = (wd->img_zoom->w >> 2);
                    gadget->Width = width;
                    wd->zoomwidth = width;
                    gadget->Height = wd->img_zoom->h;
                    gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;
                    if (hasclose && data->closeright)
                    {
                        if (data->threestate) width += (wd->img_close->w / 3); else width += (wd->img_close->w >> 2);
                    }
                    if (hasdepth)
                    {
                        if (data->threestate) width += (wd->img_depth->w / 3); else width += (wd->img_depth->w >> 2);
                        gadget->LeftEdge = -data->BarPostGadget_s - width;
                    }
                    else
                    {
                        gadget->LeftEdge = -data->BarPostGadget_s - width;
                    }
                    gadget->Flags &= ~GFLG_RELWIDTH;
                    gadget->Flags |= GFLG_RELRIGHT;
    
                    break;
    
                case GTYP_SIZING:
                    rightborder = data->rightbordergads;
                    if ((gadget->Flags & WFLG_SIZEBBOTTOM) != 0) bottomborder = data->bottombordergads;
                    break;
    
                case GTYP_WDRAGGING:
                    break;
    
            }
        }
        return TRUE;
    }

    int sysrgad = -data->BarPostGadget_s - 1;

    if (data->closeright && hasclose) sysrgad -= wd->closewidth;
    if (hasdepth) sysrgad -= wd->depthwidth;
    if (haszoom) sysrgad -= wd->zoomwidth;
    while(gadget)
    {
        if ((gadget->GadgetType & GTYP_SYSTYPEMASK) == 0)
        {
        switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
        {
                case GTYP_WDRAGGING:
                   break;

                default:
                if ((gadget->Flags & GFLG_EXTENDED) != 0)
                {
                    if ((((struct ExtGadget *) gadget)->MoreFlags & GMORE_BOOPSIGADGET) != 0)
                    {
                        ULONG   rtsm = 0;
                        get((Object *) gadget, GA_RightBorder, &rtsm);
                        if (rtsm)
                        {
                            if (get((Object *) gadget, PGA_Top, &rtsm))
                            {
                                SetAttrs((Object *) gadget, GA_RelRight, - data->rightbordergads + ((data->rightbordergads - (wd->img_verticalcontainer->w >> 1) + 1) >> 1) + 1, GA_Width, wd->img_verticalcontainer->w >> 1, TAG_DONE);
                            }
                            else
                            {
                                get((Object *) gadget, GA_Width, &rtsm);
                                SetAttrs((Object *) gadget, GA_RelRight, - data->rightbordergads + ((data->rightbordergads - rtsm + 1) >> 1) + 1, TAG_DONE);
                            }
                        }
                        else
                        {
                            get((Object *) gadget, GA_BottomBorder, &rtsm);
                            if (rtsm)
                            {
                                if (get((Object *) gadget, PGA_Top, &rtsm))
                                {
                                    SetAttrs((Object *) gadget, GA_RelBottom, - data->bottombordergads + ((data->bottombordergads - (wd->img_horizontalcontainer->h >> 1) + 1)  >> 1) +1, GA_Height, (wd->img_horizontalcontainer->h >> 1), TAG_DONE);
                                }
                                else
                                {
                                    get((Object *) gadget, GA_Height, &rtsm);
                                    SetAttrs((Object *) gadget, GA_RelBottom, - data->bottombordergads + ((data->bottombordergads - rtsm + 1) >> 1) + 1, TAG_DONE);
                                }
                            }
                        }
                    }
                }
                break;
                }
        }
        if (msg->wdp_Flags & WDF_LBG_MULTIPLE)
        {
            gadget = gadget->NextGadget;
        }
        else
        {
            gadget = NULL;
        }
   }
    gadget = msg->wdp_Gadgets;

    while(gadget)
    {
        if ((gadget->GadgetType & GTYP_SYSTYPEMASK) == 0)
        {
            if ((gadget->Activation & GACT_TOPBORDER) != 0)
            {
                if ((gadget->Flags & GFLG_RELRIGHT) != 0)
                {
                    gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;
                    sysrgad -= gadget->Width;
                    gadget->LeftEdge = sysrgad;
                }
            }
        }
        gadget = gadget->NextGadget;
    }

    gadget = msg->wdp_Gadgets;

    if ((msg->wdp_Flags & WDF_LBG_SYSTEMGADGET) != 0) while(gadget)
    {
        switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
        {
            case GTYP_WDRAGGING:
                gadget->Width = sysrgad;
                if (hasclose && !data->closeright)
                {
                    gadget->Width -= data->BarPreGadget_s;
                    if (data->threestate) gadget->Width -= (wd->img_close->w / 3); else gadget->Width -= (wd->img_close->w >> 2);
                }
                break;
        }
        gadget = gadget->NextGadget;
    }

    if (draggadget)
    {
    }

    return TRUE;
}

/**************************************************************************************************/
static IPTR windecor_draw_borderpropback(Class *cl, Object *obj, struct wdpDrawBorderPropBack *msg)
{
    /* Simply return, we need to render the back in the knob method    */
    /* because we want to use irregular (alpha images) for the sliders */
    return TRUE;
}

/**************************************************************************************************/
static IPTR windecor_draw_borderpropknob(Class *cl, Object *obj, struct wdpDrawBorderPropKnob *msg)
{
    struct windecor_data   *data = INST_DATA(cl, obj);
    struct Window          *window = msg->wdp_Window;
    struct RastPort        *winrp = msg->wdp_RPort;
    struct WindowData      *wd = (struct WindowData *) msg->wdp_UserBuffer;

    struct RastPort        *rp;
    struct Gadget          *gadget = msg->wdp_Gadget;
    struct Rectangle       *r;
    struct PropInfo        *pi = ((struct PropInfo *)gadget->SpecialInfo);
    struct NewImage        *ni = NULL;
    BOOL                    hit = (msg->wdp_Flags & WDF_DBPK_HIT) ? TRUE : FALSE;
    ULONG                   y, x, bx0, bx1, by0, by1;
    int                     size, is, pos;
    ULONG                   bc, color, s_col, e_col, arc;
    LONG                    pen = -1;
    struct BitMap          *cachedgadgetbitmap = NULL;

    if (!(pi->Flags & PROPNEWLOOK) || (gadget->Activation && (GACT_RIGHTBORDER | GACT_BOTTOMBORDER) == 0))
    {
        return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    /* Detect change in gadget dimensions (which needs to trigger redraw) */
    if ((pi->Flags & FREEVERT) != 0)
    {
        if (HasPropGadgetChanged(&wd->vert, msg->wdp_PropRect, msg->wdp_RenderRect, msg->wdp_Window))
        {
            if (wd->vert.bm != NULL)
                FreeBitMap(wd->vert.bm);
            wd->vert.bm = NULL;
        }
        cachedgadgetbitmap = wd->vert.bm;
    }
    else if ((pi->Flags & FREEHORIZ) != 0)
    {
        if (HasPropGadgetChanged(&wd->horiz, msg->wdp_PropRect, msg->wdp_RenderRect, msg->wdp_Window))
        {
            if (wd->horiz.bm != NULL)
                FreeBitMap(wd->horiz.bm);
            wd->horiz.bm = NULL;
        }
        cachedgadgetbitmap = wd->horiz.bm;

    }
    else
        return TRUE; /* Return TRUE - after all this is not an error */
    
    /* Reuse the bitmap if that is possible */
    if (cachedgadgetbitmap != NULL)
    {
        /* Final blitting of gadget bitmap to window rast port */
        BltBitMapRastPort(cachedgadgetbitmap, 0, 0, winrp, msg->wdp_PropRect->MinX, 
            msg->wdp_PropRect->MinY, 
            msg->wdp_PropRect->MaxX - msg->wdp_PropRect->MinX + 1, 
            msg->wdp_PropRect->MaxY - msg->wdp_PropRect->MinY + 1, 0xc0);
        return TRUE;
    }

    /* Regenerate the bitmap */
    r = msg->wdp_PropRect;

    bx0 = r->MinX;
    by0 = r->MinY;
    bx1 = r->MaxX;
    by1 = r->MaxY;

    rp = CreateRastPort();
    if (rp)
    {
    	struct Rectangle cliprect = {0, 0, bx1 - bx0, by1 - by0};
    	struct TagItem rptags[] =
        {
            {RPTAG_ClipRectangle, (IPTR)&cliprect},
            {TAG_DONE	    	    	    	 }
        };

        rp->BitMap = AllocBitMap(bx1 - bx0 + 1, by1 - by0 + 1, 1, 0, window->WScreen->RastPort.BitMap);
        if (rp->BitMap == NULL)
        {
            FreeRastPort(rp);
            return FALSE;
        }

    	SetRPAttrsA(rp, rptags);
    }
    else
        return FALSE;

    color = 0x00cccccc;

    if (wd->img_border_normal->ok) ni = wd->img_border_normal;

    if (ni == NULL) data->usegradients = TRUE;

    if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX))
    {
        s_col = data->a_col_s;
        e_col = data->a_col_e;
        arc = data->a_arc;
        bc = data->b_col_a;
        pen = wd->ActivePen;
    }
    else
    {
        s_col = data->d_col_s;
        e_col = data->d_col_e;
        arc = data->d_arc;
        bc = data->b_col_d;
        if (!data->usegradients)
        {
            if (wd->img_border_deactivated->ok) ni = wd->img_border_deactivated;
        }
        pen = wd->DeactivePen;
    }

    /* Drawing background - this solves WDM_DRAW_BORDERPROPBACK without need of
       reading from window when drawing container and knob */
    if (data->usegradients)
    {
        FillPixelArrayGradientDelta(pen, wd->truecolor, rp, 0, 0, window->Width-1, window->Height-1,  0, 0, bx1 - bx0 + 1, by1 - by0 + 1, s_col, e_col, arc, 0, 0);
    }
    else
    {
        if (ni->ok != 0)
        {
            ULONG   color = 0x00cccccc;

            DrawTileToRPRoot(rp, ni, color, 0, 0, bx0, by0, bx1 - bx0 + 1, by1 - by0 + 1);
        }
    }

    /* Drawing knob container */
    r = msg->wdp_PropRect;

    bx0 = 0;
    by0 = 0;
    bx1 = r->MaxX - r->MinX;
    by1 = r->MaxY - r->MinY;

    if ((pi->Flags & FREEVERT) != 0)
    {

        is = wd->img_verticalcontainer->w >> 1;
        if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX)) pos = 0; else pos = is;
        y = by0;
        size = by1 - by0 - data->ContainerTop_s - data->ContainerBottom_s + 1;
        y = WriteTiledImageVertical(rp, wd->img_verticalcontainer, pos, data->ContainerTop_o, is, data->ContainerTop_s, bx0, y, is, data->ContainerTop_s);
        if (size > 0) y = WriteTiledImageVertical(rp, wd->img_verticalcontainer, pos, data->ContainerVertTile_o, is, data->ContainerVertTile_s, bx0, y, is, size);

        y = WriteTiledImageVertical(rp, wd->img_verticalcontainer, pos, data->ContainerBottom_o, is, data->ContainerBottom_s, bx0, y, is, data->ContainerBottom_s);

    }
    else if ((pi->Flags & FREEHORIZ) != 0)
    {

        is = wd->img_horizontalcontainer->h >> 1;
        if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX)) pos = 0; else pos = is;
        x = bx0;
        size = bx1 - bx0 - data->ContainerLeft_s - data->ContainerRight_s + 1;
        x = WriteTiledImageHorizontal(rp, wd->img_horizontalcontainer, data->ContainerLeft_o, pos, data->ContainerLeft_s, is, x, by0, data->ContainerLeft_s, is);
        if (size > 0) x = WriteTiledImageHorizontal(rp, wd->img_horizontalcontainer, data->ContainerHorTile_o, pos, data->ContainerHorTile_s, is, x, by0, size, is);
        x = WriteTiledImageHorizontal(rp, wd->img_horizontalcontainer, data->ContainerRight_o, pos, data->ContainerRight_s, is, x, by0, data->ContainerRight_s, is);
    }


    /* Drawing knob */
    bx0 = msg->wdp_PropRect->MinX;
    by0 = msg->wdp_PropRect->MinY;
    bx1 = msg->wdp_PropRect->MaxX;
    by1 = msg->wdp_PropRect->MaxY;

    r = msg->wdp_RenderRect;
    if ((pi->Flags & FREEVERT) != 0)
    {
        is = wd->img_verticalknob->w / 3;
        if (hit) pos = is; else if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX)) pos = 0; else pos = is * 2;
        y = r->MinY - by0;
        size = r->MaxY - r->MinY - data->KnobTop_s - data->KnobBottom_s + 1;

        y = WriteTiledImageVertical(rp, wd->img_verticalknob, pos, data->KnobTop_o, is, data->KnobTop_s, r->MinX - bx0, y, is, data->KnobTop_s);
        if (size > 0)
        {
            if (size > data->KnobVertGripper_s)
            {
                size = size - data->KnobVertGripper_s;
                int size_bak = size;
                size = size / 2;
                if (size > 0) y = WriteTiledImageVertical(rp, wd->img_verticalknob, pos, data->KnobTileTop_o, is, data->KnobTileTop_s, r->MinX - bx0, y, is, size);
                y = WriteTiledImageVertical(rp, wd->img_verticalknob, pos, data->KnobVertGripper_o, is, data->KnobVertGripper_s, r->MinX - bx0, y, is, data->KnobVertGripper_s);
                size = size_bak - size;
                if (size > 0) y = WriteTiledImageVertical(rp, wd->img_verticalknob, pos, data->KnobTileBottom_o, is, data->KnobTileBottom_s, r->MinX - bx0, y, is, size);
            }
            else
            {
                y = WriteTiledImageVertical(rp, wd->img_verticalknob, pos, data->KnobTileTop_o, is, data->KnobTileTop_s, r->MinX - bx0, y, is, size);
            }
        }
        y = WriteTiledImageVertical(rp, wd->img_verticalknob, pos, data->KnobBottom_o, is, data->KnobBottom_s, r->MinX - bx0, y, is, data->KnobBottom_s);
    }
    else if ((pi->Flags & FREEHORIZ) != 0)
    {

        is = wd->img_horizontalknob->h / 3;
        if (hit) pos = is; else if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX)) pos = 0; else pos = is * 2;
        x = r->MinX - bx0;
        size = r->MaxX - r->MinX - data->KnobLeft_s - data->KnobRight_s + 1;
        x = WriteTiledImageHorizontal(rp, wd->img_horizontalknob, data->KnobLeft_o, pos, data->KnobLeft_s, is, x, r->MinY - by0, data->KnobLeft_s, is);

        if (size > 0)
        {
            if (size > data->KnobHorGripper_s)
            {
                size = size - data->KnobHorGripper_s;
                int size_bak = size;
                size = size / 2;
                if (size > 0) x = WriteTiledImageHorizontal(rp, wd->img_horizontalknob, data->KnobTileLeft_o, pos, data->KnobTileLeft_s, is, x, r->MinY - by0, size, is);
                x = WriteTiledImageHorizontal(rp, wd->img_horizontalknob, data->KnobHorGripper_o, pos, data->KnobHorGripper_s, is, x, r->MinY - by0, data->KnobHorGripper_s, is);
                size = size_bak - size;
                if (size > 0) x = WriteTiledImageHorizontal(rp, wd->img_horizontalknob, data->KnobTileRight_o, pos, data->KnobTileRight_s, is, x, r->MinY - by0, size, is);
            }
            else
            {
                x = WriteTiledImageHorizontal(rp, wd->img_horizontalknob, data->KnobTileRight_o, pos, data->KnobTileRight_s, is, x, r->MinY - by0, size, is);
            }
        }
        x = WriteTiledImageHorizontal(rp, wd->img_horizontalknob, data->KnobRight_o, pos, data->KnobRight_s, is, x, r->MinY - by0, data->KnobRight_s, is);
    }

    /* Final blitting of gadget bitmap to window rast port */
    BltBitMapRastPort(rp->BitMap, 0, 0, winrp, msg->wdp_PropRect->MinX, 
        msg->wdp_PropRect->MinY, 
        msg->wdp_PropRect->MaxX - msg->wdp_PropRect->MinX + 1, 
        msg->wdp_PropRect->MaxY - msg->wdp_PropRect->MinY + 1, 0xc0);

    /* Cache the actual bitmap */
    if ((pi->Flags & FREEVERT) != 0)
        CachePropGadget(&wd->vert, msg->wdp_PropRect, msg->wdp_RenderRect, msg->wdp_Window, rp->BitMap);
    else if ((pi->Flags & FREEHORIZ) != 0)
        CachePropGadget(&wd->horiz, msg->wdp_PropRect, msg->wdp_RenderRect, msg->wdp_Window, rp->BitMap);

    FreeRastPort(rp);

    return TRUE;
}

/**************************************************************************************************/
static IPTR windecor_getdefsizes(Class *cl, Object *obj, struct wdpGetDefSizeSysImage *msg)
{
    struct windecor_data   *data = INST_DATA(cl, obj);
    struct NewImage        *n = NULL;
    WORD                    w = 0, h = 0;
    BOOL                    isset = FALSE;
    switch(msg->wdp_Which)
    {
        case SIZEIMAGE:
            n = NULL;
            w = data->rightbordergads;
            h = data->bottombordergads;
            isset = TRUE;
            break;

        case CLOSEIMAGE:
            n = data->img_close;
            isset = TRUE;
            break;

        case MUIIMAGE:
            n = data->img_mui;
            isset = TRUE;
            break;

        case POPUPIMAGE:
            n = data->img_popup;
            isset = TRUE;
            break;

        case SNAPSHOTIMAGE:
            n = data->img_snapshot;
            isset = TRUE;
            break;

        case ICONIFYIMAGE:
            n = data->img_iconify;
            isset = TRUE;
            break;

        case LOCKIMAGE:
            n = data->img_lock;
            isset = TRUE;
            break;

        case UPIMAGE:
            n = NULL;
            w = data->rightbordergads;
            h = data->img_up->h + data->updownaddy;
            isset = TRUE;
            break;

        case DOWNIMAGE:
            n = NULL;
            w = data->rightbordergads;
            h = data->img_down->h + data->updownaddy;
            isset = TRUE;
            break;

        case LEFTIMAGE:
            n = NULL;
            if (data->threestate) w = (data->img_left->w / 3); else w = (data->img_left->w >> 2);
            w += data->leftrightaddx;
            h = data->bottombordergads;
            isset = TRUE;
            break;

        case RIGHTIMAGE:
            n = NULL;
            if (data->threestate) w = (data->img_right->w / 3); else w = (data->img_right->w >> 2);
            w += data->leftrightaddx;
            h = data->bottombordergads;
            isset = TRUE;
            break;

        case DEPTHIMAGE:
            n = data->img_depth;
            isset = TRUE;
            break;

        case ZOOMIMAGE:
            n = data->img_zoom;
            isset = TRUE;
            break;

        default:
            return FALSE;
    }

    if (!isset) return DoSuperMethodA(cl, obj, (Msg) msg);

    if (n == NULL)
    {
        *msg->wdp_Width = w;
        *msg->wdp_Height = h;
    }
    else
    {
        if (n->ok) {
            if (data->threestate)
            {
                *msg->wdp_Width = (n->w / 3);
                *msg->wdp_Height = n->h;
            }
            else
            {
                *msg->wdp_Width = (n->w >> 2);
                *msg->wdp_Height = n->h;
            }
        } else return DoSuperMethodA(cl, obj, (Msg) msg);
    }

    return TRUE;
}

/**************************************************************************************************/
static IPTR windecor_windowshape(Class *cl, Object *obj, struct wdpWindowShape *msg)
{
    struct windecor_data   *data = INST_DATA(cl, obj);
    struct WindowData      *wd = (struct WindowData *) msg->wdp_UserBuffer;
    struct Window          *window = msg->wdp_Window;

    if (data->barmasking)
    {
        struct  NewLUT8ImageContainer *shape;
        IPTR    back = 0;
        shape = (struct  NewLUT8ImageContainer *)NewLUT8ImageContainer(window->Width, window->BorderTop);
        if (shape)
        {
            if (window->BorderTop == data->winbarheight) DrawShapePartialTitleBar(wd, (struct NewLUT8Image *)shape, data, window, data->txt_align, 0, window->Width);
            back =(IPTR) RegionFromLUT8Image(msg->wdp_Width, msg->wdp_Height, (struct NewLUT8Image *)shape);

            DisposeLUT8ImageContainer((struct NewLUT8Image *)shape);
            return back;

        }

    }

    if (!data->rounded) return (IPTR) NULL;

    struct  Region *newshape;

    int x2 = msg->wdp_Width-1;
    int y2 = msg->wdp_Height-1;
    
    if ((newshape = NewRegion()))
    {
        struct Rectangle rect;
        BOOL success = TRUE;

        rect.MinX = 9;
        rect.MinY = 0;
        rect.MaxX = x2 - 9;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);

        rect.MinX = 6;
        rect.MinY = 1;
        rect.MaxX = x2 - 6;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);

        rect.MinX = 4;
        rect.MinY = 2;
        rect.MaxX = x2 - 4;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);

        rect.MinX = 3;
        rect.MinY = 3;
        rect.MaxX = x2 - 3;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);

        rect.MinX = 2;
        rect.MinY = 4;
        rect.MaxX = x2 - 2;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);

        rect.MinX = 1;
        rect.MinY = 6;
        rect.MaxX = x2 - 1;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);

        rect.MinX = 0;
        rect.MinY = 9;
        rect.MaxX = x2;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);
    }
    return (IPTR) newshape;
}

/**************************************************************************************************/
static IPTR windecor_initwindow(Class *cl, Object *obj, struct wdpInitWindow *msg)
{
    struct WindowData *wd = (struct WindowData *)msg->wdp_UserBuffer;
    struct ScreenData *sd = (struct ScreenData *)msg->wdp_ScreenUserBuffer;

    wd->truecolor = msg->wdp_TrueColor;

    wd->ActivePen = sd->ActivePen;
    wd->DeactivePen = sd->DeactivePen;
    
    wd->vert.bm = NULL;
    wd->horiz.bm = NULL;

    SETIMAGE_WIN(size);
    SETIMAGE_WIN(close);
    SETIMAGE_WIN(depth);
    SETIMAGE_WIN(zoom);
    SETIMAGE_WIN(up);
    SETIMAGE_WIN(down);
    SETIMAGE_WIN(left);
    SETIMAGE_WIN(right);
    SETIMAGE_WIN(mui);
    SETIMAGE_WIN(popup);
    SETIMAGE_WIN(snapshot);
    SETIMAGE_WIN(iconify);
    SETIMAGE_WIN(lock);
    SETIMAGE_WIN(winbar_normal);
    SETIMAGE_WIN(border_normal);
    SETIMAGE_WIN(border_deactivated);
    SETIMAGE_WIN(verticalcontainer);
    SETIMAGE_WIN(verticalknob);
    SETIMAGE_WIN(horizontalcontainer);
    SETIMAGE_WIN(horizontalknob);

    return TRUE;
}

/**************************************************************************************************/
static IPTR windecor_exitwindow(Class *cl, Object *obj, struct wdpExitWindow *msg)
{
    struct WindowData *wd = (struct WindowData *) msg->wdp_UserBuffer;

    if (wd->rp)
    {
        if (wd->rp->BitMap) FreeBitMap(wd->rp->BitMap);
        FreeRastPort(wd->rp);
    }
    
    if (wd->vert.bm) FreeBitMap(wd->vert.bm);
    if (wd->horiz.bm) FreeBitMap(wd->horiz.bm);

    return TRUE;
}

/**************************************************************************************************/
static IPTR windecor_dispatcher(struct IClass *cl, Object *obj, Msg msg)
{
    IPTR retval;

    switch(msg->MethodID)
    {
        case OM_NEW:
            retval = windecor_new(cl, obj, (struct opSet *)msg);
            break;

        case OM_DISPOSE:
            retval = windecor_dispose(cl, obj, (struct opSet *)msg);
            break;

        case OM_GET:
            retval = windecor_get(cl, obj, (struct opGet *)msg);
            break;

        case WDM_DRAW_SYSIMAGE:
            retval = windecor_draw_sysimage(cl, obj, (struct wdpDrawSysImage *)msg);
            break;

        case WDM_DRAW_WINBORDER:
            retval = windecor_draw_winborder(cl, obj, (struct wdpDrawWinBorder *)msg);
            break;

        case WDM_LAYOUT_BORDERGADGETS:
            retval = windecor_layout_bordergadgets(cl, obj, (struct wdpLayoutBorderGadgets *)msg);
            break;

        case WDM_DRAW_BORDERPROPBACK:
            retval = windecor_draw_borderpropback(cl, obj, (struct wdpDrawBorderPropBack *)msg);
            break;

        case WDM_DRAW_BORDERPROPKNOB:
            retval = windecor_draw_borderpropknob(cl, obj, (struct wdpDrawBorderPropKnob *)msg);
            break;

        case WDM_GETDEFSIZE_SYSIMAGE:
            retval = windecor_getdefsizes(cl, obj, (struct wdpGetDefSizeSysImage *) msg);
            break;

        case WDM_WINDOWSHAPE:
            retval = windecor_windowshape(cl, obj, (struct wdpWindowShape *) msg);
            break;

        case WDM_INITWINDOW:
            retval = windecor_initwindow(cl, obj, (struct wdpInitWindow *) msg);
            break;

        case WDM_EXITWINDOW:
            retval = windecor_exitwindow(cl, obj, (struct wdpExitWindow *) msg);
            break;

        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}

struct IClass * MakeWindowDecorClass()
{
    struct IClass * cl = MakeClass(NULL, WINDECORCLASS, NULL, sizeof(struct windecor_data), 0);
    if (cl)
    {
        cl->cl_Dispatcher.h_Entry    = HookEntry;
        cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC)windecor_dispatcher;
    }
    
    return cl;
}
