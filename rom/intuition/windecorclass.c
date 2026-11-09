/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    Copyright (C) 2001-2003, The MorphOS Development Team. All Rights Reserved.
*/

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/windecorclass.h>
#include <intuition/cghooks.h>
#include <intuition/icclass.h>
#include <intuition/extensions.h>

#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <clib/macros.h>

#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <proto/alib.h>

#include "intuition_intern.h"
#include "gadgets.h"
#include "decorsupport.h"

/**************************************************************************************************/

#ifdef __AROS__
#define USE_AROS_DEFSIZE 1
#else
#define USE_AROS_DEFSIZE 0
#endif

#define DEFSIZE_WIDTH  14
#define DEFSIZE_HEIGHT 14

#define DRI(dri) ((struct DrawInfo *)(dri))

/**************************************************************************************************/

/**************************************************************************************************/

IPTR WinDecorClass__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    return decor_om_new(cl, obj, msg, WDA_UserBuffer);
}

/**************************************************************************************************/

IPTR WinDecorClass__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    return decor_om_get(cl, obj, msg, WDA_UserBuffer, WDA_TrueColorOnly);
}
    

/**************************************************************************************************/

IPTR WinDecorClass__WDM_GETDEFSIZE_SYSIMAGE(Class *cl, Object *obj, struct wdpGetDefSizeSysImage *msg)
{
    ULONG def_low_width = DEFSIZE_WIDTH, def_low_height = DEFSIZE_HEIGHT;
    ULONG def_med_width = DEFSIZE_WIDTH, def_med_height = DEFSIZE_HEIGHT;
    ULONG def_high_width = DEFSIZE_WIDTH, def_high_height = DEFSIZE_HEIGHT;

    ULONG refheight = msg->wdp_ReferenceFont->tf_YSize;
    ULONG refwidth = refheight;

    switch(msg->wdp_Which)
    {
    case LEFTIMAGE:
    case RIGHTIMAGE:
    #if USE_AROS_DEFSIZE
            def_low_width = def_med_width = def_high_width = DEFSIZE_WIDTH;
            def_low_height = def_med_height = def_high_height = DEFSIZE_HEIGHT;
    #else
            def_low_width = 16;
            def_med_width = 16;
            def_high_width = 23;
            def_low_height = 11;
            def_med_height = 10;
            def_high_height = 22;
    #endif
            break;

    case UPIMAGE:
    case DOWNIMAGE:
    #if USE_AROS_DEFSIZE
            def_low_width = def_med_width = def_high_width = DEFSIZE_WIDTH;
            def_low_height = def_med_height = def_high_height = DEFSIZE_HEIGHT;
    #else
            def_low_width = 13;
            def_med_width = 18;
            def_high_width = 23;
            def_low_height = 11;
            def_med_height = 11;
            def_high_height = 22;
    #endif
            break;

    case DEPTHIMAGE:
    case ZOOMIMAGE:
    case ICONIFYIMAGE:
    case LOCKIMAGE:
    case MUIIMAGE:
    case POPUPIMAGE:
    case SNAPSHOTIMAGE:
    case JUMPIMAGE:
    #if USE_AROS_DEFSIZE
            def_low_width = def_med_width = def_high_width = DEFSIZE_WIDTH;
            def_low_height = def_med_height = def_high_height = DEFSIZE_HEIGHT;
    #else
            def_low_width = 18;
            def_med_width = 24;
            def_high_width = 24;
    #endif
            break;

    case SDEPTHIMAGE:
    #if USE_AROS_DEFSIZE
            def_low_width = def_med_width = def_high_width = DEFSIZE_WIDTH;
            def_low_height = def_med_height = def_high_height = DEFSIZE_HEIGHT;
    #else
            def_low_width = 17;
            def_med_width = 23;
            def_high_width = 23;
    #endif
            break;

    case CLOSEIMAGE:
    #if USE_AROS_DEFSIZE
            def_low_width = def_med_width = def_high_width = DEFSIZE_WIDTH;
            def_low_height = def_med_height = def_high_height = DEFSIZE_HEIGHT;
    #else
            def_low_width = 15;
            def_med_width = 20;
            def_high_width = 20;
    #endif
            break;

    case SIZEIMAGE:
    #if USE_AROS_DEFSIZE
            def_low_width = def_med_width = def_high_width = DEFSIZE_WIDTH;
            def_low_height = def_med_height = def_high_height = DEFSIZE_HEIGHT;
    #else
            def_low_width = 13;
            def_med_width = 18;
            def_high_width = 18;
            def_low_height = 11;
            def_med_height = 10;
            def_high_height = 10;
    #endif
            break;

    case MENUCHECK:
            def_low_width  =
                def_med_width  =
                def_high_width = refwidth / 2 + 4; // reffont->tf_XSize * 3 / 2;
            def_low_height =
                def_med_height =
                def_high_height = refheight;
            break;

    case MXIMAGE:
            def_low_width  =
                def_med_width  =
                def_high_width = (refwidth + 1) * 2; // reffont->tf_XSize * 3 - 1;
            def_low_height =
                def_med_height =
                def_high_height = refheight + 1;
            break;

    case CHECKIMAGE:
            def_low_width  = (refwidth + 3) * 2;//reffont->tf_XSize * 2;
            def_low_height = refheight + 3;
            break;

    default:
        return FALSE;
    }

    switch(msg->wdp_SysiSize)
    {
    case SYSISIZE_LOWRES:
        *msg->wdp_Width = def_low_width;
        *msg->wdp_Height = def_low_height;
        break;

    case SYSISIZE_MEDRES:
        *msg->wdp_Width = def_med_width;
        *msg->wdp_Height = def_med_height;
        break;

    case SYSISIZE_HIRES:
        default:
        *msg->wdp_Width = def_high_width;
        *msg->wdp_Height = def_high_height;
        break;
    }

    return TRUE;
}

/**************************************************************************************************/

IPTR WinDecorClass__WDM_DRAW_SYSIMAGE(Class *cl, Object *obj, struct wdpDrawSysImage *msg)
{
    struct IntuitionBase        *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase              *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct RastPort             *rp = msg->wdp_RPort;
    UWORD                       *pens = DRI(msg->wdp_Dri)->dri_Pens;
    LONG                        state = msg->wdp_State;
    LONG                        left = msg->wdp_X;
    LONG                        top = msg->wdp_Y;
    LONG                        width = msg->wdp_Width;
    LONG                        height = msg->wdp_Height;
    LONG                        right = left + width - 1;
    LONG                        bottom = top + height - 1;
    LONG                        h_spacing, v_spacing;
    UWORD                       frameh, framev;
    UWORD                       hspacing, vspacing;

    getintframesizes(IntuitionBase, &frameh, &framev);

    SetDrMd(rp, JAM1);

    switch(msg->wdp_Which)
    {
    case MUIIMAGE:
    case POPUPIMAGE:
    case ICONIFYIMAGE:
    case CLOSEIMAGE:
    case ZOOMIMAGE:
    case DEPTHIMAGE:
    case SIZEIMAGE:
    case LEFTIMAGE:
    case UPIMAGE:
    case RIGHTIMAGE:
    case DOWNIMAGE:
        break;

    default:
        return FALSE;
    }

    /* The arrow spacing depends on the size of the whole image */
    getarrowspacing(width, height, &hspacing, &vspacing);

    /* The frame only depends on the image type for the four arrows */
    renderimageframe(rp, msg->wdp_Which, state, pens, left, top, width, height, IntuitionBase);

    if ((msg->wdp_Which == MUIIMAGE) || (msg->wdp_Which == POPUPIMAGE))
    {
        /* no code yet */
        return TRUE;
    }

    left++;
    top++;
    width -= (frameh << 1);
    height -= (framev << 1);

    switch(msg->wdp_Which)
    {
    case ICONIFYIMAGE:
    case CLOSEIMAGE:
    case ZOOMIMAGE:
        right = left + width - 1;
        bottom = top + height - 1;
        break;

    default:
        right--;
        bottom--;
        break;
    }

    /* Clear background into correct color */
    SetAPen(rp, getbgpen(state, pens));
    RectFill(rp, left, top, right, bottom);

    switch(msg->wdp_Which)
    {
    case ICONIFYIMAGE:
    case ZOOMIMAGE:
    {
        WORD   h_spacing;
        WORD   v_spacing;

        h_spacing = width / 6;
        v_spacing = height / 6;

        left += h_spacing;
        right -= h_spacing;
        top += v_spacing;
        bottom -= v_spacing;

        SetAPen(rp, pens[SHADOWPEN]);
        RectFill(rp, left, top, right, bottom);

        SetAPen(rp, pens[(state == IDS_SELECTED) ? SHINEPEN :
                            (state == IDS_NORMAL) ? FILLPEN : BACKGROUNDPEN]);
        RectFill(rp, left + 1, top + 1, right - 1, bottom - 1);

        right = left + (right - left + 1) / 2;
        if (msg->wdp_Which == ICONIFYIMAGE)
            top = top + (bottom - top + 1) / 2;
        else
            bottom = top + (bottom - top + 1) / 2;

        if (right - left <  4) right = left + 4;

        SetAPen(rp, pens[SHADOWPEN]);
        RectFill(rp, left, top, right, bottom);

        left += 2;
        right -= 2;
        top += 1;
        bottom -= 1;

        SetAPen(rp, pens[(state == IDS_SELECTED) ? FILLPEN :
                            (state == IDS_NORMAL) ? SHINEPEN : BACKGROUNDPEN]);
        RectFill(rp,left, top, right, bottom);
        break;
    }

    case CLOSEIMAGE:
    {
        h_spacing = width * 4 / 10;
        v_spacing = height * 3 / 10;

        left += h_spacing;
        right -= h_spacing;
        top += v_spacing;
        bottom -= v_spacing;

        SetAPen(rp, pens[SHADOWPEN]);
        RectFill(rp, left, top, right, bottom);

        left++;
        top++;
        right--;
        bottom--;

        SetAPen(rp, pens[(state == IDS_NORMAL) ? SHINEPEN : BACKGROUNDPEN]);
        RectFill(rp, left, top, right, bottom);

        break;
    }

    case DEPTHIMAGE:
        drawdepthwindows(rp, pens, state, FALSE, left, top, width, height, IntuitionBase);
        break;

    case SIZEIMAGE:
    {
        WORD   h_spacing;
        WORD   v_spacing;
        WORD   x, y;

        h_spacing = width  / 5;
        v_spacing = height / 5;

        /* A triangle image */

        left += h_spacing;
        top  += v_spacing;

        right  = left + width  - 1 - (h_spacing * 2);
        bottom = top  + height - 1 - (v_spacing * 2);

        width  = right  - left + 1;
        height = bottom - top  + 1;

        if (state != IDS_INACTIVENORMAL)
        {
            SetAPen(rp, pens[SHINEPEN]);

            for(y = top; y <= bottom; y++)
            {
                x = left + (bottom - y) * width / height;
                RectFill(rp, x, y, right, y);
            }
        }

        SetAPen(rp, pens[SHADOWPEN]);
        /* Draw triangle border */
        Move(rp, left, bottom);
        Draw(rp, right, top);
        Draw(rp, right, bottom);
        Draw(rp, left, bottom);

        break;
    }

    default:    /* LEFTIMAGE, UPIMAGE, RIGHTIMAGE, DOWNIMAGE */
        drawarrowimage(rp, msg->wdp_Which, pens[SHADOWPEN], left + hspacing, top + vspacing,
                       width - hspacing * 2, height - vspacing * 2, IntuitionBase);
        break;
    }

    return TRUE;
}

/**************************************************************************************************/

static void findtitlearea(struct Window *win, LONG *left, LONG *right)
{
    struct Gadget *g;

    *left = 0;
    *right = win->Width - 1;

    for (g = win->FirstGadget; g; g = g->NextGadget)
    {
        if (g->Activation & GACT_TOPBORDER && g != (struct Gadget *)IW(win)->sysgads[DRAGBAR])
        {
            if (!(g->Flags & GFLG_RELRIGHT))
            {
                if (g->LeftEdge + g->Width > *left)
                    *left = g->LeftEdge + g->Width;
            }
            else
            {
                if (g->LeftEdge + win->Width - 1 - 1 < *right)
                    *right = g->LeftEdge + win->Width - 1 - 1;
            }
        }
    }
}

/**************************************************************************************************/

IPTR INTERNAL_WDM_DRAW_WINTITLE(Class *cl, Object *obj, struct wdpDrawWinBorder *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase       *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct RastPort      *rp = msg->wdp_RPort;
    struct Window        *window = msg->wdp_Window;
    UWORD                *pens = DRI(msg->wdp_Dri)->dri_Pens;
    LONG                  right, left;

    findtitlearea(window, &left, &right);

    SetDrMd(rp, JAM1);
    SetAPen(rp, pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN]);
    CheckRectFill(rp, left + 1, 1, right - 1, window->BorderTop - 2, IntuitionBase);

    if (right - left > (window->BorderLeft + window->BorderRight))
    {
        ULONG                   textlen, titlelen;
        struct TextFont         *tf;
        struct TextExtent       te;
        UWORD                   xunused, ypad;

        getintframesizes(IntuitionBase, &xunused, &ypad);

        tf = DRI(msg->wdp_Dri)->dri_Font;
        SetFont(rp, tf);

        titlelen = strlen(window->Title);
        textlen = TextFit(rp
                          , window->Title
                          , titlelen
                          , &te
                          , NULL
                          , 1
                          , right - left - (window->BorderLeft + window->BorderRight)
                          , window->BorderTop - (ypad << 1));

        if (textlen)
        {
#define WINTITLEPAD 3
            left = left + WINTITLEPAD;

            SetAPen(rp, pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLTEXTPEN : TEXTPEN]);

            Move(rp, left, ypad + tf->tf_Baseline + ((window->BorderTop - tf->tf_YSize) >> 1));
            Text(rp, window->Title, textlen);
        }
    }

    return TRUE;
}

/**************************************************************************************************/

IPTR WinDecorClass__WDM_DRAW_WINBORDER(Class *cl, Object *obj, struct wdpDrawWinBorder *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase       *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct RastPort      *rp = msg->wdp_RPort;
    struct Window        *window = msg->wdp_Window;
    UWORD                *pens = DRI(msg->wdp_Dri)->dri_Pens;
    LONG                  left, right;
    /* The window's size and border sizes do not change while it is drawn */
    WORD                  width = window->Width, height = window->Height;
    BYTE                  borderleft = window->BorderLeft, bordertop = window->BorderTop;
    BYTE                  borderright = window->BorderRight, borderbottom = window->BorderBottom;

    SetDrMd(rp, JAM1);
    SetAPen(rp, pens[SHINEPEN]);

    if (bordertop > 0)
    {
        /* Outer shine edge on top side */

        CheckRectFill(rp, 0, 0, width - 1, 0, IntuitionBase);
    }

    if (!(msg->wdp_Flags & WDF_DWB_TOP_ONLY))
    {
        if (borderleft > 0)
        {
            /* Outer shine edge on left side */

            CheckRectFill(rp, 0, 0, 0, height - 1, IntuitionBase);
        }

        if (borderright > 1)
        {
            /* Inner shine edge on right side */

            CheckRectFill(rp,
                      width - borderright, bordertop,
                          width - borderright, height - borderbottom,
                          IntuitionBase);
        }

        if (borderbottom > 1)
        {
            /* Inner shine edge on bottom side */

            CheckRectFill(rp,
                          borderleft, height - borderbottom,
                          width - borderright, height - borderbottom,
                          IntuitionBase);
        }
    }

    SetAPen(rp, pens[SHADOWPEN]);

    if (!(msg->wdp_Flags & WDF_DWB_TOP_ONLY))
    {
        if (borderright > 0)
        {
            /* Outer shadow edge on right side */

            CheckRectFill(rp, width - 1, 1,
                      width - 1, height - 1, IntuitionBase);
        }

        if (borderbottom > 0)
        {
            /* Outer shadow edge on bottom side */

            CheckRectFill(rp, 1, height - 1,
                  width - 1, height - 1, IntuitionBase);
        }

        if (borderleft > 1)
        {
            /* Inner shadow edge on left side */

            CheckRectFill(rp, borderleft - 1, bordertop - 1,
                      borderleft - 1, height - borderbottom,
                      IntuitionBase);
        }
    }

    if (bordertop > 1)
    {
        /* Inner shadow edge on top side */

        CheckRectFill(rp, borderleft - 1, bordertop - 1,
                      width - borderright, bordertop - 1,
                      IntuitionBase);
    }

    SetAPen(rp, pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN]);

    if (bordertop > 2)
    {
        /* Fill on top side */

        CheckRectFill(rp, 1, 1, width - 2, bordertop - 2, IntuitionBase);
    }

    if (!(msg->wdp_Flags & WDF_DWB_TOP_ONLY))
    {
        if (borderleft > 2)
        {
            /* Fill on left side */

            CheckRectFill(rp, 1, 1, borderleft - 2, height - 2, IntuitionBase);
        }

        if (borderright > 2)
        {
            /* Fill on right side */

            CheckRectFill(rp, width - borderright + 1, 1,
                      width - 2, height - 2, IntuitionBase);
        }

        if (borderbottom > 2)
        {
            /* Fill on bottom side */

            CheckRectFill(rp, 1, height - borderbottom + 1,
                          width - 2, height - 2, IntuitionBase);
        }
    }

    findtitlearea(window, &left, &right);

    if (left != 0)
    {
        /* Left edge of title area */

        SetAPen(rp, pens[SHINEPEN]);
        Move(rp, left, 1);
        Draw(rp, left, bordertop - 2);
    }

    if (right != width - 1)
    {
        /* Right edges of title area */

        SetAPen(rp, pens[SHADOWPEN]);
        Move(rp, right, 1);
        Draw(rp, right, bordertop - 2);
    }

    if (window->Title) INTERNAL_WDM_DRAW_WINTITLE(cl, obj, msg);

    return TRUE;
}

/**************************************************************************************************/

IPTR WinDecorClass__WDM_LAYOUT_BORDERGADGETS(Class *cl, Object *obj, struct wdpLayoutBorderGadgets *msg)
{
    struct Gadget        *gadget = msg->wdp_Gadgets;

    if (!(msg->wdp_Flags & WDF_LBG_SYSTEMGADGET)) return TRUE;

    while(gadget)
    {
        switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
        {
        case GTYP_CLOSE:
            gadget->LeftEdge = 0;
            gadget->Width = gadget->Height;
            gadget->Flags &= ~(GFLG_RELRIGHT | GFLG_RELWIDTH);
            break;

        case GTYP_WDEPTH:
            gadget->LeftEdge = -gadget->Height + 1;
            gadget->Width = gadget->Height;
            gadget->Flags &= ~GFLG_RELWIDTH;
            gadget->Flags |= GFLG_RELRIGHT;
            break;

        case GTYP_WZOOM:
            gadget->LeftEdge = -gadget->Height * 2 + 1;
            gadget->Width = gadget->Height;
            gadget->Flags &= ~GFLG_RELWIDTH;
            gadget->Flags |= GFLG_RELRIGHT;
            break;

        case GTYP_WDRAGGING:
            gadget->LeftEdge = 0;
            gadget->Width = 0;
            gadget->Flags &= ~GFLG_RELRIGHT;
            gadget->Flags |= GFLG_RELWIDTH;
            break;
        }

        if (gadget->GadgetType == GTYP_CUSTOMGADGET)
        {
            switch(gadget->GadgetID)
            {
            case ETI_Iconify:
                gadget->LeftEdge = -gadget->Height * 3 + 1;
                gadget->Width = gadget->Height;
                gadget->Flags &= ~GFLG_RELWIDTH;
                gadget->Flags |= GFLG_RELRIGHT;
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

    return TRUE;
}

/**************************************************************************************************/

IPTR WinDecorClass__WDM_DRAW_BORDERPROPBACK(Class *cl, Object *obj, struct wdpDrawBorderPropBack *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase       *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct Window        *window = msg->wdp_Window;
    struct RastPort      *rp = msg->wdp_RPort;
    struct Gadget        *gadget = msg->wdp_Gadget;
    struct Rectangle     *r = msg->wdp_RenderRect;
    struct PropInfo      *pi = ((struct PropInfo *)gadget->SpecialInfo);
    UWORD                *pens = DRI(msg->wdp_Dri)->dri_Pens;

    SetDrMd(rp, JAM2);

    if (pi->Flags & PROPNEWLOOK)
    {
        static CONST UWORD pattern[] = {0x5555,0xAAAA};

        SetAfPt(rp, pattern, 1);
        SetAPen(rp, pens[SHADOWPEN]);
        SetBPen(rp, pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN]);
        RectFill(rp, r->MinX, r->MinY, r->MaxX, r->MaxY);
        SetAfPt(rp, NULL, 0);
    }
    else
    {
        SetAPen(rp, pens[BACKGROUNDPEN]);
        RectFill(rp, r->MinX, r->MinY, r->MaxX, r->MaxY);
    }

    return TRUE;
}

/**************************************************************************************************/

IPTR WinDecorClass__WDM_DRAW_BORDERPROPKNOB(Class *cl, Object *obj, struct wdpDrawBorderPropKnob *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct GfxBase       *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct Window        *window = msg->wdp_Window;
    struct RastPort      *rp = msg->wdp_RPort;
    struct Gadget        *gadget = msg->wdp_Gadget;
    struct Rectangle     *r = msg->wdp_RenderRect;
    struct PropInfo      *pi = ((struct PropInfo *)gadget->SpecialInfo);
    UWORD                *pens = DRI(msg->wdp_Dri)->dri_Pens;

    SetDrMd(rp, JAM2);

    if (pi->Flags & PROPBORDERLESS)
    {
        SetAPen(rp, pens[SHINEPEN]);

        /* Top edge */
        RectFill(rp, r->MinX, r->MinY, r->MaxX - 1, r->MinY);

        /* Left edge */
        RectFill(rp, r->MinX, r->MinY + 1, r->MinX, r->MaxY - 1);

        SetAPen(rp, pens[SHADOWPEN]);

        /* Right edge */
        RectFill(rp, r->MaxX, r->MinY, r->MaxX, r->MaxY);

        /* Bottom edge */
        RectFill(rp, r->MinX, r->MaxY, r->MaxX - 1, r->MaxY);

        r->MinX++;
        r->MinY++;
        r->MaxX--;
        r->MaxY--;

    } /* PROPBORDERLESS */
    else
    {
        SetAPen(rp, pens[SHADOWPEN]);

        if (pi->Flags & FREEHORIZ)
        {
            /* black line at the left and at the right */

            RectFill(rp, r->MinX, r->MinY, r->MinX, r->MaxY);
            RectFill(rp, r->MaxX, r->MinY, r->MaxX, r->MaxY);

            r->MinX++;
            r->MaxX--;
        }

        if (pi->Flags & FREEVERT)
        {
            /* black line at the top and at the bottom */

            RectFill(rp, r->MinX, r->MinY, r->MaxX, r->MinY);
            RectFill(rp, r->MinX, r->MaxY, r->MaxX, r->MaxY);

            r->MinY++,
            r->MaxY--;
        }

    } /* not PROPBORDERLESS */

    SetAPen(rp, pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN]);

    /* interior */
    RectFill(rp, r->MinX, r->MinY, r->MaxX, r->MaxY);

    return TRUE;
}


/**************************************************************************************************/

IPTR WinDecorClass__WDM_INITWINDOW(Class *cl, Object *obj, struct wdpInitWindow *msg)
{
    return TRUE;
}

/**************************************************************************************************/

IPTR WinDecorClass__WDM_EXITWINDOW(Class *cl, Object *obj, struct wdpExitWindow *msg)
{
    return TRUE;
}

/**************************************************************************************************/

IPTR WinDecorClass__WDM_WINDOWSHAPE(Class *cl, Object *obj, struct wdpWindowShape *msg)
{
    return 0;
}
